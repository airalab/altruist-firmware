#include "message_formatter.h"
#include "value_crypto.h"
#include "../../utils.h"
#include "../../config_manager/config_defaults.h"
#include "../../sensors/sensor_names.h"

#include <Ed25519.h>
#include <string.h>

#define SCD4X_WARMUP_SEC 360

// Connectivity Keypair.verify() checks the raw message. ESPRobonomicsClient::signMessage
// goes through doSign(), which Blake2b-hashes payloads >256 bytes (Substrate extrinsic
// rule). CPS e.* values push the signed string over that limit → signature mismatch →
// HTTP 500. Map/custom HTTP must always sign the raw bytes.
static bool parseHex32Key(const char *hex, uint8_t out[32]) {
	if (!hex) {
		return false;
	}
	size_t n = strlen(hex);
	if (n >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
		hex += 2;
		n -= 2;
	}
	if (n != 64) {
		return false;
	}
	for (size_t i = 0; i < 32; i++) {
		auto nib = [](char c) -> int {
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			return -1;
		};
		const int hi = nib(hex[i * 2]);
		const int lo = nib(hex[i * 2 + 1]);
		if (hi < 0 || lo < 0) {
			return false;
		}
		out[i] = static_cast<uint8_t>((hi << 4) | lo);
	}
	return true;
}

static bool signMessageRaw(const String &message, String &signature, Robonomics *robonomics) {
	if (!robonomics) {
		return false;
	}
	uint8_t sk[32];
	uint8_t pk[32];
	if (!parseHex32Key(robonomics->getPrivateKey(), sk)) {
		debug_outln_error(F("[Map] Cannot parse device private key for raw sign"));
		return false;
	}
	Ed25519::derivePublicKey(pk, sk);

	uint8_t sig[64];
	Ed25519::sign(sig, sk, pk, reinterpret_cast<const uint8_t *>(message.c_str()), message.length());

	signature = "";
	signature.reserve(128);
	for (size_t i = 0; i < sizeof(sig); i++) {
		if (sig[i] < 0x10) {
			signature += '0';
		}
		signature += String(sig[i], HEX);
	}
	return true;
}

static bool maybeEncryptValue(const String &value, bool should_encrypt, String &formatted) {
	if (!should_encrypt || value.isEmpty()) {
		formatted = value;
		return true;
	}
	formatted = valueCryptoEncryptValue(value);
	return formatted.startsWith(VALUE_CRYPTO_CPS_PREFIX);
}

static bool appendRobonomicsValue(String &out, const __FlashStringHelper *alias, const String &value,
				  bool should_encrypt, bool *encrypted_any) {
	String formatted;
	if (!maybeEncryptValue(value, should_encrypt, formatted)) {
		out = "";
		return false;
	}
	if (should_encrypt && !value.isEmpty() && encrypted_any) {
		*encrypted_any = true;
	}
	out += alias;
	out += ':';
	out += formatted;
	out += ',';
	return true;
}

/** Robonomics parachain datalog pallet hard limit (see MaximumMessageSize). */
static constexpr size_t DATALOG_CHAIN_MAX_BYTES = 512;
static constexpr size_t DATALOG_CHAIN_SAFE_BYTES = 480;

static bool anyMetricEncryptionEnabled() {
	return cfg::encrypt_pm || cfg::encrypt_noise || cfg::encrypt_temperature || cfg::encrypt_humidity ||
	       cfg::encrypt_pressure || cfg::encrypt_co2 || cfg::encrypt_co || cfg::encrypt_radiation ||
	       cfg::encrypt_o3 || cfg::encrypt_no2 || cfg::encrypt_fast_aqi || cfg::encrypt_epa_aqi;
}

static bool appendRobonomicsFields(JsonDocument &data, String &out, bool per_field_encrypt,
				   bool *encrypted_any = nullptr) {
	out = "";
	if (encrypted_any) {
		*encrypted_any = false;
	}

	bool has_scd4x = !data[SCD4X_SENSOR_NAME].isNull();
	bool use_bme680_for_temp_hum = !has_scd4x || (millis() / 1000 < SCD4X_WARMUP_SEC);

	for (JsonPair sensor : data.as<JsonObject>()) {
        String sensor_name = sensor.key().c_str();

        JsonObject sensorData = sensor.value().as<JsonObject>();

        for (JsonPair measurement : sensorData) {
            String type = measurement.key().c_str();
            JsonObject measurementData = measurement.value().as<JsonObject>();
            String value;
			if (measurementData["value"].is<uint8_t>()) {
				value = String(measurementData["value"].as<uint8_t>());
			}else if (measurementData["value"].is<float>()) {
				value = String(measurementData["value"].as<float>(), 2);
			} else {
				value = measurementData["value"].as<String>();
			}

            bool is_bme680 = (sensor_name == BME680_SENSOR_NAME);
            bool is_scd4x = (sensor_name == SCD4X_SENSOR_NAME);
            bool skip_temp_hum = (is_bme680 && !use_bme680_for_temp_hum)
                              || (is_scd4x && use_bme680_for_temp_hum);

			const bool encrypt_climate = cfg::encrypt_temperature || cfg::encrypt_humidity;

			if (type == "P1" && cfg::share_pm) {
				if (!appendRobonomicsValue(out, F("p1"), value, per_field_encrypt && cfg::encrypt_pm, encrypted_any)) return false;
			} else if (type == "P2" && cfg::share_pm) {
				if (!appendRobonomicsValue(out, F("p2"), value, per_field_encrypt && cfg::encrypt_pm, encrypted_any)) return false;
			} else if (type == "noiseMax" && cfg::share_noise) {
				if (!appendRobonomicsValue(out, F("nm"), value, per_field_encrypt && cfg::encrypt_noise, encrypted_any)) return false;
			} else if (type == "noiseAvg" && cfg::share_noise) {
				if (!appendRobonomicsValue(out, F("na"), value, per_field_encrypt && cfg::encrypt_noise, encrypted_any)) return false;
			} else if (type == "temperature" && cfg::share_temperature && out.indexOf("t:") == -1 &&
				   !skip_temp_hum) {
				if (!appendRobonomicsValue(out, F("t"), value, per_field_encrypt && encrypt_climate, encrypted_any)) return false;
			} else if (type == "pressure" && cfg::share_pressure) {
				if (!appendRobonomicsValue(out, F("p"), value, per_field_encrypt && cfg::encrypt_pressure, encrypted_any)) return false;
			} else if (type == "humidity" && cfg::share_humidity && out.indexOf("h:") == -1 && !skip_temp_hum) {
				if (!appendRobonomicsValue(out, F("h"), value, per_field_encrypt && encrypt_climate, encrypted_any)) return false;
			} else if (type == "radiation") {
				if (!appendRobonomicsValue(out, F("gc"), value, per_field_encrypt && cfg::encrypt_radiation, encrypted_any)) return false;
			} else if (type == "co2" && cfg::share_co2) {
				if (!appendRobonomicsValue(out, F("co2"), value, per_field_encrypt && cfg::encrypt_co2, encrypted_any)) return false;
			} else if (type == "co" && cfg::share_co) {
				if (!appendRobonomicsValue(out, F("co"), value, per_field_encrypt && cfg::encrypt_co, encrypted_any)) return false;
			} else if (type == "o3" && cfg::share_o3) {
				if (!appendRobonomicsValue(out, F("o3"), value, per_field_encrypt && cfg::encrypt_o3, encrypted_any)) return false;
			} else if (type == "no2" && cfg::share_no2) {
				if (!appendRobonomicsValue(out, F("no2"), value, per_field_encrypt && cfg::encrypt_no2, encrypted_any)) return false;
			} else if (type == "fast_aqi" && cfg::share_fast_aqi) {
				if (!appendRobonomicsValue(out, F("fa"), value, per_field_encrypt && cfg::encrypt_fast_aqi, encrypted_any)) return false;
			} else if (type == "epa_aqi" && cfg::share_epa_aqi) {
				if (!appendRobonomicsValue(out, F("ea"), value, per_field_encrypt && cfg::encrypt_epa_aqi, encrypted_any)) return false;
			}
		}
	}
	if (out.length() > 0) {
		out.remove(out.length() - 1);
	}
	return true;
}

static const String *plainSampleForLog(const String &plain, bool encrypted) {
	if (plain.isEmpty()) {
		return nullptr;
	}
#if defined(ALTRUIST_BUILD_DEBUG)
	(void)encrypted;
	return &plain;
#else
	return encrypted ? nullptr : &plain;
#endif
}

static void logPayload(const __FlashStringHelper *channel, const char *encoding, bool encrypted,
			       size_t payload_len, const String *sample) {
	String line;
	line.reserve(112 + (sample ? sample->length() : 0));
	line = F("[PAYLOAD] channel=");
	line += channel;
	line += F(" encoding=");
	line += encoding;
	line += F(" encrypted=");
	line += encrypted ? '1' : '0';
	line += F(" payload_len=");
	line += String(payload_len);
	line += F(" sample_available=");
	line += sample ? '1' : '0';
	if (sample) {
		line += F(" sample=");
		line += *sample;
	}
	Serial.println(line);
}

bool formatRobonomicsString(JsonDocument &data, String &datalog_data, const __FlashStringHelper *channel) {
	bool encrypted_any = false;
	if (!appendRobonomicsFields(data, datalog_data, true, &encrypted_any)) {
		debug_outln_error(F("[Payload] Encryption failed; send aborted"));
		return false;
	}

	const String *sample = datalog_data.isEmpty() ? nullptr : &datalog_data;
#if defined(ALTRUIST_BUILD_DEBUG)
	String plain_sample;
	if (encrypted_any) {
		if (!appendRobonomicsFields(data, plain_sample, false)) {
			return false;
		}
		sample = plain_sample.isEmpty() ? nullptr : &plain_sample;
	}
#else
	if (encrypted_any) {
		sample = nullptr;
	}
#endif

	logPayload(channel, encrypted_any ? "mixed" : "plain", encrypted_any, datalog_data.length(), sample);
	debug_outln_verbose(F("[Payload] Wire data: "), datalog_data);
	return true;
}

DatalogFormatStatus formatRobonomicsDatalogString(JsonDocument &data, String &datalog_data) {
	String plain;
	if (!appendRobonomicsFields(data, plain, false)) {
		datalog_data = "";
		return DATALOG_FORMAT_ENCRYPTION_FAILED;
	}
	if (plain.isEmpty()) {
		datalog_data = plain;
		logPayload(F("datalog"), "plain", false, 0, nullptr);
		debug_outln_verbose(F("[Datalog] Plain record (empty): "), datalog_data);
		return DATALOG_FORMAT_PAYLOAD_EMPTY;
	}

	if (!anyMetricEncryptionEnabled()) {
		if (plain.length() > DATALOG_CHAIN_SAFE_BYTES) {
			logPayload(F("datalog"), "plain", false, plain.length(), plainSampleForLog(plain, false));
			debug_outln_error(F("[Datalog] Plain record too large for chain limit"));
			debug_outln_verbose(String(F("[Datalog] Record size: ")) + String(plain.length()) + F(" bytes (max ") +
					    String(DATALOG_CHAIN_MAX_BYTES) + F(")"));
			datalog_data = "";
			return DATALOG_FORMAT_PAYLOAD_TOO_LARGE;
		}
		datalog_data = plain;
		logPayload(F("datalog"), "plain", false, datalog_data.length(), plainSampleForLog(plain, false));
		debug_outln_verbose(F("[Datalog] Plain record: "), datalog_data);
		return DATALOG_FORMAT_PLAIN;
	}

	const String encrypted = valueCryptoEncryptValue(plain);
	if (encrypted.startsWith(VALUE_CRYPTO_CPS_PREFIX) &&
	    encrypted.length() <= DATALOG_CHAIN_SAFE_BYTES) {
		datalog_data = encrypted;
		logPayload(F("datalog"), "cps", true, datalog_data.length(), plainSampleForLog(plain, true));
		debug_outln_verbose(String(F("[Datalog] Bulk encrypted record (")) + String(datalog_data.length()) +
					    F(" bytes): ") + datalog_data);
		return DATALOG_FORMAT_CPS;
	}

	if (encrypted.startsWith(VALUE_CRYPTO_CPS_PREFIX) && encrypted.length() > DATALOG_CHAIN_SAFE_BYTES) {
		logPayload(F("datalog"), "cps", true, encrypted.length(), plainSampleForLog(plain, true));
		debug_outln_error(F("[Datalog] Bulk encrypted record too large for chain limit"));
		debug_outln_verbose(String(F("[Datalog] Record size: ")) + String(encrypted.length()) + F(" bytes (max ") +
				    String(DATALOG_CHAIN_MAX_BYTES) + F(")"));
		datalog_data = "";
		return DATALOG_FORMAT_PAYLOAD_TOO_LARGE;
	} else {
		logPayload(F("datalog"), "cps", true, 0, plainSampleForLog(plain, true));
		debug_outln_error(F("[Datalog] Bulk encrypt failed; not sending plaintext fallback"));
	}
	datalog_data = "";
	return DATALOG_FORMAT_ENCRYPTION_FAILED;
}

void addTimeAndSign(const String &data, String &signature, Robonomics *robonomics) {
  // Get the local time.
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    debug_outln_verbose(F("Failed to obtain time"));
    return;
  }
  debug_outln_verbose(F("Local time: "), String(timeinfo.tm_hour));
  
  // Convert local time to a Unix timestamp.
  time_t timestamp = mktime(&timeinfo);
  String timestampStr = String(timestamp);
  
  // Remove the last two digits from the timestamp string.
  if (timestampStr.length() > 2) {
    timestampStr = timestampStr.substring(0, timestampStr.length() - 2);
  }
  
  debug_outln_verbose(F("Modified Timestamp: "), timestampStr);

  String messageWithTimestamp = data + ",time:" + timestampStr;

  debug_outln_verbose(F("Message to sign: "), messageWithTimestamp);

  if (!signMessageRaw(messageWithTimestamp, signature, robonomics)) {
    signature = "";
    debug_outln_error(F("[Map] Raw Ed25519 sign failed"));
    return;
  }

  debug_outln_verbose(F("Signature: "), signature);
}

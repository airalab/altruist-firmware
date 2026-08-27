#include "proto_envelope.h"

/*
 * JSON + cfg → ProtoSample → codec. Arduino/JSON stay in this file only.
 */

#include "../../config_manager/config_defaults.h"
#include "../../sensors/sensor_names.h"
#include "../../utils.h"
#include "proto_codec.h"
#include "value_crypto.h"

#include <Ed25519.h>
#include <Robonomics.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SCD4X_WARMUP_SEC 360

static enum ProtoDest dest_of(int share, int encrypt) {
	/* encrypt wins: metric goes to private[], not public[]. */
	if (encrypt) {
		return PROTO_DEST_PRIVATE;
	}
	if (share) {
		return PROTO_DEST_PUBLIC;
	}
	return PROTO_DEST_SKIP;
}

static int json_double(JsonDocument &data, const char *sensor, const char *type, double *out) {
	JsonObject obj;
	const char *as_text;
	char *end;

	if (!out || data[sensor].isNull()) {
		return 0;
	}
	obj = data[sensor].as<JsonObject>();
	if (obj[type].isNull() || obj[type]["value"].isNull()) {
		return 0;
	}
	if (obj[type]["value"].is<float>()) {
		*out = obj[type]["value"].as<float>();
		return 1;
	}
	if (obj[type]["value"].is<double>()) {
		*out = obj[type]["value"].as<double>();
		return 1;
	}
	if (obj[type]["value"].is<int>()) {
		*out = (double)obj[type]["value"].as<int>();
		return 1;
	}
	if (obj[type]["value"].is<unsigned>()) {
		*out = (double)obj[type]["value"].as<unsigned>();
		return 1;
	}
	as_text = obj[type]["value"].as<const char *>();
	if (!as_text) {
		return 0;
	}
	*out = strtod(as_text, &end);
	return end != as_text;
}

static int read_gps(double *lat, double *lon) {
	if (!lat || !lon || sscanf(cfg::coords_gps, "%lf,%lf", lat, lon) != 2) {
		return 0;
	}
	return !(*lat == 0.0 && *lon == 0.0);
}

static void fill_sample(JsonDocument &data, struct ProtoSample *s) {
	memset(s, 0, sizeof(*s));
#if defined(ALTRUIST_INSIGHT)
	s->insight = 1;
#else
	s->insight = 0;
#endif
	s->has_gps = read_gps(&s->lat, &s->lon);
	s->dest_temperature = dest_of(cfg::share_temperature, cfg::encrypt_temperature);
	s->dest_humidity = dest_of(cfg::share_humidity, cfg::encrypt_humidity);
	s->dest_pressure = dest_of(cfg::share_pressure, cfg::encrypt_pressure);
	s->dest_pm = dest_of(cfg::share_pm, cfg::encrypt_pm);
	s->dest_noise = dest_of(cfg::share_noise, cfg::encrypt_noise);
	s->dest_co2 = dest_of(cfg::share_co2, cfg::encrypt_co2);

	if (s->insight) {
		const int has_scd = !data[SCD4X_SENSOR_NAME].isNull();
		/* SCD41 T/H are used only after warmup; until then BME680. */
		s->temp_from_scd41 = has_scd && ((esp_timer_get_time() / 1000000LL) >= SCD4X_WARMUP_SEC);
		s->has_pressure = json_double(data, BME680_SENSOR_NAME, "pressure", &s->pressure);
		s->has_co2 = json_double(data, SCD4X_SENSOR_NAME, "co2", &s->co2);
		if (s->temp_from_scd41) {
			s->has_temperature = json_double(data, SCD4X_SENSOR_NAME, "temperature", &s->temperature);
			s->has_humidity = json_double(data, SCD4X_SENSOR_NAME, "humidity", &s->humidity);
		} else {
			s->has_temperature = json_double(data, BME680_SENSOR_NAME, "temperature", &s->temperature);
			s->has_humidity = json_double(data, BME680_SENSOR_NAME, "humidity", &s->humidity);
		}
		return;
	}

	{
		const char *bme = data[BME_SENSOR_NAME].isNull() ? BMP_SENSOR_NAME : BME_SENSOR_NAME;
		s->pressure_hpa = 1;
		s->has_temperature = json_double(data, bme, "temperature", &s->temperature);
		s->has_humidity = json_double(data, bme, "humidity", &s->humidity);
		s->has_pressure = json_double(data, bme, "pressure", &s->pressure);
	}
	s->has_pm25 = json_double(data, SDS_SENSOR_NAME, "P2", &s->pm25);
	s->has_pm10 = json_double(data, SDS_SENSOR_NAME, "P1", &s->pm10);
	s->has_noise_max = json_double(data, I2S_NOISE_SENSOR_NAME, "noiseMax", &s->noise_max);
	s->has_noise_avg = json_double(data, I2S_NOISE_SENSOR_NAME, "noiseAvg", &s->noise_avg);
}

static int aead_cps(const uint8_t *plain, size_t plain_len, uint8_t from_pk[32], uint8_t nonce[12], uint8_t **cipher,
		    size_t *cipher_len) {
	return valueCryptoEncryptBytesForOwner(plain, plain_len, from_pk, nonce, cipher, cipher_len) ? 0 : -1;
}

struct SignCtx {
	uint8_t sk[32];
	uint8_t pk[32];
};

static void sign_ed25519(void *ctx, const uint8_t *msg, size_t len, uint8_t sig[64]) {
	struct SignCtx *keys = (struct SignCtx *)ctx;
	Ed25519::sign(sig, keys->sk, keys->pk, msg, len);
}

static void log_envelope(size_t envelope_len, size_t message_len, uint64_t timestamp_ms, const uint8_t sensor_id[32]) {
	char line[128];
	snprintf(line, sizeof(line),
		 "envelope_len=%u message_len=%u timestamp_ms=%" PRIu64 " sensor_id=%02x%02x%02x%02x...",
		 (unsigned)envelope_len, (unsigned)message_len, timestamp_ms, sensor_id[0], sensor_id[1], sensor_id[2],
		 sensor_id[3]);
	debug_outln_info(F("[PROTO] "), String(line));
}

ProtoBuildStatus protoBuildSignedEnvelope(JsonDocument &data, Robonomics *robonomics, uint8_t *out, size_t out_cap,
					  size_t *out_len) {
	struct ProtoSample sample;
	struct SignCtx keys;
	static uint8_t message[PROTO_MESSAGE_BUF_BYTES];
	size_t message_len = 0;
	uint8_t nonce[PROTO_ENVELOPE_NONCE_LEN];
	time_t now;
	uint64_t timestamp_ms;
	ProtoBuildStatus st;

	if (out_len) {
		*out_len = 0;
	}
	if (!protoProtocolEnabled()) {
		return PROTO_BUILD_DISABLED;
	}
	if (!out || !out_len || out_cap < 64) {
		return PROTO_BUILD_BUFFER_TOO_SMALL;
	}

	if (robonomics && valueCryptoParseHex32(robonomics->getPrivateKey(), keys.sk)) {
		Ed25519::derivePublicKey(keys.pk, keys.sk);
	} else if (!valueCryptoDeviceKeys(keys.sk, keys.pk)) {
		return PROTO_BUILD_KEY_FAILED;
	}

	fill_sample(data, &sample);
	if (!valueCryptoOwnerPublicKey(keys.pk, sample.owner)) {
		return PROTO_BUILD_KEY_FAILED;
	}

	st = proto_encode_message(&sample, aead_cps, message, sizeof(message), &message_len);
	if (st != PROTO_BUILD_OK) {
		return st;
	}

	now = time(NULL);
	if (now < 1600000000) {
		/* NTP not ready: unix ms would be wrong and signature would not verify. */
		return PROTO_BUILD_SIGN_FAILED;
	}
	timestamp_ms = (uint64_t)now * 1000ULL;
	esp_fill_random(nonce, sizeof(nonce));

	st = proto_encode_envelope(message, message_len, keys.pk, timestamp_ms, nonce, sizeof(nonce), sign_ed25519,
				   &keys, out, out_cap, out_len);
	if (st == PROTO_BUILD_OK) {
		log_envelope(*out_len, message_len, timestamp_ms, keys.pk);
	}
	return st;
}

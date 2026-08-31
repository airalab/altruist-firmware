#include <Arduino.h>

#include "lora_uart.h"

#if defined(CONFIG_IDF_TARGET_ESP32C6) && defined(ALTRUIST_URBAN)

#include "../../config_manager/config_defaults.h"
#include "../../defines.h"
#include "../../sensors/sensor_names.h"
#include "../../utils.h"
#include "value_crypto.h"

#include <Ed25519.h>
#include <cstdlib>
#include <cstring>
#include <math.h>
#include <mbedtls/base64.h>
#include <stdio.h>
#include <time.h>

namespace {

// Meshtastic Data.payload is 237 bytes. Typical signed JSON is ~232; CRLF is extra.
constexpr size_t LORA_UART_MAX_LINE_BYTES = 237;
constexpr unsigned long LORA_UART_MIN_INTERVAL_MS = 30000UL;
constexpr time_t LORA_UART_MIN_UNIX_TS = 1609459200; // 2021-01-01

unsigned long last_send_ms = 0;
bool uart_ready = false;

bool readMeasurementNumber(
    const JsonDocument& source,
    const char* sensor,
    const char* measurement,
    double& number
) {
	const JsonVariantConst value = source[sensor][measurement]["value"];
	if (value.isNull()) {
		return false;
	}
	if (value.is<const char*>()) {
		const char* text = value.as<const char*>();
		if (!text || !*text) {
			return false;
		}
		char* end = nullptr;
		number = strtod(text, &end);
		if (!end || *end != '\0') {
			return false;
		}
	} else {
		number = value.as<double>();
	}
	return isfinite(number);
}

void appendJsonKey(String& body, bool& first, const char* key)
{
	if (!first) {
		body += ',';
	}
	first = false;
	body += '"';
	body += key;
	body += "\":";
}

bool appendMeasurement(
    const JsonDocument& source,
    String& body,
    bool& first,
    const char* sensor,
    const char* measurement,
    const char* alias,
    const char* fmt
) {
	double number = 0;
	if (!readMeasurementNumber(source, sensor, measurement, number)) {
		return false;
	}
	char buf[24];
	if (snprintf(buf, sizeof(buf), fmt, number) <= 0) {
		return false;
	}
	appendJsonKey(body, first, alias);
	body += buf;
	return true;
}

bool appendSignature(const String& body, String& line)
{
	if (body.length() < 2 || body[body.length() - 1] != '}') {
		return false;
	}

	uint8_t sk[VALUE_CRYPTO_KEY_LEN];
	uint8_t pk[VALUE_CRYPTO_KEY_LEN];
	if (!valueCryptoDeviceKeys(sk, pk)) {
		debug_outln_error(F("[LoRa UART] no device key; unsigned JSONL skipped"));
		return false;
	}

	uint8_t sig[64];
	Ed25519::sign(
	    sig,
	    sk,
	    pk,
	    reinterpret_cast<const uint8_t*>(body.c_str()),
	    body.length()
	);

	unsigned char b64[96];
	size_t olen = 0;
	if (mbedtls_base64_encode(b64, sizeof(b64), &olen, sig, sizeof(sig)) != 0 || olen == 0) {
		debug_outln_error(F("[LoRa UART] signature base64 failed"));
		return false;
	}
	b64[olen] = '\0';

	line = body;
	line.remove(line.length() - 1);
	line += F(",\"s\":\"");
	line += reinterpret_cast<const char*>(b64);
	line += F("\"}");
	return true;
}

} // namespace

void setupLoRaUart()
{
	if (!cfg::lora_uart_enabled) {
		return;
	}
	Debug.beginStructuredOutput(LORA_UART_BAUD, LORA_UART_RX_PIN, LORA_UART_TX_PIN);
	uart_ready = true;
	Serial.printf(
	    "[LoRa UART] JSONL enabled: TX=GPIO%d RX=GPIO%d baud=%d max=%u interval=%lus signed=ed25519\r\n",
	    LORA_UART_TX_PIN,
	    LORA_UART_RX_PIN,
	    LORA_UART_BAUD,
	    static_cast<unsigned int>(LORA_UART_MAX_LINE_BYTES),
	    static_cast<unsigned long>(cfg::lora_uart_sending_intervall_ms) / 1000UL
	);
}

void sendLoRaTelemetryIfDue(const JsonDocument& data, const char* sensor_id)
{
	if (!uart_ready || !cfg::lora_uart_enabled) {
		return;
	}

	const unsigned long interval =
	    cfg::lora_uart_sending_intervall_ms < LORA_UART_MIN_INTERVAL_MS
	        ? LORA_UART_MIN_INTERVAL_MS
	        : cfg::lora_uart_sending_intervall_ms;
	if (last_send_ms != 0 && msSince(last_send_ms) < interval) {
		return;
	}

	if (!sensor_id || !*sensor_id || strcmp(sensor_id, "Not Set") == 0) {
		debug_outln_error(F("[LoRa UART] no SS58 id; unsigned JSONL skipped"));
		last_send_ms = millis();
		return;
	}

	const time_t now = time(nullptr);
	if (now < LORA_UART_MIN_UNIX_TS) {
		debug_outln_error(F("[LoRa UART] no valid ts; waiting for time"));
		return;
	}

	String body;
	body.reserve(160);
	body += '{';
	bool first = true;
	appendJsonKey(body, first, "id");
	body += '"';
	body += sensor_id;
	body += '"';
	appendJsonKey(body, first, "ts");
	body += String(static_cast<uint32_t>(now));

	unsigned int measurements = 0;
	measurements += appendMeasurement(data, body, first, SDS_SENSOR_NAME, "P1", "p1", "%.1f");
	measurements += appendMeasurement(data, body, first, SDS_SENSOR_NAME, "P2", "p2", "%.1f");
	measurements += appendMeasurement(data, body, first, BME_SENSOR_NAME, "temperature", "t", "%.1f");
	measurements += appendMeasurement(data, body, first, BME_SENSOR_NAME, "humidity", "h", "%.1f");
	measurements += appendMeasurement(data, body, first, BME_SENSOR_NAME, "pressure", "p", "%.0f");
	measurements += appendMeasurement(data, body, first, I2S_NOISE_SENSOR_NAME, "noiseAvg", "n", "%.0f");
	measurements += appendMeasurement(data, body, first, I2S_NOISE_SENSOR_NAME, "noiseMax", "nm", "%.0f");
	if (measurements == 0) {
		return;
	}
	body += '}';

	String line;
	if (!appendSignature(body, line)) {
		last_send_ms = millis();
		return;
	}
	if (line.length() > LORA_UART_MAX_LINE_BYTES) {
		Serial.printf(
		    "[LoRa UART] JSONL payload exceeds %u bytes (%u); skipped\r\n",
		    static_cast<unsigned int>(LORA_UART_MAX_LINE_BYTES),
		    static_cast<unsigned int>(line.length())
		);
		last_send_ms = millis();
		return;
	}

	if (Debug.writeStructuredLine(line)) {
		last_send_ms = millis();
		debug_outln_info(F("[LoRa UART] JSONL sent, bytes="), String(line.length()));
	} else {
		debug_outln_error(F("[LoRa UART] write failed"));
	}
}

#else

void setupLoRaUart() {}

void sendLoRaTelemetryIfDue(const JsonDocument& data, const char* sensor_id)
{
	(void)data;
	(void)sensor_id;
}

#endif

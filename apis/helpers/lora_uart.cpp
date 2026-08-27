#include "lora_uart.h"

#include "../../config_manager/config_defaults.h"
#include "../../defines.h"
#include "../../sensors/sensor_names.h"
#include "../../utils.h"

#include <cstdlib>
#include <cstring>
#include <time.h>

namespace {

constexpr size_t LORA_UART_MAX_LINE_BYTES = 220;
constexpr unsigned long LORA_UART_MIN_INTERVAL_MS = 30000UL;

unsigned long last_send_ms = 0;
bool uart_ready = false;

bool addMeasurement(
    const JsonDocument& source,
    StaticJsonDocument<384>& output,
    const char* sensor,
    const char* measurement,
    const char* alias
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
		const double number = strtod(text, &end);
		if (!end || *end != '\0') {
			return false;
		}
		output[alias] = number;
	} else {
		output[alias] = value;
	}
	return true;
}

} // namespace

void setupLoRaUart()
{
#if defined(CONFIG_IDF_TARGET_ESP32C6) && defined(ALTRUIST_URBAN)
	if (!cfg::lora_uart_enabled) {
		return;
	}
	Debug.beginStructuredOutput(LORA_UART_BAUD, LORA_UART_RX_PIN, LORA_UART_TX_PIN);
	uart_ready = true;
	Serial.printf(
	    "[LoRa UART] JSONL enabled: TX=GPIO%d RX=GPIO%d baud=%d max=%u\r\n",
	    LORA_UART_TX_PIN,
	    LORA_UART_RX_PIN,
	    LORA_UART_BAUD,
	    static_cast<unsigned int>(LORA_UART_MAX_LINE_BYTES)
	);
#endif
}

void sendLoRaTelemetryIfDue(const JsonDocument& data, const char* sensor_id)
{
#if defined(CONFIG_IDF_TARGET_ESP32C6) && defined(ALTRUIST_URBAN)
	if (!uart_ready || !cfg::lora_uart_enabled) {
		return;
	}

	const unsigned long interval =
	    cfg::sending_intervall_ms < LORA_UART_MIN_INTERVAL_MS
	        ? LORA_UART_MIN_INTERVAL_MS
	        : cfg::sending_intervall_ms;
	if (last_send_ms != 0 && msSince(last_send_ms) < interval) {
		return;
	}

	StaticJsonDocument<384> output;
	output["v"] = 1;
	if (sensor_id && *sensor_id && strcmp(sensor_id, "Not Set") != 0) {
		output["id"] = sensor_id;
	}
	const time_t now = time(nullptr);
	if (now >= 1609459200) {
		output["ts"] = static_cast<uint32_t>(now);
	}

	unsigned int measurements = 0;
	measurements += addMeasurement(data, output, SDS_SENSOR_NAME, "P1", "p1");
	measurements += addMeasurement(data, output, SDS_SENSOR_NAME, "P2", "p2");
	measurements += addMeasurement(data, output, BME_SENSOR_NAME, "temperature", "t");
	measurements += addMeasurement(data, output, BME_SENSOR_NAME, "humidity", "h");
	measurements += addMeasurement(data, output, BME_SENSOR_NAME, "pressure", "p");
	measurements += addMeasurement(data, output, I2S_NOISE_SENSOR_NAME, "noiseAvg", "n");
	measurements += addMeasurement(data, output, I2S_NOISE_SENSOR_NAME, "noiseMax", "nm");
	if (measurements == 0) {
		return;
	}

	String line;
	line.reserve(LORA_UART_MAX_LINE_BYTES + 1);
	serializeJson(output, line);
	if (line.length() > LORA_UART_MAX_LINE_BYTES) {
		debug_outln_error(F("[LoRa UART] JSONL payload exceeds 220 bytes; skipped"));
		return;
	}

	if (Debug.writeStructuredLine(line)) {
		last_send_ms = millis();
		debug_outln_info(F("[LoRa UART] JSONL sent, bytes="), String(line.length()));
	} else {
		debug_outln_error(F("[LoRa UART] write failed"));
	}
#else
	(void)data;
	(void)sensor_id;
#endif
}

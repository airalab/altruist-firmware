#include "lora_uart.h"

#if defined(CONFIG_IDF_TARGET_ESP32C6) && defined(ALTRUIST_URBAN)

#include "../../config_manager/config_defaults.h"
#include "../../defines.h"
#include "../../utils.h"
#include "meshtastic_frame.h"
#include "meshtastic_toradio.h"
#include "proto_envelope.h"

#include <Arduino.h>
#include <ctype.h>
#include <esp_random.h>
#include <stdlib.h>
#include <string.h>

namespace {

constexpr unsigned long LORA_UART_MIN_INTERVAL_MS = 30000UL;
constexpr unsigned long LORA_HEARTBEAT_MS = 15000UL;
constexpr unsigned LORA_HANDSHAKE_BOOT_MS = 2500;
constexpr unsigned long LORA_HANDSHAKE_RETRY_MS = 15000UL;
constexpr size_t LORA_RX_MAX = 512;

unsigned long last_send_ms = 0;
unsigned long last_heartbeat_ms = 0;
unsigned long last_handshake_ms = 0;
bool uart_ready = false;
bool session_up = false;
uint32_t want_config_id = 0;

enum RxState : uint8_t { RxMagic1, RxMagic2, RxLenHi, RxLenLo, RxPayload };
RxState rx_state = RxMagic1;
uint16_t rx_len = 0;
size_t rx_got = 0;
uint8_t rx_buf[LORA_RX_MAX];

bool parseDestNode(uint32_t *dest)
{
	if (!dest) {
		return false;
	}
	String raw = String(cfg::lora_dest_node);
	raw.trim();
	if (raw.length() == 0 || raw.equalsIgnoreCase(F("not set"))) {
		return false;
	}
	if (raw.charAt(0) == '!') {
		raw.remove(0, 1);
	}
	if (raw.length() == 0 || raw.length() > 8) {
		return false;
	}
	for (unsigned i = 0; i < raw.length(); ++i) {
		if (!isxdigit(static_cast<unsigned char>(raw.charAt(i)))) {
			return false;
		}
	}
	char *end = nullptr;
	const unsigned long value = strtoul(raw.c_str(), &end, 16);
	if (!end || *end != '\0' || value == 0 || value == MESHTASTIC_BROADCAST_NODE) {
		return false;
	}
	*dest = static_cast<uint32_t>(value);
	return true;
}

uint32_t nextNonzeroRandom()
{
	uint32_t value = 0;
	while (value == 0) {
		value = esp_random();
	}
	return value;
}

void handleFromRadio(const uint8_t *pb, size_t pb_len)
{
	uint32_t complete_id = 0;
	if (meshtasticFromRadioConfigComplete(pb, pb_len, &complete_id) && complete_id == want_config_id &&
	    want_config_id != 0) {
		session_up = true;
		debug_outln_info(F("[LoRa UART] Meshtastic session up"));
	}
}

void pumpRx()
{
	while (Debug.structuredAvailable() > 0) {
		const int raw = Debug.readStructured();
		if (raw < 0) {
			break;
		}
		const uint8_t byte = static_cast<uint8_t>(raw);
		switch (rx_state) {
		case RxMagic1:
			if (byte == MESHTASTIC_SERIAL_START1) {
				rx_state = RxMagic2;
			}
			break;
		case RxMagic2:
			rx_state = (byte == MESHTASTIC_SERIAL_START2) ? RxLenHi : RxMagic1;
			break;
		case RxLenHi:
			rx_len = static_cast<uint16_t>(byte) << 8;
			rx_state = RxLenLo;
			break;
		case RxLenLo:
			rx_len |= byte;
			if (rx_len == 0 || rx_len > LORA_RX_MAX) {
				rx_state = RxMagic1;
				break;
			}
			rx_got = 0;
			rx_state = RxPayload;
			break;
		case RxPayload:
			rx_buf[rx_got++] = byte;
			if (rx_got >= rx_len) {
				handleFromRadio(rx_buf, rx_len);
				rx_state = RxMagic1;
			}
			break;
		}
	}
}

bool writeSerial(const uint8_t *packet, size_t len)
{
	return Debug.writeStructuredBytes(packet, len);
}

bool writeWantConfig()
{
	want_config_id = nextNonzeroRandom();
	uint8_t packet[32];
	const size_t n = meshtasticEncodeWantConfig(want_config_id, packet, sizeof(packet));
	if (n == 0 || !writeSerial(packet, n)) {
		debug_outln_error(F("[LoRa UART] want_config_id write failed"));
		want_config_id = 0;
		return false;
	}
	last_handshake_ms = millis();
	if (last_handshake_ms == 0) {
		last_handshake_ms = 1;
	}
	return true;
}

bool handshakeBlocking(unsigned timeout_ms)
{
	session_up = false;
	if (!writeWantConfig()) {
		return false;
	}
	const unsigned long start = millis();
	while (msSince(start) < timeout_ms) {
		pumpRx();
		if (session_up) {
			return true;
		}
		delay(10);
	}
	pumpRx();
	if (!session_up) {
		debug_outln_error(F("[LoRa UART] Meshtastic handshake timeout"));
	}
	return session_up;
}

void maintainSession()
{
	pumpRx();
	if (session_up) {
		return;
	}
	if (last_handshake_ms != 0 && msSince(last_handshake_ms) < LORA_HANDSHAKE_RETRY_MS) {
		return;
	}
	if (want_config_id != 0) {
		debug_outln_error(F("[LoRa UART] Meshtastic handshake timeout"));
	}
	session_up = false;
	writeWantConfig();
}

void maybeHeartbeat()
{
	if (!session_up) {
		return;
	}
	if (last_heartbeat_ms != 0 && msSince(last_heartbeat_ms) < LORA_HEARTBEAT_MS) {
		return;
	}
	uint8_t packet[32];
	const size_t n = meshtasticEncodeHeartbeat(nextNonzeroRandom(), packet, sizeof(packet));
	if (n != 0 && writeSerial(packet, n)) {
		last_heartbeat_ms = millis();
	}
}

bool writeToRadio(const uint8_t *frame, size_t frame_len, uint32_t dest)
{
	uint8_t packet[4 + 32 + MESHTASTIC_TRANSPORT_MTU + 96];
	const size_t n = meshtasticEncodeToRadioUnicast(dest, MESHTASTIC_PORTNUM_PRIVATE_APP, nextNonzeroRandom(), frame,
							frame_len, packet, sizeof(packet));
	if (n == 0) {
		debug_outln_error(F("[LoRa UART] ToRadio encode failed"));
		return false;
	}
	if (!writeSerial(packet, n)) {
		debug_outln_error(F("[LoRa UART] UART write failed"));
		return false;
	}
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
	    "[LoRa UART] Meshtastic v1: TX=GPIO%d RX=GPIO%d baud=%d port=%u interval=%lus\r\n",
	    LORA_UART_TX_PIN,
	    LORA_UART_RX_PIN,
	    LORA_UART_BAUD,
	    static_cast<unsigned int>(MESHTASTIC_PORTNUM_PRIVATE_APP),
	    static_cast<unsigned long>(cfg::lora_uart_sending_intervall_ms) / 1000UL
	);
	handshakeBlocking(LORA_HANDSHAKE_BOOT_MS);
}

void sendLoRaTelemetryIfDue(JsonDocument &data)
{
	if (!uart_ready || !cfg::lora_uart_enabled) {
		return;
	}

	maintainSession();
	maybeHeartbeat();
	if (!session_up) {
		return;
	}

	const unsigned long interval =
	    cfg::lora_uart_sending_intervall_ms < LORA_UART_MIN_INTERVAL_MS
	        ? LORA_UART_MIN_INTERVAL_MS
	        : cfg::lora_uart_sending_intervall_ms;
	if (last_send_ms != 0 && msSince(last_send_ms) < interval) {
		return;
	}

	uint32_t dest = 0;
	if (!parseDestNode(&dest)) {
		debug_outln_error(F("[LoRa UART] dest node unset or broadcast; PKI unicast required"));
		last_send_ms = millis();
		return;
	}

	static uint8_t message[PROTO_MESSAGE_BUF_BYTES];
	size_t message_len = 0;
	const ProtoBuildStatus st = protoBuildMessage(&data, message, sizeof(message), &message_len);
	if (st != PROTO_BUILD_OK || message_len == 0) {
		debug_outln_info(F("[LoRa UART] message skipped: "), String(protoBuildStatusReason(st)));
		if (st == PROTO_BUILD_SIGN_FAILED) {
			return;
		}
		last_send_ms = millis();
		return;
	}

	const uint8_t count = meshtasticFragmentCount(message_len);
	if (count == 0) {
		debug_outln_error(F("[LoRa UART] Message exceeds Meshtastic v1 size"));
		last_send_ms = millis();
		return;
	}

	static uint8_t frame[MESHTASTIC_TRANSPORT_MTU];
	if (count == 1) {
		const size_t frame_len = meshtasticEncodeSingle(message, message_len, frame, sizeof(frame));
		if (frame_len == 0 || !writeToRadio(frame, frame_len, dest)) {
			return;
		}
		last_send_ms = millis();
		debug_outln_info(F("[LoRa UART] SINGLE Message sent, bytes="), String(static_cast<unsigned>(message_len)));
		return;
	}

	uint8_t message_id[MESHTASTIC_MESSAGE_ID_LEN];
	if (!meshtasticMessageId(message, message_len, message_id)) {
		debug_outln_error(F("[LoRa UART] message_id failed"));
		last_send_ms = millis();
		return;
	}

	for (uint8_t i = 0; i < count; ++i) {
		const size_t frame_len =
		    meshtasticEncodeFragment(message, message_len, message_id, i, count, frame, sizeof(frame));
		if (frame_len == 0 || !writeToRadio(frame, frame_len, dest)) {
			debug_outln_error(F("[LoRa UART] fragment send failed"));
			return;
		}
		pumpRx();
	}
	last_send_ms = millis();
	debug_outln_info(F("[LoRa UART] FRAGMENT Message sent, bytes="), String(static_cast<unsigned>(message_len)));
}

#else

void setupLoRaUart() {}

void sendLoRaTelemetryIfDue(JsonDocument &data)
{
	(void)data;
}

#endif

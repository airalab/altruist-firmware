#ifndef ALTRUIST_LORA_UART_H
#define ALTRUIST_LORA_UART_H

#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_ARDUINO_PRINT 0
#define ARDUINOJSON_DECODE_UNICODE 0
#include <ArduinoJson.h>

void setupLoRaUart();
/* Prototype: core.v1.Message in a Meshtastic DM. SINGLE 0x01, or FRAGMENT 0x41 if >220 bytes. */
void sendLoRaTelemetryIfDue(JsonDocument &data);

#endif

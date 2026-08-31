#ifndef ALTRUIST_LORA_UART_H
#define ALTRUIST_LORA_UART_H

#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_ARDUINO_PRINT 0
#define ARDUINOJSON_DECODE_UNICODE 0
#include <ArduinoJson.h>

void setupLoRaUart();
// Compact JSONL: {"id","ts","p1","p2","t","h","p","n","nm","s"}.
// `s` is standard base64(Ed25519(body without s)). Unsigned lines are not sent.
void sendLoRaTelemetryIfDue(const JsonDocument& data, const char* sensor_id);

#endif

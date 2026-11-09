#include "improv_serial.h"
#include "../defines.h"
#include "../utils.h"
#include "../wifi_manager.h"
#include <WiFi.h>
#include <cstring>

#define IMPROV_SERIAL_VERSION 1

#define IMPROV_TYPE_CURRENT_STATE 0x01
#define IMPROV_TYPE_ERROR_STATE 0x02
#define IMPROV_TYPE_RPC 0x03
#define IMPROV_TYPE_RPC_RESPONSE 0x04

#define IMPROV_CMD_WIFI_SETTINGS 0x01
#define IMPROV_CMD_IDENTIFY 0x02
#define IMPROV_CMD_GET_CURRENT_STATE 0x02
#define IMPROV_CMD_GET_DEVICE_INFO 0x03
#define IMPROV_CMD_GET_WIFI_NETWORKS 0x04
#define IMPROV_CMD_HOSTNAME 0x05
#define IMPROV_CMD_DEVICE_NAME 0x06
#define IMPROV_CMD_BAD_CHECKSUM 0xFF
#define IMPROV_CMD_CUSTOM_RWS_OWNER 0x80

#define IMPROV_STATE_STOPPED 0x00
#define IMPROV_STATE_AWAITING_AUTH 0x01
#define IMPROV_STATE_AUTHORIZED 0x02
#define IMPROV_STATE_PROVISIONING 0x03
#define IMPROV_STATE_PROVISIONED 0x04

#define IMPROV_ERROR_NONE 0x00
#define IMPROV_ERROR_INVALID_RPC 0x01
#define IMPROV_ERROR_UNKNOWN_RPC 0x02
#define IMPROV_ERROR_UNABLE_TO_CONNECT 0x03
#define IMPROV_ERROR_NOT_AUTHORIZED 0x04
#define IMPROV_ERROR_UNKNOWN 0xFF

static const char* const IMPROV_TAG = "[IMPROV]";

static ImprovWifiCallback s_wifi_cb = nullptr;
static ImprovRwsOwnerCallback s_rws_owner_cb = nullptr;
static uint8_t s_improv_buf[256];
static size_t s_improv_pos = 0;
static uint8_t s_improv_state = IMPROV_STATE_STOPPED;
static uint8_t s_improv_announce_remaining = 0;
static unsigned long s_improv_last_announce_ms = 0;
static constexpr unsigned long IMPROV_ANNOUNCE_INTERVAL_MS = 500;

static void improv_send_error(uint8_t error);

static void improv_send_state(uint8_t state) {
    uint8_t checksum = 'I' + 'M' + 'P' + 'R' + 'O' + 'V' + IMPROV_SERIAL_VERSION;
    checksum += IMPROV_TYPE_CURRENT_STATE;
    checksum += 1;
    checksum += state;
    Serial.write('I'); Serial.write('M'); Serial.write('P');
    Serial.write('R'); Serial.write('O'); Serial.write('V');
    Serial.write(IMPROV_SERIAL_VERSION);
    Serial.write(IMPROV_TYPE_CURRENT_STATE);
    Serial.write(1);
    Serial.write(state);
    Serial.write(checksum);
    Serial.write('\n');
    Serial.flush();
}

void improv_set_state(uint8_t state) {
    s_improv_state = state;
    improv_send_state(state);
}

void improv_start_announce(uint8_t count) {
    s_improv_announce_remaining = count;
}

void improv_set_error(uint8_t error) {
    improv_send_error(error);
}

static void improv_send_error(uint8_t error) {
    uint8_t checksum = 'I' + 'M' + 'P' + 'R' + 'O' + 'V' + IMPROV_SERIAL_VERSION;
    checksum += IMPROV_TYPE_ERROR_STATE;
    checksum += 1;
    checksum += error;
    Serial.write('I'); Serial.write('M'); Serial.write('P');
    Serial.write('R'); Serial.write('O'); Serial.write('V');
    Serial.write(IMPROV_SERIAL_VERSION);
    Serial.write(IMPROV_TYPE_ERROR_STATE);
    Serial.write(1);
    Serial.write(error);
    Serial.write(checksum);
    Serial.write('\n');
    Serial.flush();
}

static void improv_send_rpc_response(uint8_t cmd, const std::vector<String>& datum) {
    size_t strings_len = 0;
    for (const auto& s : datum) {
        strings_len += 1 + s.length();
    }
    if (strings_len + 2 + 10 > 256) return;

    uint8_t buf[256];
    buf[0] = 'I'; buf[1] = 'M'; buf[2] = 'P'; buf[3] = 'R'; buf[4] = 'O'; buf[5] = 'V';
    buf[6] = IMPROV_SERIAL_VERSION;
    buf[7] = IMPROV_TYPE_RPC_RESPONSE;

    size_t pos = 9;
    buf[pos++] = cmd;
    buf[pos++] = (uint8_t)strings_len;
    for (const auto& s : datum) {
        buf[pos++] = (uint8_t)s.length();
        memcpy(buf + pos, s.c_str(), s.length());
        pos += s.length();
    }
    buf[8] = (uint8_t)(pos - 9);

    uint32_t checksum = 0;
    for (size_t i = 0; i < pos; i++) checksum += buf[i];
    buf[pos] = (uint8_t)(checksum & 0xFF);
    pos++;

    Serial.write(buf, pos);
    Serial.write('\n');
    Serial.flush();
}

void improv_send_response(const std::vector<String>& datum) {
    improv_send_rpc_response(IMPROV_CMD_WIFI_SETTINGS, datum);
}

static void improv_handle_rpc(const uint8_t* data, size_t len) {
    if (len < 1) {
        improv_send_error(IMPROV_ERROR_INVALID_RPC);
        return;
    }

    uint8_t cmd = data[0];

    if (cmd == IMPROV_CMD_GET_CURRENT_STATE) {
        improv_set_state(s_improv_state);
    } else if (cmd == IMPROV_CMD_GET_DEVICE_INFO) {
        std::vector<String> info;
        info.push_back(F("Altruist"));
#ifdef ALTRUIST_INSIGHT
        info.push_back(F("Altruist-Insight"));
#else
        info.push_back(F("Altruist-Urban"));
#endif
        info.push_back(String(SOFTWARE_VERSION_STR));
        info.push_back(F("altruist-firmware"));
        improv_send_rpc_response(IMPROV_CMD_GET_DEVICE_INFO, info);
    } else if (cmd == IMPROV_CMD_GET_WIFI_NETWORKS) {
        int8_t count = WiFi.scanNetworks(false, true);
        for (int8_t i = 0; i < count && i < 15; i++) {
            std::vector<String> net;
            net.push_back(WiFi.SSID(i));
            net.push_back(String(WiFi.RSSI(i)));
            net.push_back(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "YES" : "NO");
            improv_send_rpc_response(IMPROV_CMD_GET_WIFI_NETWORKS, net);
        }
        WiFi.scanDelete();
        improv_send_rpc_response(IMPROV_CMD_GET_WIFI_NETWORKS, std::vector<String>());
    } else if (cmd == IMPROV_CMD_WIFI_SETTINGS) {
        if (len < 2) {
            improv_send_error(IMPROV_ERROR_INVALID_RPC);
            return;
        }
        uint8_t payload_len = data[1];
        if ((size_t)(2 + payload_len) > len) {
            improv_send_error(IMPROV_ERROR_INVALID_RPC);
            return;
        }
        uint8_t ssid_len = data[2];
        if ((size_t)(3 + ssid_len) > 2 + payload_len) {
            improv_send_error(IMPROV_ERROR_INVALID_RPC);
            return;
        }
        String ssid((const char*)(data + 3), ssid_len);
        uint8_t pwd_len = data[3 + ssid_len];
        if ((size_t)(4 + ssid_len + pwd_len) > 2 + payload_len) {
            improv_send_error(IMPROV_ERROR_INVALID_RPC);
            return;
        }
        String password((const char*)(data + 4 + ssid_len), pwd_len);

        // Spec: clear error, then Provisioning. Do not send an RPC result yet —
        // ESP Web Tools treats the first WIFI_SETTINGS result as "Connect done".
        // An empty result made the UI stop waiting, then a later failure/reboot
        // showed "Unable to connect" even when Wi-Fi was saved and the device restarted.
        improv_send_error(IMPROV_ERROR_NONE);
        improv_set_state(IMPROV_STATE_PROVISIONING);

        bool connected = false;
        if (s_wifi_cb) {
            debugSetUsbQuiet(true);
            connected = s_wifi_cb(ssid, password);
            if (!connected) {
                debugSetUsbQuiet(false);
            }
        }

        if (connected) {
            improv_send_error(IMPROV_ERROR_NONE);
            improv_set_state(IMPROV_STATE_PROVISIONED);
            std::vector<String> result;
            result.push_back(String(F("http://")) + WiFi.localIP().toString() + F("/"));
            improv_send_rpc_response(IMPROV_CMD_WIFI_SETTINGS, result);
            Serial.flush();
            delay(400);
            wifiRequestImprovProvisionRestart();
        } else {
            improv_set_error(IMPROV_ERROR_UNABLE_TO_CONNECT);
            improv_set_state(IMPROV_STATE_AUTHORIZED);
        }
    } else if (cmd == IMPROV_CMD_CUSTOM_RWS_OWNER) {
        if (len < 2) {
            improv_send_error(IMPROV_ERROR_INVALID_RPC);
            return;
        }
        uint8_t payload_len = data[1];
        if ((size_t)(2 + payload_len) > len) {
            improv_send_error(IMPROV_ERROR_INVALID_RPC);
            return;
        }
        uint8_t owner_len = data[2];
        if ((size_t)(3 + owner_len) > 2 + payload_len) {
            improv_send_error(IMPROV_ERROR_INVALID_RPC);
            return;
        }
        String rws_owner((const char*)(data + 3), owner_len);

        if (s_rws_owner_cb) {
            s_rws_owner_cb(rws_owner);
        }
        improv_send_rpc_response(IMPROV_CMD_CUSTOM_RWS_OWNER, std::vector<String>());
    } else {
        improv_send_error(IMPROV_ERROR_UNKNOWN_RPC);
    }
}

void improv_serial_setup() {
    s_improv_pos = 0;
    s_improv_state = IMPROV_STATE_AUTHORIZED;
    improv_set_state(IMPROV_STATE_AUTHORIZED);
}

void improv_serial_loop() {
    if (s_improv_announce_remaining > 0 && s_improv_state != IMPROV_STATE_STOPPED) {
        unsigned long now = millis();
        if (now - s_improv_last_announce_ms >= IMPROV_ANNOUNCE_INTERVAL_MS) {
            improv_send_state(s_improv_state);
            s_improv_announce_remaining--;
            s_improv_last_announce_ms = now;
        }
    }
    while (Serial.available()) {
        uint8_t byte = Serial.read();
        if (s_improv_pos == 0 && byte == 'I') {
            s_improv_announce_remaining = 0;
        }
        if (s_improv_pos == 0 && byte != 'I') continue;
        if (s_improv_pos == 1 && byte != 'M') { s_improv_pos = 0; continue; }
        if (s_improv_pos == 2 && byte != 'P') { s_improv_pos = 0; continue; }
        if (s_improv_pos == 3 && byte != 'R') { s_improv_pos = 0; continue; }
        if (s_improv_pos == 4 && byte != 'O') { s_improv_pos = 0; continue; }
        if (s_improv_pos == 5 && byte != 'V') { s_improv_pos = 0; continue; }

        s_improv_buf[s_improv_pos++] = byte;

        if (s_improv_pos < 7) continue;
        if (s_improv_pos == 6 && byte != IMPROV_SERIAL_VERSION) {
            s_improv_pos = 0;
            continue;
        }

        uint8_t type = s_improv_buf[7];
        uint8_t data_len = s_improv_buf[8];

        if (s_improv_pos < 9 + data_len + 1) continue;

        uint8_t checksum = 0;
        for (size_t i = 0; i < s_improv_pos - 1; i++) checksum += s_improv_buf[i];
        if (checksum != s_improv_buf[s_improv_pos - 1]) {
            // USB CDC also carries boot/debug text. Do not send INVALID_RPC (webflasher
            // shows that as "Unknown error (1)") for leftover noise after a valid exchange.
            s_improv_pos = 0;
            continue;
        }

        if (s_improv_state == IMPROV_STATE_STOPPED) {
            s_improv_pos = 0;
            continue;
        }

        if (type == IMPROV_TYPE_RPC) {
            if (s_improv_state == IMPROV_STATE_AUTHORIZED) {
                improv_handle_rpc(&s_improv_buf[9], data_len);
            } else if (s_improv_state == IMPROV_STATE_PROVISIONING && data_len >= 1 &&
                       s_improv_buf[9] == IMPROV_CMD_GET_CURRENT_STATE) {
                improv_send_state(s_improv_state);
            }
        }

        s_improv_pos = 0;
    }
}

void improv_set_wifi_callback(ImprovWifiCallback cb) {
    s_wifi_cb = cb;
}

void improv_set_rws_owner_callback(ImprovRwsOwnerCallback cb) {
    s_rws_owner_cb = cb;
}
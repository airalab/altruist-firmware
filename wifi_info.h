// wifi_info.h
#ifndef __WIFI_INFO_H__
#define __WIFI_INFO_H__

#include <Arduino.h>

struct struct_wifiInfo {
    char ssid[32];
    uint8_t encryptionType;
    int32_t RSSI;
    uint8_t channel;
    bool isHidden;
};

/** Max networks shown in captive-portal Wi‑Fi list (stack buffers). */
constexpr uint8_t WIFI_SCAN_LIST_MAX = 20;

#endif // __WIFI_INFO_H__
#ifdef ALTRUIST_INSIGHT

#ifndef __HTTP_ALTRUIST_H__
#define __HTTP_ALTRUIST_H__

#include "sensor.h"
#include "HTTPClient.h"
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <vector>
#include "../defines.h"

#define JSON_DATA_PATH "/data.json"
#define SENSOR_URL_PREFIX "http://"

class HTTPAltruistSensor : public Sensor
{

public:
    HTTPAltruistSensor(unsigned long sending_timeout = 1000UL);
    bool begin() override;
    /** Advance Urban view. Returns false at the last entry (no wrap). */
    bool cycleNext(JsonDocument &data);
    bool cycleView();
    bool cycleViewNoWrap();
    bool cyclePrevNoWrap();
    void syncToJson(JsonDocument &data);
    void requestImmediateFetch();
    uint8_t pagerIndex() const;
    uint8_t pagerTotal() const;
    bool copyCurrentSnap(float *pm10, float *pm25, float *noise_avg, float *noise_max,
                         float *temp, float *hum, float *press_pa, char *ip, size_t ip_len) const;
    bool copySnapForIp(const String &ip, float *pm10, float *pm25, float *noise_avg, float *noise_max,
                       float *temp, float *hum, float *press_pa) const;

private:
    bool _discoverSensors();
    void _fetch(JsonDocument &data) override;
    void _fetch_one_sensor(JsonDocument &data, HTTPClient& http, WiFiClient& client,
			   const String &ip_address, bool apply_to_json, bool quick);
    void _writePager(JsonDocument &data);
    void _applySnap(JsonDocument &data, uint8_t cache_index);
    int _cacheIndexForIp(const String &ip) const;
    int _ensureCacheSlot(const String &ip);
    void _capAddressList();
    void _ensureExtraUrbans();
    std::vector<String> sensor_addresses;
    String chosen_address;
    unsigned long last_success_time = 0;
    uint8_t       consecutive_failures = 0;
    // Discovery retry bookkeeping: when Urban is not initially present,
    // we perform a limited number of rediscovery attempts spaced in time.
    unsigned long last_discovery_attempt_time = 0;
    uint8_t       discovery_attempts = 0;
    uint8_t       view_index = 0;
    uint8_t       bg_rotate = 0;

    static constexpr uint8_t kMaxUrbans = 1 + MAX_EXTRA_URBANS;
    struct UrbanSnap {
        String ip;
        bool valid = false;
        uint32_t last_ok_ms = 0;
        String ss58;
        float pm10 = -1;
        float pm25 = -1;
        float noise_avg = -1;
        float noise_max = -1;
        float temp = -1000;
        float hum = -1;
        float press = -1;
    };
    UrbanSnap urban_cache[kMaxUrbans];
};

bool httpUrbanCycleNext(JsonDocument &data);
bool httpUrbanCycleNext();
/** MAIN like graphs: true = wrapped past last/first, caller should change screen. */
bool httpUrbanCycleNextView();
bool httpUrbanCyclePrevView();
bool httpUrbanCanCycleOnMain();
void httpUrbanResetToMainPage();
void httpUrbanSyncToJson(JsonDocument &data);
bool httpUrbanGetOutdoorView(uint8_t *index1, uint8_t *total, bool *snap_valid,
                             float *pm10, float *pm25, float *noise_avg, float *noise_max,
                             float *temp, float *hum, float *press_pa, char *ip, size_t ip_len,
                             char *name, size_t name_len, bool *solo);

#endif // __HTTP_ALTRUIST_H__

#endif

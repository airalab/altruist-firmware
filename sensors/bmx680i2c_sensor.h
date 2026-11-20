#ifndef __BMX680_SENSOR_H__
#define __BMX680_SENSOR_H__

#include "Arduino.h"
#include "sensor.h"

#include "driver/i2c.h"
#include "drivers/i2c.h"
#include "drivers/bme680/bme680.h"

#define BME680_ADDR_PRIMARY 0x76
#define BME680_ADDR_SECONDARY 0x77


class BME680Sensor : public Sensor {
public:
    explicit BME680Sensor(unsigned long sending_timeout = 300000UL);
    ~BME680Sensor();
    bool begin();

protected:
    void _fetch(JsonDocument &data) override;

private:
#if defined(ALTRUIST_BUILD_DEBUG)
    uart_v2::Probe debugGasProbe;
#endif
    Adafruit_BME680 *bme680 = nullptr;
    uint8_t sensor_address = 0;

    // Последние считанные значения
    float last_temperature_value = 0.0f;
    float last_pressure_value = 0.0f;
    float last_humidity_value = 0.0f;
    uint32_t last_gas_resistance_value = 0;
    float gas_baseline_ohm = 0.0f;
    uint8_t gas_baseline_samples = 0;
    uint16_t voc_spikes_today = 0;
    /** 0 = none, 1 = low, 2 = medium, 3 = high; -1 = not ready. */
    int8_t voc_level = -1;
    /** 0–100 from ABC ppb (2000 ppb = 100). Kept in JSON for older readers. */
    uint8_t voc_index = 0;
    /** display scale 0–10000 ppb vs 5-day clean-air max R. */
    uint16_t voc_ppb = 0;
    /** Unsmoothed ABC ppb — graph min/max. */
    uint16_t voc_ppb_raw = 0;
    int voc_spikes_yday = -1;
    int voc_spikes_year = -1;
    bool voc_event_open = false;
    uint8_t voc_event_samples = 0;
    bool prev_gas_valid = false;
    float prev_gas_ohm = 0.0f;
    float prev_temp_c = 0.0f;

    static constexpr uint8_t kAbcHours = 120; // 5 days of hourly max compensated R
    uint32_t abc_slot[kAbcHours] = {};
    uint32_t abc_slot_hour[kAbcHours] = {}; // unix hour for each slot; 0 = empty
    uint32_t abc_cur_hour = 0;
    uint32_t abc_cur_max = 0;
    float abc_r_clean = 0.0f;
    float abc_r_session = 0.0f; // post-skip max R, only while 5-day ABC is not primed
    float voc_ppb_f = 0.0f;
    bool voc_ppb_smooth = false;
    bool abc_dirty = false;
    bool abc_from_file = false; // 5-day baseline loaded (ABC3 or migrated ABC2)
    uint8_t abc_boot_skip = 6; // ~30 min after boot: do not raise ABC yet

    void updateVocSpikeDetector();
    float abcAbsHumidity(float t_c, float rh) const;
    float abcCompensate(float r_ohm, float t_c, float rh) const;
    void abcNoteSample(float r_comp, bool cooling, bool allow_baseline);
    void abcClearRing();
    void abcRefreshClean();
    bool abcPrimed() const;
    uint16_t abcToPpb(float r_comp, float r_clean) const;
    void applyVocFromAbc(float r_comp, bool cooling);
    void loadVocDay();
    void persistVocDay();
    void loadVocAbc();
    void persistVocAbc();

    // Gas / VOC (#172): 5 min forced sample. Shorter intervals (60 s) heated the case;
    // perfume-length odors still miss the enclosure. Sustained events fit this cadence.
    static constexpr unsigned long BME680_SENSOR_MIN_TIMEOUT = 300000UL;
};

#endif //__BMX680_SENSOR_H__

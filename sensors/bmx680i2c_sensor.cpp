#include "bmx680i2c_sensor.h"
#include "../intl.h"
#include "../config_manager/config_helpers.h"
#include "sensor_names.h"
#include "../utils.h"

#include <time.h>
#include <string.h>
#include <math.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

#define ROOM_TEMP_OFFSET_C     -3.0f   // Измеренное смещение
#define ESP_TEMP_NOMINAL_C     30.0f   // Типичная температура ESP32-C6 в режиме ожидания
#define ESP_TEMP_FACTOR        0.25f   // °C датчика на °C ESP
#define MAX_ESP_COMP_C         2.5f    // Ограничение безопасности

#define RH_PER_DEG_C           2.0f    // %RH на °C

BME680Sensor::BME680Sensor(unsigned long sending_timeout)
    : Sensor(sending_timeout) 
    {
    if (sending_timeout > BME680_SENSOR_MIN_TIMEOUT) {
        timeout = sending_timeout;
    } else {
        timeout = BME680_SENSOR_MIN_TIMEOUT;
    }
    sensor_name = BME680_SENSOR_NAME;
}

BME680Sensor::~BME680Sensor() {
    delete bme680;
}

bool BME680Sensor::begin() {
    debug_outln_info(F("Begin BME680Sensor"));
    I2cBusLock bus;
    if (!bus.ok()) {
        logSupportEvent(DEBUG_ERROR, F("sensor"), F("bme680_bus_lock_failed"));
        debug_outln_error(F("BME680 I2C bus lock failed"));
        return false;
    }

    for (uint8_t addr : {0x77, 0x76}) {
        auto test_bme680 = new Adafruit_BME680(I2C_NUM_0, addr);
        if (test_bme680->begin()) {
            // VOC probe: heater 320 °C / 150 ms.
            // T/H/P of the same forced sample are taken before the heater runs.
            test_bme680->setGasHeater(320, 150);
            bme680 = test_bme680;
            sensor_address = addr;
            break;
        }
        delete test_bme680; // не наш адрес
    }

    if (bme680) {
        debug_outln_info(F("BME680 Sensor started at address: 0x"), String(sensor_address, HEX));
        debug_outln_info(F("Fetch interval (sec): "), String(timeout / 1000));
        loadVocDay();
        loadVocAbc();
        abc_boot_skip = 6;
        last_fetch_time = millis() - timeout;
        return true;
    }

    logSupportEvent(DEBUG_ERROR, F("sensor"), F("bme680_not_found"));
    return false;
}


// bool BME680Sensor::begin() {
//     debug_outln_info(F("Begin BME680Sensor"));
//     i2c_master_init();
//     bool res = bme680.begin(); 
//     deinit_i2c();
//     if (res) {
//         debug_outln_info(F("BME680 Sensor started with fetch interval (sec): "), String(timeout / 1000));
//     }
//     last_fetch_time = millis() - timeout;
//     return res;
// }

void BME680Sensor::_fetch(JsonDocument &data) {
    debug_outln_verbose(F("fetch BME680"));

    float raw_temp = 0.0f;
    float raw_humidity = 0.0f;
    float pressure = 0.0f;
    uint32_t gas_ohm = 0;
    {
        I2cBusLock bus;
        if (!bus.ok()) {
            logSupportEvent(DEBUG_ERROR, F("sensor"), F("bme680_bus_lock_failed"));
            debug_outln_error(F("BME680 I2C bus lock failed in fetch"));
            return;
        }

        if (!bme680->performReading()) {
            logSupportEvent(DEBUG_ERROR, F("sensor"), F("bme680_read_failed"));
            debug_outln_error(F("BME680 reading failed"));
            return;
        }

        raw_temp     = bme680->temperature;
        raw_humidity = bme680->humidity;
        pressure     = bme680->pressure;
        gas_ohm      = bme680->gas_resistance;
    }

    // ---------------- СЫРЫЕ ЗНАЧЕНИЯ ----------------
    // Config offset is in hPa; BME680 reports Pa.
    pressure += readCorrectionOffset(cfg::pressure_correction) * 100.0f;

    // ---------------- ТЕМПЕРАТУРА ----------------
    // 1. Фиксированное смещение комнатной температуры
    float corrected_temp = raw_temp + ROOM_TEMP_OFFSET_C;

    // 2. Компенсация самонагрева ESP
    float esp_temp = getESPTemperature();
    float esp_comp = 0.0f;

    if (!isnan(esp_temp)) {
        float esp_excess = esp_temp - ESP_TEMP_NOMINAL_C;

        if (esp_excess > 0.0f) {
            esp_comp = esp_excess * ESP_TEMP_FACTOR;
            if (esp_comp > MAX_ESP_COMP_C) {
                esp_comp = MAX_ESP_COMP_C;
            }

            corrected_temp -= esp_comp;
        }

        if (corrected_temp < -40.0f) corrected_temp = -40.0f;
        if (corrected_temp > 85.0f)  corrected_temp = 85.0f;

        debug_outln_verbose(
            F("[BME680] Temp comp: raw="),
            String(raw_temp, 1) +
            F("°C, offset=") + String(ROOM_TEMP_OFFSET_C, 1) +
            F("°C, esp=") + String(esp_temp, 1) +
            F("°C, esp_comp=") + String(esp_comp, 2) +
            F("°C, final=") + String(corrected_temp, 1) + F("°C")
        );
    } else {
        debug_outln_verbose(F("[BME680] ESP temp unavailable, skipping ESP compensation"));
    }

    // ---------------- ВЛАЖНОСТЬ ----------------
    // Компенсация нагрева ESP: когда температура датчика скорректирована вниз,
    // нужно вернуть влажность, которая была "потеряна" из-за нагрева
    // Влажность уменьшается примерно на 4.5% на °C при типичных комнатных условиях
    // temp_error положителен, когда мы скорректировали температуру ВНИЗ (сырое значение было слишком высоким)
    float temp_error = raw_temp - corrected_temp;
    float corrected_humidity = raw_humidity + (temp_error * RH_PER_DEG_C);

    if (corrected_humidity > 100.0f) corrected_humidity = 100.0f;
    if (corrected_humidity < 0.0f)  corrected_humidity = 0.0f; 

    // ---------------- СГЛАЖИВАНИЕ ----------------
    // Применяем простое экспоненциальное скользящее среднее для уменьшения скачков
    // alpha = 0.2 => новое значение 20%, старое 80%
    static float smoothed_humidity = corrected_humidity;  // сохраняем состояние между вызовами
    const float alpha = 0.2f;
    smoothed_humidity = smoothed_humidity * (1.0f - alpha) + corrected_humidity * alpha;

    // ---------------- СОХРАНЕНИЕ ----------------
    last_temperature_value = corrected_temp;
    last_humidity_value    = smoothed_humidity;  // Используем сглаженное значение
    last_pressure_value    = pressure;
    last_gas_resistance_value = gas_ohm;
    updateVocSpikeDetector();

    // ---------------- ОТЛАДКА ----------------
    debug_outln_verbose(F("BME680 temperature: "), String(last_temperature_value, 1));
    debug_outln_verbose(
        F("[BME680] Humidity comp: raw="),
        String(raw_humidity, 1) +
        F("%, temp_error=") + String(temp_error, 2) +
        F("°C, corrected=") + String(corrected_humidity, 1) +
        F("%, smoothed=") + String(smoothed_humidity, 1) + F("%")
    );
    debug_outln_verbose(F("BME680 pressure: "), String(last_pressure_value));
    debug_outln_info(F("BME680 gas resistance (Ohm)"), String(last_gas_resistance_value));

    // ---------------- JSON ----------------
    addValueToJSON(data, F("temperature"), last_temperature_value, INTL_TEMPERATURE, F("°C"));
    addValueToJSON(data, F("pressure"),    last_pressure_value,    INTL_PRESSURE,    F("Pa"));
    addValueToJSON(data, F("humidity"),    last_humidity_value,    INTL_HUMIDITY,    F("%"));
    const char *voc_phrase = INTL_VOC_PHRASE_NONE;
    if (voc_level >= 3) {
        voc_phrase = INTL_VOC_PHRASE_HIGH;
    } else if (voc_level == 2) {
        voc_phrase = INTL_VOC_PHRASE_MED;
    } else if (voc_level == 1) {
        voc_phrase = INTL_VOC_PHRASE_LOW;
    }
    addValueToJSON(data, F("voc_phrase"), String(voc_phrase), INTL_VOC_ORGANIC, F(""));
    addValueToJSON(data, F("voc_ppb"), voc_ppb, INTL_VOC_PPB, F("ppb"));
    addValueToJSON(data, F("voc_index"), voc_index, INTL_VOC_INDEX, INTL_VOC_AU);
    addValueToJSON(data, F("voc_level"), voc_level, INTL_VOC_ORGANIC, F(""));
    addValueToJSON(data, F("voc_spikes_today"), voc_spikes_today, INTL_VOC_SPIKES_TODAY, F(""));
    addValueToJSON(data, F("gas_resistance"), last_gas_resistance_value, INTL_GAS_RESISTANCE, F("Ohm"));
    addValueToJSON(data, F("gas_baseline"), (uint32_t)(gas_baseline_ohm + 0.5f), INTL_GAS_BASELINE, F("Ohm"));

#if defined(ALTRUIST_BUILD_DEBUG)
    serializeJson(data, Serial);
    Serial.println();
#endif
}

void BME680Sensor::updateVocSpikeDetector() {
    // Spikes (#172): sudden drop vs the short rolling baseline, not ppm.
    // ppb: ABC vs 5-day max gas resistance (cleanest air).
    // Ignore drops while the package is cooling — MOX resistance follows T/H.
    // Spike count is local calendar day (until 00:00), persisted on SPIFFS.
    static constexpr uint8_t kWarmupSamples = 4;
    static constexpr float kSpikeRatio = 0.92f;
    static constexpr float kSuddenRatio = 0.92f;
    static constexpr float kRecoverRatio = 0.92f;
    static constexpr uint8_t kEventTimeoutSamples = 6;
    static constexpr float kCoolingDeltaC = 0.6f;

    const uint16_t count_before = voc_spikes_today;
    const bool event_before = voc_event_open;
    const int yday_before = voc_spikes_yday;
    const int year_before = voc_spikes_year;

    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
        if (voc_spikes_year != timeinfo.tm_year || voc_spikes_yday != timeinfo.tm_yday) {
            voc_spikes_year = timeinfo.tm_year;
            voc_spikes_yday = timeinfo.tm_yday;
            voc_spikes_today = 0;
            voc_event_open = false;
            voc_event_samples = 0;
        }
    }

    auto persist_if_changed = [this, count_before, event_before, yday_before, year_before]() {
        if (voc_spikes_today != count_before || voc_event_open != event_before ||
            voc_spikes_yday != yday_before || voc_spikes_year != year_before) {
            persistVocDay();
        }
    };

    const uint32_t r = last_gas_resistance_value;
    if (r == 0) {
        voc_level = -1;
        voc_index = 0;
        voc_ppb = 0;
        persist_if_changed();
        return;
    }

    const float rf = (float)r;
    const float t = last_temperature_value;
    const bool cooling = prev_gas_valid && (prev_temp_c - t) > kCoolingDeltaC;
    const bool sudden = prev_gas_valid && (rf < prev_gas_ohm * kSuddenRatio);

    if (gas_baseline_samples < kWarmupSamples) {
        if (gas_baseline_ohm <= 0.0f) {
            gas_baseline_ohm = rf;
        } else {
            gas_baseline_ohm = 0.5f * gas_baseline_ohm + 0.5f * rf;
        }
        gas_baseline_samples++;
        prev_gas_ohm = rf;
        prev_temp_c = t;
        prev_gas_valid = true;
        voc_level = 0;
        voc_index = 0;
        voc_ppb = 0;
        const float r_comp = abcCompensate(rf, t, last_humidity_value);
        const bool allow_abc = (abc_boot_skip == 0);
        if (abc_boot_skip > 0) {
            abc_boot_skip--;
        }
        abcNoteSample(r_comp, cooling, allow_abc);
        applyVocFromAbc(r_comp, cooling);
        persistVocAbc();
        debug_outln_info(F("BME680 gas baseline warmup"),
            String((int)gas_baseline_samples) + F("/") + String((int)kWarmupSamples) +
            F(" R=") + String(r) + F(" baseline=") + String((uint32_t)(gas_baseline_ohm + 0.5f)));
        persist_if_changed();
        return;
    }

    if (cooling && voc_event_open) {
        voc_event_open = false;
        voc_event_samples = 0;
        debug_outln_info(F("BME680 VOC spike ignored (cooling)"),
            String(F("R=")) + String(r) + F(" dT=") + String(prev_temp_c - t, 2));
    }

    if (!voc_event_open && sudden && !cooling && rf < gas_baseline_ohm * kSpikeRatio) {
        voc_event_open = true;
        voc_event_samples = 0;
        if (voc_spikes_today < 65535) {
            voc_spikes_today++;
        }
        debug_outln_info(F("BME680 VOC spike"),
            String(F("today=")) + String(voc_spikes_today) +
            F(" R=") + String(r) +
            F(" baseline=") + String((uint32_t)(gas_baseline_ohm + 0.5f)));
    } else if (voc_event_open) {
        if (voc_event_samples < 255) {
            voc_event_samples++;
        }
        const bool recovered = rf >= gas_baseline_ohm * kRecoverRatio;
        const bool timed_out = voc_event_samples >= kEventTimeoutSamples;
        if (recovered || timed_out) {
            voc_event_open = false;
            voc_event_samples = 0;
            debug_outln_info(timed_out ? F("BME680 VOC spike timed out") : F("BME680 VOC spike recovered"),
                             String(r));
        }
    } else if (!voc_event_open && rf < gas_baseline_ohm * kSpikeRatio && (cooling || !sudden)) {
        debug_outln_info(F("BME680 VOC drop skipped"),
            String(F("R=")) + String(r) +
            F(" baseline=") + String((uint32_t)(gas_baseline_ohm + 0.5f)) +
            (cooling ? F(" cooling") : F(" slow")));
    }

    if (!voc_event_open) {
        if (rf >= gas_baseline_ohm) {
            gas_baseline_ohm = 0.4f * rf + 0.6f * gas_baseline_ohm;
        } else if (cooling) {
            gas_baseline_ohm = 0.35f * rf + 0.65f * gas_baseline_ohm;
        } else {
            gas_baseline_ohm = 0.1f * rf + 0.9f * gas_baseline_ohm;
        }
    }

    const float r_comp = abcCompensate(rf, t, last_humidity_value);
    const bool allow_abc = (abc_boot_skip == 0);
    if (abc_boot_skip > 0) {
        abc_boot_skip--;
    }
    abcNoteSample(r_comp, cooling, allow_abc);
    applyVocFromAbc(r_comp, cooling);
    persistVocAbc();
    debug_outln_info(F("BME680 VOC ppb"),
        String(voc_ppb) + F(" R=") + String(r) +
        F(" Rc=") + String((uint32_t)(r_comp + 0.5f)) +
        F(" Rclean=") + String((uint32_t)(abc_r_clean + 0.5f)));

    prev_gas_ohm = rf;
    prev_temp_c = t;
    prev_gas_valid = true;
    persist_if_changed();
}

void BME680Sensor::loadVocDay() {
    if (!SPIFFS.begin(true)) {
        return;
    }
    File f = SPIFFS.open(F("/voc_day.json"), "r");
    if (!f) {
        return;
    }
    DynamicJsonDocument doc(256);
    const DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        return;
    }
    voc_spikes_year = doc["y"] | -1;
    voc_spikes_yday = doc["d"] | -1;
    voc_spikes_today = doc["n"] | 0;
    voc_event_open = doc["e"] | false;

    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
        if (voc_spikes_year != timeinfo.tm_year || voc_spikes_yday != timeinfo.tm_yday) {
            voc_spikes_year = timeinfo.tm_year;
            voc_spikes_yday = timeinfo.tm_yday;
            voc_spikes_today = 0;
            voc_event_open = false;
            voc_event_samples = 0;
            persistVocDay();
        }
    }
    debug_outln_info(F("BME680 VOC day loaded"), String(voc_spikes_today));
}

void BME680Sensor::persistVocDay() {
    if (!SPIFFS.begin(true)) {
        return;
    }
    DynamicJsonDocument doc(256);
    doc["y"] = voc_spikes_year;
    doc["d"] = voc_spikes_yday;
    doc["n"] = voc_spikes_today;
    doc["e"] = voc_event_open;
    File f = SPIFFS.open(F("/voc_day.json"), "w");
    if (!f) {
        logSupportEvent(DEBUG_ERROR, F("sensor"), F("bme680_voc_save_failed"));
        debug_outln_error(F("BME680 VOC day save failed"));
        return;
    }
    serializeJson(doc, f);
    f.close();
}

// ABC on BME680 (320 °C / 150 ms). Humidity: water vapor drops MOX R and looks
// like VOC. Compensate with Magnus absolute humidity:
// R_comp = R * exp(0.03 * AH[g/m³]).
//
// Baseline: 90th percentile of hourly *max* R_comp over 5 days — not a single
// glitch. Persist /voc_abc.bin.
//
// Scale: 0–10000 ppb, Good <250 / Elevated 250–2000 / High ≥2000.
// Relative MOS equivalent, not lab TVOC. Logarithmic:
//   ppb = 3248 * ln(R_clean / R_comp)   so ~8% drop → 250, 50% drop → ~2250.
// First ~30 min after boot does not raise the baseline.
static constexpr uint32_t kAbcMagic = 0x32434241u; // "ABC2" (humidity-compensated R)
static constexpr float kAbcHumSlope = 0.03f;
static constexpr float kAbcLnGain = 3248.4f; // 250 / ln(1.08)
static constexpr uint16_t kAbcMaxPpb = 10000;
static constexpr uint16_t kAbcGoodPpb = 250;
static constexpr uint16_t kAbcPoorPpb = 2000;
static constexpr float kAbcSmooth = 0.45f;

static bool abcUnixHour(uint32_t *out) {
    if (!out) {
        return false;
    }
    const time_t now = time(nullptr);
    if (now < 1700000000LL) {
        return false;
    }
    *out = (uint32_t)((uint64_t)now / 3600ULL);
    return true;
}

float BME680Sensor::abcAbsHumidity(float t_c, float rh) const {
    if (t_c < -40.0f) {
        t_c = -40.0f;
    } else if (t_c > 85.0f) {
        t_c = 85.0f;
    }
    if (rh < 0.0f) {
        rh = 0.0f;
    } else if (rh > 100.0f) {
        rh = 100.0f;
    }
    const float e_hpa = 6.112f * expf((17.625f * t_c) / (243.04f + t_c));
    return 216.7f * (rh / 100.0f) * e_hpa / (t_c + 273.15f);
}

float BME680Sensor::abcCompensate(float r_ohm, float t_c, float rh) const {
    if (r_ohm <= 0.0f) {
        return 0.0f;
    }
    const float ah = abcAbsHumidity(t_c, rh);
    float factor = expf(kAbcHumSlope * ah);
    if (factor < 1.0f) {
        factor = 1.0f;
    } else if (factor > 4.0f) {
        factor = 4.0f;
    }
    return r_ohm * factor;
}

void BME680Sensor::abcRefreshClean() {
    uint32_t tmp[kAbcHours + 1];
    uint8_t n = 0;
    for (uint8_t i = 0; i < kAbcHours; i++) {
        if (abc_slot[i] > 0) {
            tmp[n++] = abc_slot[i];
        }
    }
    if (abc_cur_max > 0) {
        tmp[n++] = abc_cur_max;
    }
    if (n == 0) {
        abc_r_clean = 0.0f;
        return;
    }
    for (uint8_t i = 1; i < n; i++) {
        const uint32_t v = tmp[i];
        uint8_t j = i;
        while (j > 0 && tmp[j - 1] > v) {
            tmp[j] = tmp[j - 1];
            j--;
        }
        tmp[j] = v;
    }
    uint8_t idx = (uint8_t)((uint16_t)n * 9u / 10u);
    if (idx >= n) {
        idx = (uint8_t)(n - 1);
    }
    abc_r_clean = (float)tmp[idx];
}

void BME680Sensor::abcNoteSample(float r_comp, bool cooling, bool allow_baseline) {
    if (cooling || r_comp <= 0.0f) {
        return;
    }
    if (r_comp < 1000.0f || r_comp > 8000000.0f) {
        return;
    }
    const uint32_t r = (uint32_t)(r_comp + 0.5f);
    if (!allow_baseline) {
        if (abc_r_clean <= 0.0f) {
            abc_r_clean = r_comp;
        }
        return;
    }
    uint32_t hour = 0;
    if (!abcUnixHour(&hour)) {
        if ((float)r > abc_r_clean) {
            abc_r_clean = (float)r;
            abc_cur_max = r;
            abc_dirty = true;
        }
        return;
    }
    if (abc_cur_hour == 0) {
        abc_cur_hour = hour;
        if (r > abc_cur_max) {
            abc_cur_max = r;
        }
        if (abc_r_clean > (float)abc_cur_max) {
            abc_cur_max = (uint32_t)(abc_r_clean + 0.5f);
        }
        abc_dirty = true;
    } else if (hour != abc_cur_hour) {
        if (abc_cur_max > 0) {
            abc_slot[abc_cur_hour % kAbcHours] = abc_cur_max;
        }
        if (hour > abc_cur_hour) {
            const uint32_t gap = hour - abc_cur_hour;
            if (gap >= (uint32_t)kAbcHours) {
                memset(abc_slot, 0, sizeof(abc_slot));
            } else {
                for (uint32_t h = abc_cur_hour + 1; h < hour; h++) {
                    abc_slot[h % kAbcHours] = 0;
                }
            }
        }
        abc_cur_hour = hour;
        abc_cur_max = r;
        abc_dirty = true;
    } else if (r > abc_cur_max) {
        abc_cur_max = r;
        abc_dirty = true;
    }
    abcRefreshClean();
}

uint16_t BME680Sensor::abcToPpb(float r_comp) const {
    if (abc_r_clean <= 0.0f || r_comp <= 0.0f) {
        return 0;
    }
    float ratio = abc_r_clean / r_comp;
    if (ratio < 1.0f) {
        return 0;
    }
    if (ratio > 50.0f) {
        ratio = 50.0f;
    }
    const float ppb = kAbcLnGain * logf(ratio);
    if (ppb < 0.0f) {
        return 0;
    }
    if (ppb > (float)kAbcMaxPpb) {
        return kAbcMaxPpb;
    }
    return (uint16_t)(ppb + 0.5f);
}

void BME680Sensor::applyVocFromAbc(float r_comp, bool cooling) {
    if (cooling) {
        return;
    }
    if (abc_r_clean <= 0.0f) {
        voc_ppb = 0;
        voc_ppb_f = 0.0f;
        voc_index = 0;
        voc_level = 0;
        return;
    }
    const uint16_t raw = abcToPpb(r_comp);
    if (!voc_ppb_smooth) {
        voc_ppb_f = (float)raw;
        voc_ppb_smooth = true;
    } else {
        voc_ppb_f = kAbcSmooth * (float)raw + (1.0f - kAbcSmooth) * voc_ppb_f;
    }
    voc_ppb = (uint16_t)(voc_ppb_f + 0.5f);
    if (voc_ppb > kAbcMaxPpb) {
        voc_ppb = kAbcMaxPpb;
    }
    uint32_t idx = ((uint32_t)voc_ppb * 100u + (uint32_t)kAbcPoorPpb / 2u) / (uint32_t)kAbcPoorPpb;
    voc_index = idx > 100u ? 100 : (uint8_t)idx;
    if (voc_ppb >= kAbcPoorPpb) {
        voc_level = 3;
    } else if (voc_ppb >= 1000) {
        voc_level = 2;
    } else if (voc_ppb >= kAbcGoodPpb) {
        voc_level = 1;
    } else {
        voc_level = 0;
    }
}

void BME680Sensor::loadVocAbc() {
    if (!SPIFFS.begin(true)) {
        return;
    }
    File f = SPIFFS.open(F("/voc_abc.bin"), "r");
    if (!f) {
        return;
    }
    const size_t need = 12 + sizeof(abc_slot);
    if (f.size() < need) {
        f.close();
        return;
    }
    uint32_t magic = 0;
    if (f.read((uint8_t *)&magic, 4) != 4 || magic != kAbcMagic) {
        f.close();
        return;
    }
    if (f.read((uint8_t *)&abc_cur_hour, 4) != 4 ||
        f.read((uint8_t *)&abc_cur_max, 4) != 4 ||
        f.read((uint8_t *)abc_slot, sizeof(abc_slot)) != sizeof(abc_slot)) {
        memset(abc_slot, 0, sizeof(abc_slot));
        abc_cur_hour = 0;
        abc_cur_max = 0;
        f.close();
        return;
    }
    f.close();
    abcRefreshClean();
    debug_outln_info(F("BME680 VOC ABC loaded"),
        String(F("Rclean=")) + String((uint32_t)(abc_r_clean + 0.5f)));
}

void BME680Sensor::persistVocAbc() {
    if (!abc_dirty) {
        return;
    }
    if (!SPIFFS.begin(true)) {
        return;
    }
    File f = SPIFFS.open(F("/voc_abc.bin"), "w");
    if (!f) {
        debug_outln_error(F("BME680 VOC ABC save failed"));
        return;
    }
    const uint32_t magic = kAbcMagic;
    f.write((const uint8_t *)&magic, 4);
    f.write((const uint8_t *)&abc_cur_hour, 4);
    f.write((const uint8_t *)&abc_cur_max, 4);
    f.write((const uint8_t *)abc_slot, sizeof(abc_slot));
    f.close();
    abc_dirty = false;
}

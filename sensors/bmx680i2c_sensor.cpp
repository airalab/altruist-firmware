#include "bmx680i2c_sensor.h"
#include "../intl.h"
#include "../config_manager/config_helpers.h"
#include "sensor_names.h"
#include "../utils.h"

#include <time.h>
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
        debug_outln_error(F("BME680 I2C bus lock failed"));
        return false;
    }

    for (uint8_t addr : {0x77, 0x76}) {
        auto test_bme680 = new Adafruit_BME680(I2C_NUM_0, addr);
        if (test_bme680->begin()) {
            // VOC probe: heater 320 °C / 150 ms (datasheet / Adafruit default).
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
        last_fetch_time = millis() - timeout;
        return true;
    }

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

    I2cBusLock bus;
    if (!bus.ok()) {
        debug_outln_error(F("BME680 I2C bus lock failed in fetch"));
        return;
    }

    if (!bme680->performReading()) {
        debug_outln_error(F("BME680 reading failed"));
        return;
    }

    // ---------------- СЫРЫЕ ЗНАЧЕНИЯ ----------------
    float raw_temp     = bme680->temperature;
    float raw_humidity = bme680->humidity;
    float pressure     = bme680->pressure;
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
    last_gas_resistance_value = bme680->gas_resistance;
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
    addValueToJSON(data, F("gas_resistance"), last_gas_resistance_value, INTL_GAS_RESISTANCE, F("Ohm"));
    addValueToJSON(data, F("gas_baseline"), (uint32_t)(gas_baseline_ohm + 0.5f), INTL_GAS_BASELINE, F("Ohm"));
    addValueToJSON(data, F("voc_spikes_today"), voc_spikes_today, INTL_VOC_SPIKES_TODAY, F(""));

#if defined(ALTRUIST_BUILD_DEBUG)
    serializeJson(data, Serial);
#endif
}

void BME680Sensor::updateVocSpikeDetector() {
    // Qualitative VOC (#172): a spike is a sudden drop vs the device's own baseline, not ppm.
    // Ignore drops while the package is cooling — MOX resistance follows T/H and that
    // looked like a spike after the 60 s heater tests (no odor).
    // Count is local calendar day (until 00:00), persisted on SPIFFS.
    static constexpr uint8_t kWarmupSamples = 4;
    static constexpr float kSpikeRatio = 0.92f;
    static constexpr float kSuddenRatio = 0.92f;
    static constexpr float kRecoverRatio = 0.96f;
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
        debug_outln_info(F("BME680 gas baseline warmup"),
            String((int)gas_baseline_samples) + F("/") + String((int)kWarmupSamples) +
            F(" R=") + String(r) + F(" baseline=") + String((uint32_t)(gas_baseline_ohm + 0.5f)));
        persist_if_changed();
        return;
    }

    if (cooling && voc_event_open) {
        voc_event_open = false;
        debug_outln_info(F("BME680 VOC spike ignored (cooling)"),
            String(F("R=")) + String(r) + F(" dT=") + String(prev_temp_c - t, 2));
    }

    if (!voc_event_open && sudden && !cooling && rf < gas_baseline_ohm * kSpikeRatio) {
        voc_event_open = true;
        if (voc_spikes_today < 65535) {
            voc_spikes_today++;
        }
        debug_outln_info(F("BME680 VOC spike"),
            String(F("today=")) + String(voc_spikes_today) +
            F(" R=") + String(r) +
            F(" baseline=") + String((uint32_t)(gas_baseline_ohm + 0.5f)));
    } else if (voc_event_open && rf >= gas_baseline_ohm * kRecoverRatio) {
        voc_event_open = false;
        debug_outln_info(F("BME680 VOC spike recovered"), String(r));
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
        debug_outln_error(F("BME680 VOC day save failed"));
        return;
    }
    serializeJson(doc, f);
    f.close();
}

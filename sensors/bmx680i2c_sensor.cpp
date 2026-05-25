#include "bmx680i2c_sensor.h"
#include "../intl.h"
#include "../config_manager/config_helpers.h"
#include "sensor_names.h"
#include "../utils.h"

#define BME680_SENSOR_MIN_TIMEOUT 300000

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
    i2c_master_init();

    for (uint8_t addr : {0x77, 0x76}) {
        auto test_bme680 = new Adafruit_BME680(I2C_NUM_0, addr);
        if (test_bme680->begin()) {
            // We currently use BME680 only for temperature / humidity / pressure.
            // Disable gas heater to reduce power use and self-heating unless gas measurements are explicitly needed.
            test_bme680->setGasHeater(0, 0);
            bme680 = test_bme680;
            sensor_address = addr;
            break;
        }
        delete test_bme680; // не наш адрес
    }

    deinit_i2c();

    if (bme680) {
        debug_outln_info(F("BME680 Sensor started at address: 0x"), String(sensor_address, HEX));
        debug_outln_info(F("Fetch interval (sec): "), String(timeout / 1000));
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

    i2c_master_init();

    if (!bme680->performReading()) {
        debug_outln_error(F("BME680 reading failed"));
        deinit_i2c();
        return;
    }

    // ---------------- СЫРЫЕ ЗНАЧЕНИЯ ----------------
    float raw_temp     = bme680->temperature;
    float raw_humidity = bme680->humidity;
    float pressure     = bme680->pressure;

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

    // ---------------- JSON ----------------
    addValueToJSON(data, F("temperature"), last_temperature_value, INTL_TEMPERATURE, F("°C"));
    addValueToJSON(data, F("pressure"),    last_pressure_value,    INTL_PRESSURE,    F("Pa"));
    addValueToJSON(data, F("humidity"),    last_humidity_value,    INTL_HUMIDITY,    F("%"));

#ifdef DEV
    serializeJson(data, Serial);
#endif

    deinit_i2c();
}

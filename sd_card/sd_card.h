#ifndef __SD_CARD_H__
#define __SD_CARD_H__

#if defined(USE_SD_CARD)

#include <ArduinoJson.h>
#include <map>
#include <vector>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "../utils.h"

#define ROOT_FOLDER "/sensors_data/"

struct LineData {
    float* values;
    uint32_t* timestamps;
    int count;
};

class SDCard {

public:
  
    bool begin();
    void refreshCache();  // Прочитать все папки и последние файлы
    String getLastFileForSensor(const String& sensorName);
    std::vector<String> getSensorList();
    inline void logData(const String& sensorName, const JsonDocument &data) {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            debug_outln_info(F("[SDCardLogger] Failed to get time"));
            return;
        }
        time_t timestamp = mktime(&timeinfo);

        bool foundAny = false;

        // Перебираем все ключи, ищем те, что начинаются с sensorName
        for (JsonPairConst kv : data.as<JsonObjectConst>()) {
            String key = kv.key().c_str();

            if (!key.startsWith(sensorName)) {
                continue;
            }

            JsonVariantConst sensorData = kv.value();
            if (!sensorData.is<JsonObjectConst>()) {
                debug_outln_info(F("[SDCardLogger] sensorData is not an object for key: "), key);
                continue;
            }

            // Начинаем формировать строки для CSV
            String header = "timestamp";
            String values = String(timestamp);

            JsonObjectConst measurements = sensorData.as<JsonObjectConst>();
            for (JsonPairConst measurement : measurements) {
                header += "," + String(measurement.key().c_str());
                values += "," + String(measurement.value()["value"].as<float>(), 2);
            }

            // Логируем по каждому найденному ключу
            _logCSVRow(key, header, values);
            foundAny = true;
        }

        if (!foundAny) {
            debug_outln_info(F("[SDCardLogger] No matching keys found for sensorName: "), sensorName);
        }
    }
    bool checkInserted();

private:
    std::map<String, String> _sensorLastFiles;
    std::vector<String> _sensorList;
    float cardSizeMB = 0;
    float usedMemMB = 0;
    bool _beginSD(SPIClass &spi);
    String _findLastFileInFolder(const String& path);
    String _getCurrentDateFileName();
    void _logCSVRow(const String& sensorName, const String& header, const String& values);
    String _getCardTypeName(sdcard_type_t type);

};

void readSensorDataFromCSV(LineData &result, const char* sensor_name, const char* field_name, int hours_back);

#endif
#endif // __SD_CARD_H__
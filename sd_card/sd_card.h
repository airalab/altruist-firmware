#ifndef __SD_CARD_H__
#define __SD_CARD_H__

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

        String header = "timestamp";
        String values = String(timestamp);

        // Извлекаем данные из JSON
        if (!data.containsKey(sensorName)) {
            debug_outln_info(F("[SDCardLogger] sensorName not found in JSON"));
            return;
        }

        JsonVariantConst sensorData = data[sensorName];
        if (!sensorData.is<JsonObjectConst>()) {
            debug_outln_info(F("[SDCardLogger] sensorData is not an object"));
            return;
        }

        JsonObjectConst measurements = sensorData.as<JsonObjectConst>();
        for (JsonPairConst kv : measurements) {
            header += "," + String(kv.key().c_str());
            values += "," + String(kv.value()["value"].as<float>(), 2);
        }

        // Передаём готовые строки в функцию, реализованную в .cpp
        _logCSVRow(sensorName, header, values);
    }
    bool checkInserted();

private:
    std::map<String, String> _sensorLastFiles;
    std::vector<String> _sensorList;
    uint64_t cardSizeMB = 0;
    uint64_t usedMemMB = 0;
    bool _beginSD(SPIClass &spi);
    String _findLastFileInFolder(const String& path);
    String _getCurrentDateFileName();
    void _logCSVRow(const String& sensorName, const String& header, const String& values);
    String _getCardTypeName(sdcard_type_t type);

};

#endif // __SD_CARD_H__

void readSensorDataFromCSV(LineData &result, const char* sensor_name, const char* field_name, int hours_back);
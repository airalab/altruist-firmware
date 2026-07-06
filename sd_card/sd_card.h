#ifndef __SD_CARD_H__
#define __SD_CARD_H__

#if defined(USE_SD_CARD)

#include <ArduinoJson.h>
#include <map>
#include <vector>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <freertos/semphr.h>
#include "../utils.h"

#define ROOT_FOLDER "/sensors_data/"
#define EXCEPTIONS_FOLDER "/exceptions"
#define ROLLUP_ROOT_FOLDER "/sensors_rollup"
#define ROLLUP_DAILY_FOLDER "/sensors_rollup/daily"
#define ROLLUP_HOURLY_FOLDER "/sensors_rollup/hourly"
#define ROLLUP_MONTHLY_FOLDER "/sensors_rollup/monthly"

struct LineData {
    float* values;
    uint32_t* timestamps;
    int count;
};

struct PeriodStats {
    float min_v = 0.0f;
    float max_v = 0.0f;
    float avg_v = 0.0f;
    uint32_t count = 0;
    bool has_data = false;
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
    // Build daily/monthly rollups used by analytics period views.
    bool buildDailyRollupsIfNeeded();
    bool buildMonthlyRollupsIfNeeded();
    bool readPeriodStats(const char* sensor_name, const char* field_name, uint16_t period_days, float scale, PeriodStats &out_stats);

    // Retention cleanup:
    // - raw sensor CSV retention (days)
    // - daily rollup retention (days)
    // - hourly rollup retention (days)
    // - monthly rollup retention (months)
    // - keep only the newest N boot diagnostic files in /exceptions
    bool applyRetentionPolicy(uint16_t rawSensorRetentionDays,
                              uint16_t dailyRollupRetentionDays,
                              uint16_t hourlyRollupRetentionDays,
                              uint16_t monthlyRollupRetentionMonths,
                              uint16_t maxBootFiles);

    // Generic text file helpers used by exception/runtime logging.
    // These make sure parent folders exist and then write/append content.
    bool writeTextFile(const String& fullPath, const String& content);
    bool appendTextFile(const String& fullPath, const String& content);

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

/** Returns false when the SD mutex could not be acquired (result cleared). */
bool readSensorDataFromCSV(LineData &result, const char* sensor_name, const char* field_name, int hours_back, uint32_t lock_timeout_ms = 5000);
// Global SD lock helpers for modules that still use direct SD.* access.
bool sdCardLock(uint32_t timeout_ms = 5000);
/** Acquire SD lock in short slices; yields to HTTP/display while waiting (Insight). */
bool sdCardLockWithYield(uint32_t total_timeout_ms);
void sdCardUnlock();
// DEV diagnostics counters for SD activity.
void sdGetDevCounters(uint32_t &csv_ok, uint32_t &csv_fail, uint32_t &lock_busy);

#endif
#endif // __SD_CARD_H__
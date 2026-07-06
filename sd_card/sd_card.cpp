#if defined(USE_SD_CARD)

#include "sd_card.h"
#include "../defines.h"
#include "../utils.h"
#include <algorithm>

bool sdCardLockWithYield(uint32_t total_timeout_ms);

namespace {
SemaphoreHandle_t g_sd_mutex = nullptr;
volatile uint32_t g_sd_csv_write_ok = 0;
volatile uint32_t g_sd_csv_write_fail = 0;
volatile uint32_t g_sd_lock_busy = 0;

static void ensureSDMutex() {
    if (g_sd_mutex == nullptr) {
        g_sd_mutex = xSemaphoreCreateRecursiveMutex();
    }
}

class SDLockGuard {
public:
    explicit SDLockGuard(uint32_t timeout_ms = 5000, bool yield_while_waiting = false) : locked(false) {
        ensureSDMutex();
        if (!g_sd_mutex) return;
        if (yield_while_waiting) {
            locked = ::sdCardLockWithYield(timeout_ms);
        } else {
            locked = (xSemaphoreTakeRecursive(g_sd_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE);
        }
    }
    ~SDLockGuard() {
        if (locked && g_sd_mutex) {
            xSemaphoreGiveRecursive(g_sd_mutex);
        }
    }
    bool ok() const { return locked; }
private:
    bool locked;
};
} // namespace

bool sdCardLock(uint32_t timeout_ms) {
    ensureSDMutex();
    if (!g_sd_mutex) return false;
    return xSemaphoreTakeRecursive(g_sd_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

bool sdCardLockWithYield(uint32_t total_timeout_ms) {
    ensureSDMutex();
    if (!g_sd_mutex) return false;
    const uint32_t start_ms = millis();
    constexpr uint32_t kSliceMs = 50;
    while (msSince(start_ms) < total_timeout_ms) {
        const uint32_t elapsed = millis() - start_ms;
        const uint32_t remain = total_timeout_ms - elapsed;
        const uint32_t wait_ms = (remain < kSliceMs) ? remain : kSliceMs;
        if (xSemaphoreTakeRecursive(g_sd_mutex, pdMS_TO_TICKS(wait_ms)) == pdTRUE) {
            return true;
        }
#if defined(ALTRUIST_INSIGHT)
        firmwareBlockingYieldHook();
#else
        yield();
#endif
    }
    return false;
}

void sdCardUnlock() {
    if (g_sd_mutex) {
        xSemaphoreGiveRecursive(g_sd_mutex);
    }
}

void sdGetDevCounters(uint32_t &csv_ok, uint32_t &csv_fail, uint32_t &lock_busy) {
    csv_ok = g_sd_csv_write_ok;
    csv_fail = g_sd_csv_write_fail;
    lock_busy = g_sd_lock_busy;
}

bool SDCard::begin() {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in begin()"));
        g_sd_lock_busy++;
        return false;
    }
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CS_PIN);
    if (_beginSD(SPI)) {
        refreshCache();
        return true;
    } else {
        return false;
    }
}

bool SDCard::_beginSD(SPIClass &spi) {
    if (!SD.begin(SPI_CS_PIN, spi, 40000000)) {
        debug_outln_info(F("Card Mount Failed"));
        return false;
    }
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE) {
        debug_outln_info(F("No SD card attached"));
        return false;
    }

    cardSizeMB = (float)SD.cardSize() / (1024 * 1024);
    usedMemMB = (float)SD.usedBytes() / (1024 * 1024);
    debug_outln_verbose(F("SD Card Size (MB): "), String(cardSizeMB));
    debug_outln_verbose(F("Used space (MB): "), String(usedMemMB));
    return true;
}

bool SDCard::writeTextFile(const String& fullPath, const String& content) {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in writeTextFile()"));
        g_sd_lock_busy++;
        return false;
    }
    int slash = fullPath.lastIndexOf('/');
    if (slash > 0) {
        String folder = fullPath.substring(0, slash);
        if (!SD.exists(folder)) {
            if (!SD.mkdir(folder)) {
                debug_outln_info(F("[SDCardLogger] Failed to create folder for text file: "), folder);
                return false;
            }
        }
    }

    File file = SD.open(fullPath, FILE_WRITE);
    if (!file) {
        debug_outln_info(F("[SDCardLogger] Failed to open text file for write: "), fullPath);
        return false;
    }
    size_t written = file.print(content);
    file.close();
    if (written != content.length()) {
        debug_outln_info(F("[SDCardLogger] Short write when writing text file: "), fullPath);
        return false;
    }
    return true;
}

bool SDCard::appendTextFile(const String& fullPath, const String& content) {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in appendTextFile()"));
        g_sd_lock_busy++;
        return false;
    }
    int slash = fullPath.lastIndexOf('/');
    if (slash > 0) {
        String folder = fullPath.substring(0, slash);
        if (!SD.exists(folder)) {
            if (!SD.mkdir(folder)) {
                debug_outln_info(F("[SDCardLogger] Failed to create folder for append: "), folder);
                return false;
            }
        }
    }

    File file = SD.open(fullPath, FILE_APPEND);
    if (!file) {
        debug_outln_info(F("[SDCardLogger] Failed to open text file for append: "), fullPath);
        return false;
    }
    size_t written = file.print(content);
    file.close();
    if (written != content.length()) {
        debug_outln_info(F("[SDCardLogger] Short write when appending text file: "), fullPath);
        return false;
    }
    return true;
}

void SDCard::refreshCache() {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in refreshCache()"));
        g_sd_lock_busy++;
        return;
    }
    _sensorList.clear();
    _sensorLastFiles.clear();

    if (!SD.exists(ROOT_FOLDER)) {
        if (SD.mkdir(ROOT_FOLDER)) {
            debug_outln_verbose(F("[SDCardLogger] Root folder created"));
        } else {
            debug_outln_info("[SDCardLogger] mkdir failed");
        }
    }

    File root = SD.open(ROOT_FOLDER);
    if (!root || !root.isDirectory()) {
        debug_outln_info(F("[SDCardLogger] Root dir error"));
        return;
    }

    File entry = root.openNextFile();
    while (entry) {
        if (entry.isDirectory()) {
            String sensorName = entry.name();
            int slash = sensorName.lastIndexOf('/');
            if (slash >= 0) sensorName = sensorName.substring(slash + 1);
            _sensorList.push_back(sensorName);

            String path = ROOT_FOLDER + sensorName;
            String lastFile = _findLastFileInFolder(path);
            _sensorLastFiles[sensorName] = lastFile;
        }
        entry = root.openNextFile();
    }
    root.close();
}

std::vector<String> SDCard::getSensorList() {
    return _sensorList;
}

String SDCard::getLastFileForSensor(const String& sensorName) {
    if (_sensorLastFiles.count(sensorName)) {
        return _sensorLastFiles[sensorName];
    }
    return "";
}

String SDCard::_getCurrentDateFileName() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        debug_outln_info(F("[SDCardLogger] Failed to get time"));
        return "";
    }

    char filename[20];
    strftime(filename, sizeof(filename), "%Y-%m-%d.csv", &timeinfo);
    return String(filename);
}

String SDCard::_findLastFileInFolder(const String& path) {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in _findLastFileInFolder()"));
        g_sd_lock_busy++;
        return "";
    }
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) return "";

    String maxFile = "";
    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String name = file.name();
            int slash = name.lastIndexOf('/');
            if (slash >= 0) name = name.substring(slash + 1);

            if (name > maxFile) {
                maxFile = name;
            }
        }
        file = dir.openNextFile();
    }
    dir.close();

    return maxFile;
}


void SDCard::_logCSVRow(const String& sensorName, const String& header, const String& values) {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in _logCSVRow()"));
        g_sd_lock_busy++;
        g_sd_csv_write_fail++;
        return;
    }
    String folder = ROOT_FOLDER + sensorName;
    if (!SD.exists(folder)) {
        if (SD.mkdir(folder)) {
            _sensorList.push_back(sensorName);
            debug_outln_verbose(F("[SDCardLogger] folder created: "), folder);
        } else {
            debug_outln_info(F("[SDCardLogger] folder not created: "), folder);
            return;
        }
    }

    String filename = _getCurrentDateFileName();
    String fullPath = folder + "/" + filename;

    bool fileExists = SD.exists(fullPath);
    File file = SD.open(fullPath, FILE_APPEND);
    if (!file) {
        debug_outln_info(F("[SDCardLogger] Failed to open file: "), fullPath);
        g_sd_csv_write_fail++;
        return;
    }

    debug_outln_verbose(F("[SDCardLogger] Start writing data to file "), fullPath);

    _sensorLastFiles[sensorName] = filename;

    // Если файл новый — пишем заголовок
    if (!fileExists || file.size() == 0) {
        if (file.println(header)) {
            debug_outln_verbose(F("[SDCardLogger] Header writed: "), header);
        } else {
            debug_outln_info(F("Can't write header "));
        }
    }

    if (file.println(values)) {
        debug_outln_verbose(F("[SDCardLogger] Logged to: "), fullPath);
        debug_outln_verbose(F("[SDCardLogger] Data: "), values);
        g_sd_csv_write_ok++;
    } else {
        debug_outln_info(F("[SDCardLogger] Can't write data: "));
        incrementSDWriteError();
        g_sd_csv_write_fail++;
    }
    file.close();
}


bool readSensorDataFromCSV(LineData &result, const char* sensor_name, const char* field_name, int hours_back, uint32_t lock_timeout_ms) {
    SDLockGuard lock(lock_timeout_ms, true);
    if (!lock.ok()) {
        g_sd_lock_busy++;
        static unsigned long last_lock_timeout_log_ms = 0;
        if (msSince(last_lock_timeout_log_ms) > 3000UL) {
            last_lock_timeout_log_ms = millis();
            debug_outln_info(F("[SD] CSV read lock timeout ms"), String(lock_timeout_ms));
        }
        result.count = 0;
        result.values = nullptr;
        result.timestamps = nullptr;
        return false;
    }

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    // Временная память для хранения считанных значений (общая для обоих дней)
    std::vector<float> values_vec;
    std::vector<uint32_t> timestamps_vec;
    
    // Граница по времени
    uint32_t time_limit = now - (hours_back * 3600);
    
    int field_index = -1;
    bool field_index_found = false;
    
    // Helper function to read from a specific date file
    auto readFromDateFile = [&](int year, int month, int day) -> bool {
        char filename[64];
        snprintf(filename, sizeof(filename), "/sensors_data/%s/%04d-%02d-%02d.csv",
                 sensor_name, year, month, day);

        File file = SD.open(filename, FILE_READ);
        if (!file) {
            return false; // File doesn't exist, that's OK
        }

        const size_t file_size = file.size();
        
        // Читаем заголовок (только если еще не нашли field_index)
        if (!field_index_found) {
            String header_line = file.readStringUntil('\n');
            std::vector<String> headers;
            int start = 0;
            while (start < header_line.length()) {
                int comma = header_line.indexOf(',', start);
                if (comma == -1) comma = header_line.length();
                headers.push_back(header_line.substring(start, comma));
                start = comma + 1;
            }
            
            for (size_t i = 0; i < headers.size(); ++i) {
                String header = headers[i];
                header.trim();
                if (header == field_name) {
                    field_index = i;
                    field_index_found = true;
                    break;
                }
            }
            
            if (!field_index_found) {
                file.close();
                return false;
            }
        } else {
            // Skip header line if we already found field_index
            file.readStringUntil('\n');
        }

        // Append-only daily logs: for short windows seek near EOF instead of scanning from midnight.
        if (hours_back <= 24 && file_size > 8192) {
            // ~30s sampling, ~48 bytes/line; cover full window + margin.
            const size_t rows_needed = (size_t)hours_back * 120u + 256u;
            size_t tail_bytes = rows_needed * 48u;
            if (tail_bytes < 65536u) {
                tail_bytes = 65536u;
            }
            if (tail_bytes > file_size) {
                tail_bytes = file_size;
            }
            const size_t seek_pos = file_size - tail_bytes;
            if (seek_pos > 0) {
                file.seek(seek_pos);
                file.readStringUntil('\n'); // discard partial line
            }
        }

        // Чтение данных построчно
        uint32_t line_idx = 0;
        while (file.available()) {
            if ((line_idx & 63u) == 0) {
#if defined(ALTRUIST_INSIGHT)
                firmwareBlockingYieldHook();
#else
                yield();
#endif
            }
            line_idx++;
            String line = file.readStringUntil('\n');
            if (line.length() == 0) continue;
            std::vector<String> parts;
            int pos = 0;
            while (pos < line.length()) {
                int comma = line.indexOf(',', pos);
                if (comma == -1) comma = line.length();
                parts.push_back(line.substring(pos, comma));
                pos = comma + 1;
            }

            if (parts.size() <= (size_t)field_index) continue;

            uint32_t timestamp = parts[0].toInt();
            if (timestamp < time_limit) {
                continue;
            }
            
            float value;
            if (strstr(field_name, "pressure") != nullptr) {
                value = parts[field_index].toFloat() * 0.0075;
            } else {
                value = parts[field_index].toFloat();
            }

            timestamps_vec.push_back(timestamp);
            values_vec.push_back(value);
        }

        file.close();
        return true;
    };
    

    int days_to_scan = (hours_back / 24) + 2; // +2 for boundary overlap across midnight
    if (days_to_scan < 2) days_to_scan = 2;
    for (int day_shift = 0; day_shift < days_to_scan; day_shift++) {
        struct tm day_tm = *timeinfo;
        day_tm.tm_mday -= day_shift;
        mktime(&day_tm); // normalize date
        readFromDateFile(day_tm.tm_year + 1900, day_tm.tm_mon + 1, day_tm.tm_mday);
    }

    // If still no data, return early
    if (values_vec.empty()) {
        result.count = 0;
        result.values = nullptr;
        result.timestamps = nullptr;
        return true;
    }
    
    // Sort by timestamp (in case we read from two files)
    std::vector<std::pair<uint32_t, float>> combined;
    for (size_t i = 0; i < timestamps_vec.size(); i++) {
        combined.push_back({timestamps_vec[i], values_vec[i]});
    }
    std::sort(combined.begin(), combined.end());
    
    // Переносим в C-массивы
    result.count = combined.size();
    result.values = new float[result.count];
    result.timestamps = new uint32_t[result.count];

    for (int i = 0; i < result.count; ++i) {
        result.timestamps[i] = combined[i].first;
        result.values[i] = combined[i].second;
    }
    
    debug_outln_verbose(String(F("[SDCard] Read total of ")) + String(result.count) + F(" data points from CSV"));
    return true;
}

bool SDCard::checkInserted() {
    SDLockGuard lock(250);
    if (!lock.ok()) {
        g_sd_lock_busy++;
        // SD can be busy in another task (graph read/log rotation/retention).
        // Don't treat temporary lock contention as card removal.
        static unsigned long last_busy_log_ms = 0;
        if (msSince(last_busy_log_ms) > 30000UL) {
            last_busy_log_ms = millis();
            debug_outln_verbose(F("[SDCardLogger] SD mutex busy in checkInserted(); will retry later"));
        }
        return true;
    }
    static sdcard_type_t last_type = CARD_NONE;

    sdcard_type_t card_type = SD.cardType();
    // Only log when the type changes, or when we hit an error below.
    if (card_type != last_type) {
        last_type = card_type;
        debug_outln_verbose(F("[SDCardLogger] Card type: "), _getCardTypeName(card_type));
    }
    
    if (card_type == CARD_NONE) {
        debug_outln_info("[SDCardLogger] No SD card present, retry begin...");
        return _beginSD(SPI);
    }
    
    // Try to actually access the card to verify it's really there
    // cardType() might return cached value, so we need to test actual access
    if (!SD.exists("/")) {
        debug_outln_info(F("[SDCardLogger] Card type OK but filesystem not accessible - card may be removed"));
        // Try to reinitialize
        SD.end();
        delay(50);
        return _beginSD(SPI);
    }
    
    // Try to open root directory to verify card is actually accessible
    File root = SD.open("/");
    if (!root) {
        debug_outln_info(F("[SDCardLogger] Cannot open root directory - card may be removed"));
        SD.end();
        delay(50);
        return _beginSD(SPI);
    }
    root.close();
    
    return true;
}

bool SDCard::buildDailyRollupsIfNeeded() {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in buildDailyRollupsIfNeeded()"));
        g_sd_lock_busy++;
        return false;
    }
    if (!checkInserted()) {
        return false;
    }

    if (!SD.exists(ROLLUP_ROOT_FOLDER)) SD.mkdir(ROLLUP_ROOT_FOLDER);
    if (!SD.exists(ROLLUP_DAILY_FOLDER)) SD.mkdir(ROLLUP_DAILY_FOLDER);

    File root = SD.open(ROOT_FOLDER);
    if (!root || !root.isDirectory()) {
        return false;
    }

    bool changed = false;
    File sensorDir = root.openNextFile();
    while (sensorDir) {
        if (sensorDir.isDirectory()) {
            String sensorName = sensorDir.name();
            int slash = sensorName.lastIndexOf('/');
            if (slash >= 0) sensorName = sensorName.substring(slash + 1);
            String sensorDirPath = sensorDir.name();
            if (!sensorDirPath.startsWith("/")) {
                sensorDirPath = String(ROOT_FOLDER) + sensorDirPath;
            }

            String sensorDailyDir = String(ROLLUP_DAILY_FOLDER) + "/" + sensorName;
            if (!SD.exists(sensorDailyDir)) {
                SD.mkdir(sensorDailyDir);
            }

            File rawFile = sensorDir.openNextFile();
            while (rawFile) {
                if (!rawFile.isDirectory()) {
                    String rawPath = rawFile.name();
                    if (!rawPath.startsWith("/")) {
                        if (!sensorDirPath.endsWith("/")) sensorDirPath += "/";
                        rawPath = sensorDirPath + rawPath;
                    }
                    int fslash = rawPath.lastIndexOf('/');
                    String fileName = (fslash >= 0) ? rawPath.substring(fslash + 1) : rawPath;
                    if (fileName.length() == 14 && fileName.endsWith(".csv")) {
                        String outPath = sensorDailyDir + "/" + fileName;
                        if (!SD.exists(outPath)) {
                            File in = SD.open(rawPath, FILE_READ);
                            if (in) {
                                String header = in.readStringUntil('\n');
                                std::vector<String> cols;
                                int start = 0;
                                while (start < header.length()) {
                                    int comma = header.indexOf(',', start);
                                    if (comma == -1) comma = header.length();
                                    String h = header.substring(start, comma);
                                    h.trim();
                                    cols.push_back(h);
                                    start = comma + 1;
                                }

                                struct MetricAcc {
                                    float min_v = 0.0f;
                                    float max_v = 0.0f;
                                    float sum_v = 0.0f;
                                    uint32_t count = 0;
                                };
                                std::map<String, MetricAcc> acc;
                                uint32_t ts_min = 0;
                                uint32_t ts_max = 0;

                                while (in.available()) {
                                    String line = in.readStringUntil('\n');
                                    if (line.length() == 0) continue;
                                    std::vector<String> parts;
                                    int p = 0;
                                    while (p < line.length()) {
                                        int comma = line.indexOf(',', p);
                                        if (comma == -1) comma = line.length();
                                        parts.push_back(line.substring(p, comma));
                                        p = comma + 1;
                                    }
                                    if (parts.size() < 2) continue;
                                    uint32_t ts = (uint32_t)parts[0].toInt();
                                    if (ts == 0) continue;
                                    if (ts_min == 0 || ts < ts_min) ts_min = ts;
                                    if (ts > ts_max) ts_max = ts;

                                    size_t limit = (parts.size() < cols.size()) ? parts.size() : cols.size();
                                    for (size_t i = 1; i < limit; i++) {
                                        String metric = cols[i];
                                        if (metric.length() == 0) continue;
                                        float value = parts[i].toFloat();
                                        if (metric.indexOf("pressure") >= 0) {
                                            value *= 0.0075f;
                                        }
                                        auto &m = acc[metric];
                                        if (m.count == 0) {
                                            m.min_v = value;
                                            m.max_v = value;
                                        } else {
                                            if (value < m.min_v) m.min_v = value;
                                            if (value > m.max_v) m.max_v = value;
                                        }
                                        m.sum_v += value;
                                        m.count++;
                                    }
                                }
                                in.close();

                                if (!acc.empty() && ts_min > 0 && ts_max >= ts_min) {
                                    File out = SD.open(outPath, FILE_WRITE);
                                    if (out) {
                                        out.println("period_start_ts,period_end_ts,metric,min,max,avg,count");
                                        for (const auto &kv : acc) {
                                            const String &metric = kv.first;
                                            const MetricAcc &m = kv.second;
                                            if (m.count == 0) continue;
                                            String row = String(ts_min) + "," + String(ts_max) + "," + metric + "," +
                                                         String(m.min_v, 3) + "," + String(m.max_v, 3) + "," +
                                                         String(m.sum_v / (float)m.count, 3) + "," + String(m.count);
                                            out.println(row);
                                        }
                                        out.close();
                                        changed = true;
                                    }
                                }
                            }
                        }
                    }
                }
                rawFile.close();
                rawFile = sensorDir.openNextFile();
            }
        }
        sensorDir.close();
        sensorDir = root.openNextFile();
    }
    root.close();
    return true;
}

bool SDCard::buildMonthlyRollupsIfNeeded() {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in buildMonthlyRollupsIfNeeded()"));
        g_sd_lock_busy++;
        return false;
    }
    if (!checkInserted()) {
        return false;
    }
    if (!SD.exists(ROLLUP_ROOT_FOLDER)) SD.mkdir(ROLLUP_ROOT_FOLDER);
    if (!SD.exists(ROLLUP_MONTHLY_FOLDER)) SD.mkdir(ROLLUP_MONTHLY_FOLDER);
    if (!SD.exists(ROLLUP_DAILY_FOLDER)) return true;

    File dailyRoot = SD.open(ROLLUP_DAILY_FOLDER);
    if (!dailyRoot || !dailyRoot.isDirectory()) {
        return false;
    }

    File sensorDir = dailyRoot.openNextFile();
    while (sensorDir) {
        if (sensorDir.isDirectory()) {
            String sensorName = sensorDir.name();
            int slash = sensorName.lastIndexOf('/');
            if (slash >= 0) sensorName = sensorName.substring(slash + 1);
            String sensorDirPath = sensorDir.name();
            if (!sensorDirPath.startsWith("/")) {
                sensorDirPath = String(ROLLUP_DAILY_FOLDER) + "/" + sensorName;
            }
            String sensorMonthlyDir = String(ROLLUP_MONTHLY_FOLDER) + "/" + sensorName;
            if (!SD.exists(sensorMonthlyDir)) SD.mkdir(sensorMonthlyDir);

            struct MetricAcc {
                float min_v = 0.0f;
                float max_v = 0.0f;
                float sum_weighted = 0.0f;
                uint32_t count = 0;
                uint32_t ts_start = 0;
                uint32_t ts_end = 0;
            };
            std::map<String, std::map<String, MetricAcc>> monthMetric; // month -> metric -> acc

            File dayFile = sensorDir.openNextFile();
            while (dayFile) {
                if (!dayFile.isDirectory()) {
                    String dayPath = dayFile.name();
                    if (!dayPath.startsWith("/")) {
                        if (!sensorDirPath.endsWith("/")) sensorDirPath += "/";
                        dayPath = sensorDirPath + dayPath;
                    }
                    int dslash = dayPath.lastIndexOf('/');
                    String dayName = (dslash >= 0) ? dayPath.substring(dslash + 1) : dayPath;
                    if (dayName.length() == 14 && dayName.endsWith(".csv")) {
                        String monthKey = dayName.substring(0, 7); // YYYY-MM
                        File in = SD.open(dayPath, FILE_READ);
                        if (in) {
                            in.readStringUntil('\n'); // header
                            while (in.available()) {
                                String line = in.readStringUntil('\n');
                                if (line.length() == 0) continue;
                                std::vector<String> parts;
                                int p = 0;
                                while (p < line.length()) {
                                    int comma = line.indexOf(',', p);
                                    if (comma == -1) comma = line.length();
                                    parts.push_back(line.substring(p, comma));
                                    p = comma + 1;
                                }
                                if (parts.size() < 7) continue;
                                uint32_t ts_start = (uint32_t)parts[0].toInt();
                                uint32_t ts_end = (uint32_t)parts[1].toInt();
                                String metric = parts[2];
                                float min_v = parts[3].toFloat();
                                float max_v = parts[4].toFloat();
                                float avg_v = parts[5].toFloat();
                                uint32_t count = (uint32_t)parts[6].toInt();
                                if (count == 0 || metric.length() == 0) continue;

                                MetricAcc &m = monthMetric[monthKey][metric];
                                if (m.count == 0) {
                                    m.min_v = min_v;
                                    m.max_v = max_v;
                                    m.ts_start = ts_start;
                                    m.ts_end = ts_end;
                                } else {
                                    if (min_v < m.min_v) m.min_v = min_v;
                                    if (max_v > m.max_v) m.max_v = max_v;
                                    if (ts_start < m.ts_start) m.ts_start = ts_start;
                                    if (ts_end > m.ts_end) m.ts_end = ts_end;
                                }
                                m.sum_weighted += avg_v * (float)count;
                                m.count += count;
                            }
                            in.close();
                        }
                    }
                }
                dayFile.close();
                dayFile = sensorDir.openNextFile();
            }

            for (const auto &mkv : monthMetric) {
                const String &monthKey = mkv.first;
                String outPath = sensorMonthlyDir + "/" + monthKey + ".csv";
                File out = SD.open(outPath, FILE_WRITE);
                if (!out) continue;
                out.println("period_start_ts,period_end_ts,metric,min,max,avg,count");
                for (const auto &kv : mkv.second) {
                    const String &metric = kv.first;
                    const MetricAcc &m = kv.second;
                    if (m.count == 0) continue;
                    String row = String(m.ts_start) + "," + String(m.ts_end) + "," + metric + "," +
                                 String(m.min_v, 3) + "," + String(m.max_v, 3) + "," +
                                 String(m.sum_weighted / (float)m.count, 3) + "," + String(m.count);
                    out.println(row);
                }
                out.close();
            }
        }
        sensorDir.close();
        sensorDir = dailyRoot.openNextFile();
    }
    dailyRoot.close();
    return true;
}

bool SDCard::readPeriodStats(const char* sensor_name, const char* field_name, uint16_t period_days, float scale, PeriodStats &out_stats) {
    out_stats = PeriodStats{};
    // Keep analytics reads responsive; fail fast if SD is busy.
    SDLockGuard lock(300);
    if (!lock.ok()) {
        g_sd_lock_busy++;
        return false;
    }
    if (!checkInserted()) return false;

    auto finalize = [&](float min_v, float max_v, float sum_weighted, uint32_t count) -> bool {
        if (count == 0) return false;
        out_stats.min_v = min_v;
        out_stats.max_v = max_v;
        out_stats.avg_v = sum_weighted / (float)count;
        out_stats.count = count;
        out_stats.has_data = true;
        return true;
    };

    // 24h path: prefer raw data for highest fidelity.
    if (period_days <= 1) {
        LineData data{nullptr, nullptr, 0};
        readSensorDataFromCSV(data, sensor_name, field_name, 24);
        if (data.count > 0 && data.values) {
            float min_v = data.values[0] * scale;
            float max_v = min_v;
            float sum_v = 0.0f;
            for (int i = 0; i < data.count; i++) {
                float v = data.values[i] * scale;
                if (v < min_v) min_v = v;
                if (v > max_v) max_v = v;
                sum_v += v;
            }
            if (data.values) delete[] data.values;
            if (data.timestamps) delete[] data.timestamps;
            return finalize(min_v, max_v, sum_v, (uint32_t)data.count);
        }
        if (data.values) delete[] data.values;
        if (data.timestamps) delete[] data.timestamps;
        return false;
    }

    // 7d/30d path: use daily rollups first.
    time_t now = time(nullptr);
    if (now <= 0) {
        return false;
    }
    time_t window_start = now - (time_t)period_days * 24 * 60 * 60;

    bool has_any = false;
    float min_v = 0.0f;
    float max_v = 0.0f;
    float sum_weighted = 0.0f;
    uint32_t total_count = 0;

    for (time_t t = window_start; t <= now; t += 24 * 60 * 60) {
        struct tm tm_day;
        localtime_r(&t, &tm_day);
        char dayName[16];
        strftime(dayName, sizeof(dayName), "%Y-%m-%d.csv", &tm_day);
        String rollupPath = String(ROLLUP_DAILY_FOLDER) + "/" + sensor_name + "/" + dayName;
        File in = SD.open(rollupPath, FILE_READ);
        if (!in) continue;
        in.readStringUntil('\n'); // header
        while (in.available()) {
            String line = in.readStringUntil('\n');
            if (line.length() == 0) continue;
            std::vector<String> parts;
            int p = 0;
            while (p < line.length()) {
                int comma = line.indexOf(',', p);
                if (comma == -1) comma = line.length();
                parts.push_back(line.substring(p, comma));
                p = comma + 1;
            }
            if (parts.size() < 7) continue;
            if (parts[2] != String(field_name)) continue;
            float lmin = parts[3].toFloat() * scale;
            float lmax = parts[4].toFloat() * scale;
            float lavg = parts[5].toFloat() * scale;
            uint32_t lcount = (uint32_t)parts[6].toInt();
            if (lcount == 0) continue;
            if (!has_any) {
                min_v = lmin;
                max_v = lmax;
                has_any = true;
            } else {
                if (lmin < min_v) min_v = lmin;
                if (lmax > max_v) max_v = lmax;
            }
            sum_weighted += lavg * (float)lcount;
            total_count += lcount;
        }
        in.close();
    }

    if (has_any && total_count > 0) {
        return finalize(min_v, max_v, sum_weighted, total_count);
    }

    // Fallback to raw data if rollups are missing.
    LineData data{nullptr, nullptr, 0};
    readSensorDataFromCSV(data, sensor_name, field_name, (int)period_days * 24);
    if (data.count <= 0 || !data.values) {
        if (data.values) delete[] data.values;
        if (data.timestamps) delete[] data.timestamps;
        return false;
    }
    min_v = data.values[0] * scale;
    max_v = min_v;
    float sum_v = 0.0f;
    for (int i = 0; i < data.count; i++) {
        float v = data.values[i] * scale;
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
        sum_v += v;
    }
    uint32_t cnt = (uint32_t)data.count;
    if (data.values) delete[] data.values;
    if (data.timestamps) delete[] data.timestamps;
    return finalize(min_v, max_v, sum_v, cnt);
}

bool SDCard::applyRetentionPolicy(uint16_t rawSensorRetentionDays,
                                  uint16_t dailyRollupRetentionDays,
                                  uint16_t hourlyRollupRetentionDays,
                                  uint16_t monthlyRollupRetentionMonths,
                                  uint16_t maxBootFiles) {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in applyRetentionPolicy()"));
        g_sd_lock_busy++;
        return false;
    }
    if (!checkInserted()) {
        return false;
    }

    bool changed = false;
    auto toAbsolutePath = [](const String& parentDir, const String& pathOrName) -> String {
        String p = pathOrName;
        p.trim();
        if (p.length() == 0) return "";
        if (!p.startsWith("/")) {
            String base = parentDir;
            if (!base.endsWith("/")) base += "/";
            p = base + p;
        }
        return p;
    };

    // 1) sensors_data/raw retention: remove CSV files older than cutoff date.
    // File names are expected as YYYY-MM-DD.csv so lexicographical comparison works.
    if (rawSensorRetentionDays > 0) {
        time_t now = time(nullptr);
        if (now > 0) {
            time_t cutoff = now - (time_t)rawSensorRetentionDays * 24 * 60 * 60;
            struct tm cutoff_tm;
            localtime_r(&cutoff, &cutoff_tm);
            char cutoff_name[16];
            strftime(cutoff_name, sizeof(cutoff_name), "%Y-%m-%d", &cutoff_tm);
            String cutoff_date(cutoff_name);

            File root = SD.open(ROOT_FOLDER);
            if (root && root.isDirectory()) {
                File sensorDir = root.openNextFile();
                while (sensorDir) {
                    if (sensorDir.isDirectory()) {
                        String sensorDirPath = sensorDir.name();
                        if (!sensorDirPath.startsWith("/")) {
                            sensorDirPath = String(ROOT_FOLDER) + sensorDirPath;
                        }
                        File dayFile = sensorDir.openNextFile();
                        while (dayFile) {
                            if (!dayFile.isDirectory()) {
                                String filePath = toAbsolutePath(sensorDirPath, dayFile.name());
                                int slash = filePath.lastIndexOf('/');
                                String name = (slash >= 0) ? filePath.substring(slash + 1) : filePath;
                                if (name.length() == 14 && name.endsWith(".csv")) {
                                    String datePart = name.substring(0, 10); // YYYY-MM-DD
                                    if (datePart < cutoff_date) {
                                        if (filePath.length() > 1 && filePath.startsWith("/")) {
                                            if (SD.remove(filePath)) {
                                                changed = true;
                                                debug_outln_verbose(F("[SDCardLogger] Retention removed old sensor CSV: "), filePath);
                                            }
                                        } else {
                                            debug_outln_info(F("[SDCardLogger] Retention skip invalid sensor CSV path: "), filePath);
                                        }
                                    }
                                }
                            }
                            dayFile.close();
                            dayFile = sensorDir.openNextFile();
                        }
                    }
                    sensorDir.close();
                    sensorDir = root.openNextFile();
                }
                root.close();
            }
        }
    }

    // 2) daily rollup retention by day.
    if (dailyRollupRetentionDays > 0 && SD.exists(ROLLUP_DAILY_FOLDER)) {
        time_t now = time(nullptr);
        if (now > 0) {
            time_t cutoff = now - (time_t)dailyRollupRetentionDays * 24 * 60 * 60;
            struct tm cutoff_tm;
            localtime_r(&cutoff, &cutoff_tm);
            char cutoff_name[16];
            strftime(cutoff_name, sizeof(cutoff_name), "%Y-%m-%d", &cutoff_tm);
            String cutoff_date(cutoff_name);

            File root = SD.open(ROLLUP_DAILY_FOLDER);
            if (root && root.isDirectory()) {
                File sensorDir = root.openNextFile();
                while (sensorDir) {
                    if (sensorDir.isDirectory()) {
                        String sensorDirPath = sensorDir.name();
                        if (!sensorDirPath.startsWith("/")) {
                            sensorDirPath = String(ROLLUP_DAILY_FOLDER) + "/" + sensorDirPath;
                        }
                        File dayFile = sensorDir.openNextFile();
                        while (dayFile) {
                            if (!dayFile.isDirectory()) {
                                String filePath = toAbsolutePath(sensorDirPath, dayFile.name());
                                int slash = filePath.lastIndexOf('/');
                                String name = (slash >= 0) ? filePath.substring(slash + 1) : filePath;
                                if (name.length() == 14 && name.endsWith(".csv")) {
                                    String datePart = name.substring(0, 10);
                                    if (datePart < cutoff_date && filePath.startsWith("/") && filePath.length() > 1) {
                                        if (SD.remove(filePath)) changed = true;
                                    }
                                }
                            }
                            dayFile.close();
                            dayFile = sensorDir.openNextFile();
                        }
                    }
                    sensorDir.close();
                    sensorDir = root.openNextFile();
                }
                root.close();
            }
        }
    }

    // 3) hourly rollup retention by day.
    if (hourlyRollupRetentionDays > 0 && SD.exists(ROLLUP_HOURLY_FOLDER)) {
        time_t now = time(nullptr);
        if (now > 0) {
            time_t cutoff = now - (time_t)hourlyRollupRetentionDays * 24 * 60 * 60;
            struct tm cutoff_tm;
            localtime_r(&cutoff, &cutoff_tm);
            char cutoff_name[16];
            strftime(cutoff_name, sizeof(cutoff_name), "%Y-%m-%d", &cutoff_tm);
            String cutoff_date(cutoff_name);

            File root = SD.open(ROLLUP_HOURLY_FOLDER);
            if (root && root.isDirectory()) {
                File sensorDir = root.openNextFile();
                while (sensorDir) {
                    if (sensorDir.isDirectory()) {
                        String sensorDirPath = sensorDir.name();
                        if (!sensorDirPath.startsWith("/")) {
                            sensorDirPath = String(ROLLUP_HOURLY_FOLDER) + "/" + sensorDirPath;
                        }
                        File dayFile = sensorDir.openNextFile();
                        while (dayFile) {
                            if (!dayFile.isDirectory()) {
                                String filePath = toAbsolutePath(sensorDirPath, dayFile.name());
                                int slash = filePath.lastIndexOf('/');
                                String name = (slash >= 0) ? filePath.substring(slash + 1) : filePath;
                                if (name.length() == 14 && name.endsWith(".csv")) {
                                    String datePart = name.substring(0, 10);
                                    if (datePart < cutoff_date && filePath.startsWith("/") && filePath.length() > 1) {
                                        if (SD.remove(filePath)) changed = true;
                                    }
                                }
                            }
                            dayFile.close();
                            dayFile = sensorDir.openNextFile();
                        }
                    }
                    sensorDir.close();
                    sensorDir = root.openNextFile();
                }
                root.close();
            }
        }
    }

    // 4) monthly rollup retention by month (YYYY-MM.csv).
    if (monthlyRollupRetentionMonths > 0 && SD.exists(ROLLUP_MONTHLY_FOLDER)) {
        time_t now = time(nullptr);
        if (now > 0) {
            struct tm now_tm;
            localtime_r(&now, &now_tm);
            now_tm.tm_mon -= (int)monthlyRollupRetentionMonths;
            mktime(&now_tm);
            char cutoff_month_name[8];
            strftime(cutoff_month_name, sizeof(cutoff_month_name), "%Y-%m", &now_tm);
            String cutoff_month(cutoff_month_name);

            File root = SD.open(ROLLUP_MONTHLY_FOLDER);
            if (root && root.isDirectory()) {
                File sensorDir = root.openNextFile();
                while (sensorDir) {
                    if (sensorDir.isDirectory()) {
                        String sensorDirPath = sensorDir.name();
                        if (!sensorDirPath.startsWith("/")) {
                            sensorDirPath = String(ROLLUP_MONTHLY_FOLDER) + "/" + sensorDirPath;
                        }
                        File monthFile = sensorDir.openNextFile();
                        while (monthFile) {
                            if (!monthFile.isDirectory()) {
                                String filePath = toAbsolutePath(sensorDirPath, monthFile.name());
                                int slash = filePath.lastIndexOf('/');
                                String name = (slash >= 0) ? filePath.substring(slash + 1) : filePath;
                                if (name.length() == 11 && name.endsWith(".csv")) {
                                    String monthPart = name.substring(0, 7);
                                    if (monthPart < cutoff_month && filePath.startsWith("/") && filePath.length() > 1) {
                                        if (SD.remove(filePath)) changed = true;
                                    }
                                }
                            }
                            monthFile.close();
                            monthFile = sensorDir.openNextFile();
                        }
                    }
                    sensorDir.close();
                    sensorDir = root.openNextFile();
                }
                root.close();
            }
        }
    }

    // 5) exceptions retention: keep only the newest N boot_*.txt files.
    if (maxBootFiles > 0 && SD.exists(EXCEPTIONS_FOLDER)) {
        std::vector<std::pair<uint32_t, String>> boots;
        File ex = SD.open(EXCEPTIONS_FOLDER);
        if (ex && ex.isDirectory()) {
            File f = ex.openNextFile();
            while (f) {
                if (!f.isDirectory()) {
                    String fullPath = toAbsolutePath(String(EXCEPTIONS_FOLDER), f.name());
                    int slash = fullPath.lastIndexOf('/');
                    String name = (slash >= 0) ? fullPath.substring(slash + 1) : fullPath;
                    if (name.startsWith("boot_") && name.endsWith(".txt")) {
                        int us = name.indexOf('_');
                        int dot = name.lastIndexOf('.');
                        String n = name.substring(us + 1, dot);
                        uint32_t id = (uint32_t)n.toInt();
                        boots.push_back({id, fullPath});
                    }
                }
                f.close();
                f = ex.openNextFile();
            }
            ex.close();
        }

        if (boots.size() > maxBootFiles) {
            std::sort(boots.begin(), boots.end(),
                      [](const std::pair<uint32_t, String>& a, const std::pair<uint32_t, String>& b) {
                          return a.first < b.first;
                      });
            size_t removeCount = boots.size() - maxBootFiles;
            for (size_t i = 0; i < removeCount; i++) {
                const String& path = boots[i].second;
                if (path.length() > 1 && path.startsWith("/")) {
                    if (SD.remove(path)) {
                        changed = true;
                        debug_outln_verbose(F("[SDCardLogger] Retention removed old boot log: "), path);
                    }
                } else {
                    debug_outln_info(F("[SDCardLogger] Retention skip invalid boot log path: "), path);
                }
            }
        }
    }

    if (changed) {
        refreshCache();
    }
    return true;
}

String SDCard::_getCardTypeName(sdcard_type_t type) {
    if (type == CARD_MMC) {
        return String("MMC");
    } else if (type == CARD_SD) {
        return String("SD");
    } else if (type == CARD_SDHC) {
        return String("SDHC");
    } else if (type == CARD_UNKNOWN) {
        return String("UNKNOWN");
    } else {
        return String("NONE");
    }
}

#endif
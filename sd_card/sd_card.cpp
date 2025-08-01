#if defined(USE_SD_CARD)

#include "sd_card.h"
#include "../defines.h"

bool SDCard::begin() {
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
    debug_outln_info(F("SD Card Size (MB): "), cardSizeMB);
    debug_outln_info(F("Used space (MB): "), usedMemMB);
    return true;
}

void SDCard::refreshCache() {
    _sensorList.clear();
    _sensorLastFiles.clear();

    if (!SD.exists(ROOT_FOLDER)) {
        if (SD.mkdir(ROOT_FOLDER)) {
            debug_outln_info(F("[SDCardLogger] Root folder created"));
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
    String folder = ROOT_FOLDER + sensorName;
    if (!SD.exists(folder)) {
        if (SD.mkdir(folder)) {
            _sensorList.push_back(sensorName);
            debug_outln_info(F("[SDCardLogger] folder created: "), folder);
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
        return;
    }

    debug_outln_info(F("[SDCardLogger] Start writing data to file "), fullPath);

    _sensorLastFiles[sensorName] = filename;

    // Если файл новый — пишем заголовок
    if (!fileExists || file.size() == 0) {
        if (file.println(header)) {
            debug_outln_info(F("[SDCardLogger] Header writed: "), header);
        } else {
            debug_outln_info(F("Can't write header "));
        }
    }

    if (file.println(values)) {
        debug_outln_info(F("[SDCardLogger] Logged to: "), fullPath);
        debug_outln_info(F("[SDCardLogger] Data: "), values);
    } else {
        debug_outln_info(F("[SDCardLogger] Can't write data: "));
    }
    file.close();
}


void readSensorDataFromCSV(LineData &result, const char* sensor_name, const char* field_name, int hours_back) {

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    char filename[64];
    snprintf(filename, sizeof(filename), "/sensors_data/%s/%04d-%02d-%02d.csv",
             sensor_name, timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);

    File file = SD.open(filename, FILE_READ);
    if (!file) {
        debug_outln_info(F("Failed to open: "), filename);
        return;
    }

    // Временная память для хранения считанных значений
    std::vector<float> values_vec;
    std::vector<uint32_t> timestamps_vec;

    // Читаем заголовок
    String header_line = file.readStringUntil('\n');
    int field_index = -1;

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
            break;
        }
    }

    if (field_index == -1) {
        debug_outln_info(F("Field not found in CSV"));
        file.close();
        return;
    }

    // Граница по времени
    uint32_t time_limit = now - (hours_back * 3600);

    // Чтение данных построчно
    while (file.available()) {
        String line = file.readStringUntil('\n');
        std::vector<String> parts;
        int pos = 0;
        while (pos < line.length()) {
            int comma = line.indexOf(',', pos);
            if (comma == -1) comma = line.length();
            parts.push_back(line.substring(pos, comma));
            pos = comma + 1;
        }

        if (parts.size() <= field_index) continue;

        uint32_t timestamp = parts[0].toInt();
        if (timestamp < time_limit) continue;
        
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

    // Переносим в C-массивы
    result.count = values_vec.size();
    result.values = new float[result.count];
    result.timestamps = new uint32_t[result.count];

    for (int i = 0; i < result.count; ++i) {
        result.values[i] = values_vec[i];
        result.timestamps[i] = timestamps_vec[i];
    }

}

bool SDCard::checkInserted() {
    sdcard_type_t card_type = SD.cardType();
    debug_outln_info(F("[SDCardLogger] Check sd card, type: "), _getCardTypeName(card_type));
    if (card_type == CARD_NONE) {
        debug_outln_info("[SDCardLogger] No SD card present, retry begin...");
        return _beginSD(SPI);
    } else {
        return true;
    }
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
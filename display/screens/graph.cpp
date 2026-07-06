#ifdef ALTRUIST_INSIGHT

#include "../paint_driver/graphPainter.h"
#include "graph.h"
#include "../utils.h"
#include "../../utils.h"
#include "../../intl.h"
#include "../../sd_card/sd_card.h"
#include "../../config_manager/config_helpers.h"
#include "../icons/icons/icons_15x15.h"
#include "../paint_driver/fonts/fonts.h"
#include <vector>

#if defined(USE_SD_CARD)
#include "SD.h"
#include "FS.h"
#endif

#if defined(USE_SD_CARD)
namespace {
class SDGraphLockGuard {
public:
    explicit SDGraphLockGuard(uint32_t timeout_ms = 400) : locked(sdCardLockWithYield(timeout_ms)) {}
    ~SDGraphLockGuard() { if (locked) sdCardUnlock(); }
    bool ok() const { return locked; }
private:
    bool locked;
};
} // namespace
#endif

uint8_t current_graph_screen = 1;
static GraphValue current_graph_value = GraphValue::INSIGHT_TEMP;
constexpr uint8_t kGraphValueCount = static_cast<uint8_t>(GraphValue::URBAN_PRESSURE) + 1;
static bool g_graph_had_data[kGraphValueCount] = {false};
static uint8_t g_graph_no_data_streak[kGraphValueCount] = {0};
static bool g_graphs_navigation_can_cycle = false;
static bool g_graph_sd_retry_pending = false;
static uint32_t g_graph_sd_retry_at_ms = 0;
static uint8_t g_graph_sd_retry_attempts = 0;
static uint32_t g_graph_sd_backoff_until_ms = 0;
static bool g_graph_sd_gave_up = false;
static constexpr uint8_t kGraphSdMaxRetries = 6;

static bool g_graph_sd_probe_cache_valid = false;
static bool g_graph_sd_files_ok_cached = false;
static bool g_graph_sd_data_ready_cached = false;

void graphMarkValueSwitch() {
    g_graph_sd_retry_attempts = 0;
    g_graph_sd_retry_pending = false;
    g_graph_sd_backoff_until_ms = 0;
    g_graph_sd_gave_up = false;
}

void graphInvalidateSdProbeCache() {
    g_graph_sd_probe_cache_valid = false;
    g_graph_sd_files_ok_cached = false;
    g_graph_sd_data_ready_cached = false;
    g_graph_sd_retry_attempts = 0;
    g_graph_sd_retry_pending = false;
    g_graph_sd_backoff_until_ms = 0;
    g_graph_sd_gave_up = false;
}

void graphClearDeferOnScreenEntry() {
    g_graph_sd_retry_attempts = 0;
    g_graph_sd_retry_pending = false;
    g_graph_sd_backoff_until_ms = 0;
    g_graph_sd_gave_up = false;
}

static uint32_t graphSdRetryBackoffMs() {
    switch (g_graph_sd_retry_attempts) {
        case 0: return 800;
        case 1: return 1500;
        case 2: return 3000;
        default: return 5000;
    }
}

static bool graphSdInBackoff() {
    return g_graph_sd_backoff_until_ms != 0 &&
           (int32_t)(millis() - g_graph_sd_backoff_until_ms) < 0;
}

static void graphNoteSdBusy() {
    if (g_graph_sd_gave_up) {
        return;
    }
    if (g_graph_sd_retry_pending) {
        return;
    }
    if (g_graph_sd_retry_attempts >= kGraphSdMaxRetries) {
        g_graph_sd_gave_up = true;
        g_graph_sd_retry_pending = false;
        debug_outln_info(F("[Graph] SD busy — stop auto-retry until screen/metric change"));
        return;
    }
    g_graph_sd_retry_attempts++;
    const uint32_t delay_ms = graphSdRetryBackoffMs();
    g_graph_sd_backoff_until_ms = millis() + delay_ms;
    g_graph_sd_retry_pending = true;
    g_graph_sd_retry_at_ms = g_graph_sd_backoff_until_ms;
}

static void graphNoteSdDrawSuccess() {
    g_graph_sd_retry_attempts = 0;
    g_graph_sd_retry_pending = false;
    g_graph_sd_backoff_until_ms = 0;
    g_graph_sd_gave_up = false;
}

bool graphScreenSdRetryDue() {
    if (!g_graph_sd_retry_pending) {
        return false;
    }
    if ((int32_t)(millis() - g_graph_sd_retry_at_ms) < 0) {
        return false;
    }
    g_graph_sd_retry_pending = false;
    return true;
}

static uint8_t graphValueIndex(GraphValue value) {
    return static_cast<uint8_t>(value);
}

static void markGraphDataState(GraphValue value, bool has_data) {
    const uint8_t idx = graphValueIndex(value);
    if (idx >= kGraphValueCount) return;
    if (has_data) {
        g_graph_had_data[idx] = true;
        g_graph_no_data_streak[idx] = 0;
    } else if (g_graph_no_data_streak[idx] < 255) {
        g_graph_no_data_streak[idx]++;
    }
}

static bool shouldShowNoDataMessage(GraphValue value) {
    const uint8_t idx = graphValueIndex(value);
    if (idx >= kGraphValueCount) return true;
    // If this graph has never had data in this boot, show message immediately.
    if (!g_graph_had_data[idx]) return true;
    // If we had data before, suppress one transient empty read to avoid flicker.
    return g_graph_no_data_streak[idx] >= 2;
}

// Human‑readable title for the current graph (measure only, no source prefix)
static const char* getGraphTitle(GraphValue value) {
    switch (value) {
        case GraphValue::INSIGHT_TEMP:
        case GraphValue::URBAN_TEMP:
            return INTL_DISP_TEMPERATURE;
        case GraphValue::INSIGHT_HUM:
        case GraphValue::URBAN_HUM:
            return INTL_DISP_HUMIDITY;
        case GraphValue::INSIGHT_CO2:
            return INTL_CO2;
        case GraphValue::INSIGHT_PRESSURE:
        case GraphValue::URBAN_PRESSURE:
            return INTL_DISP_PRESSURE;
        case GraphValue::URBAN_AIR:
            return INTL_DISP_AIR_QUALITY;
        case GraphValue::URBAN_NOISE:
            return INTL_DISP_NOISE;
        default:
            return "";
    }
}

// Filter data to hourly intervals (keep only the last value in each hour)
static void filterToHourlyData(LineData &data) {
    if (data.count == 0 || !data.values || !data.timestamps) {
        return;
    }
    
    // Group data by hour and keep only the last value in each hour
    std::vector<float> hourly_values;
    std::vector<uint32_t> hourly_timestamps;
    
    uint32_t current_hour = 0;
    bool first_hour = true;
    float last_value_in_hour = 0;
    uint32_t last_timestamp_in_hour = 0;
    
    for (int i = 0; i < data.count; i++) {
        uint32_t timestamp = data.timestamps[i];
        uint32_t hour = timestamp / 3600;  // Group by hour (Unix hour)
        
        if (first_hour) {
            current_hour = hour;
            first_hour = false;
        } else if (hour != current_hour) {
            // New hour - save the last value from previous hour
            hourly_values.push_back(last_value_in_hour);
            hourly_timestamps.push_back(last_timestamp_in_hour);
            current_hour = hour;
        }
        
        // Always update to the latest value in this hour
        last_value_in_hour = data.values[i];
        last_timestamp_in_hour = timestamp;
    }
    
    if (data.count > 0) {
        hourly_values.push_back(last_value_in_hour);
        hourly_timestamps.push_back(last_timestamp_in_hour);
    }
    
    // Replace the original data with filtered data
    delete[] data.values;
    delete[] data.timestamps;
    
    data.count = hourly_values.size();
    if (data.count > 0) {
        data.values = new float[data.count];
        data.timestamps = new uint32_t[data.count];
        for (int i = 0; i < data.count; i++) {
            data.values[i] = hourly_values[i];
            data.timestamps[i] = hourly_timestamps[i];
        }
    } else {
        data.values = nullptr;
        data.timestamps = nullptr;
    }
}

#if defined(USE_SD_CARD)
// External reference to SD card logger
extern SDCard sdCardLogger;

// Direct exists checks are enough for graphs (no SD cache refresh on this path).
static constexpr uint32_t kGraphSdReadTimeoutMs = 400;
static constexpr uint32_t kGraphProbeSdReadTimeoutMs = 400;

static void drawGraphLoadingMessage(uint16_t navTop, uint16_t contentTop) {
    const char* msg = INTL_DISP_LOADING;
    uint16_t msg_w = Paint_GetStringWidth_Display(msg, &Font16, &font_16_cyrillic, &font_16_ascii);
    uint16_t cx = DISPLAY_WIDTH / 2;
    uint16_t cy = contentTop + (navTop - contentTop) / 2;
    Paint_DrawString_Display(cx - msg_w / 2, cy - Font16.Height / 2, msg,
                             &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
}

/** Erase graph content band so a prior Loading label cannot bleed through. */
static void graphClearContentArea(uint16_t contentTop, uint16_t navTop) {
    if (navTop <= contentTop + 4) {
        return;
    }
    const uint16_t right = (DISPLAY_WIDTH > 30) ? (DISPLAY_WIDTH - 30) : DISPLAY_WIDTH;
    Paint_DrawRectangle(0, contentTop, right, navTop - 1, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

static bool checkSDCardAvailable() {
    // Use checkInserted which properly detects card removal
    // It checks card type and tries to reinitialize if needed
    bool isConnected = sdCardLogger.checkInserted();
    if (!isConnected) {
        // Card was removed - update device status
        debug_outln_verbose(F("[Graph] SD card removed - updating status"));
        return false;
    }
    return true;
}

static bool checkDataFilesExist(bool* sd_busy_out = nullptr) {
#if defined(USE_SD_CARD)
    if (sd_busy_out) {
        *sd_busy_out = false;
    }
    SDGraphLockGuard sdLock;
    if (!sdLock.ok()) {
        debug_outln_verbose(F("[Graph] Failed to acquire SD lock"));
        if (sd_busy_out) {
            *sd_busy_out = true;
        }
        return false;
    }
#endif
    // First, verify card is still present (detect removal)
    if (!sdCardLogger.checkInserted()) {
        debug_outln_verbose(F("[Graph] SD card removed during file check"));
        return false;
    }

    // Check if root folder exists
    if (!SD.exists("/sensors_data")) {
        debug_outln_verbose(F("[Graph] /sensors_data folder does not exist"));
        return false;
    }
    
    // First, try direct check for today's files from known sensors (faster)
    time_t now_time = time(nullptr);
    struct tm* timeinfo = localtime(&now_time);
    char dateStr[11];
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", 
             timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
    
    String dateInfo = "[Graph] Checking for date: " + String(dateStr);
    debug_outln_verbose(dateInfo);
    
    // Check common sensor folders for today's file
    const char* sensors[] = {"SCD4x", "BME680", ATRUIST_URBAN_SENSOR};
    const int sensorCount = cfg::standalone ? 2 : 3;
    for (int i = 0; i < sensorCount; i++) {
        String filePath = "/sensors_data/" + String(sensors[i]) + "/" + String(dateStr) + ".csv";
        String checkMsg = "[Graph] Checking file: " + filePath;
        debug_outln_verbose(checkMsg);
        if (SD.exists(filePath)) {
            File testFile = SD.open(filePath, FILE_READ);
            if (testFile && testFile.size() > 0) {
                String foundMsg = "[Graph] Found today's file: " + filePath + " size: " + String(testFile.size());
                debug_outln_verbose(foundMsg);
                testFile.close();
                return true;
            }
            if (testFile) {
                String emptyMsg = "[Graph] File exists but empty or can't read: " + filePath;
                debug_outln_verbose(emptyMsg);
                testFile.close();
            }
        } else {
            String notFoundMsg = "[Graph] File does not exist: " + filePath;
            debug_outln_verbose(notFoundMsg);
        }
    }
    
    // If direct check didn't find files, do full directory scan
    File root = SD.open("/sensors_data");
    if (!root || !root.isDirectory()) {
        debug_outln_verbose(F("[Graph] Failed to open /sensors_data directory"));
        return false;
    }
    
    bool hasData = false;
    int sensorDirCount = 0;
    int fileCount = 0;
    
    File entry = root.openNextFile();
    while (entry) {
        if (entry.isDirectory()) {
            sensorDirCount++;
            // Check if this sensor folder has any CSV files
            String sensorName = entry.name();
            // entry.name() returns just the folder name, need to build full path
            // Remove any leading path components
            int lastSlash = sensorName.lastIndexOf('/');
            if (lastSlash >= 0) {
                sensorName = sensorName.substring(lastSlash + 1);
            }
            // Build full path
            String sensorPath = "/sensors_data/" + sensorName;
            String folderInfo = "[Graph] Checking sensor folder: " + sensorPath;
            debug_outln_verbose(folderInfo);
            File sensorDir = SD.open(sensorPath);
            if (sensorDir && sensorDir.isDirectory()) {
                File csvFile = sensorDir.openNextFile();
                while (csvFile) {
                    String fileName = csvFile.name();
                    fileCount++;
                    String fileInfo = "[Graph] Found file: " + fileName + " size: " + String(csvFile.size());
                    debug_outln_verbose(fileInfo);
                    if (fileName.endsWith(".csv") && csvFile.size() > 0) {
                        hasData = true;
                        debug_outln_verbose(F("[Graph] Found valid CSV file with data"));
                        csvFile.close();
                        sensorDir.close();
                        root.close();
                        return true;
                    }
                    csvFile.close();
                    csvFile = sensorDir.openNextFile();
                }
                sensorDir.close();
            }
        }
        entry.close();
        entry = root.openNextFile();
    }
    root.close();
    
    String checkResult = "[Graph] Checked " + String(sensorDirCount) + " sensors, " + String(fileCount) + " files total. Has data: " + (hasData ? "yes" : "no");
    debug_outln_verbose(checkResult);
    return hasData;
}

static bool buildGraphUpdateNote(char *out, size_t out_size, GraphValue value, const LineData *lineData, int lineCount) {
    if (!out || out_size == 0) return false;
    out[0] = '\0';
    if (!lineData || lineCount <= 0) return false;

    auto lastValue = [&](int idx, float &v_out) -> bool {
        if (idx < 0 || idx >= lineCount) return false;
        const LineData &ld = lineData[idx];
        if (ld.count <= 0 || ld.values == nullptr) return false;
        v_out = ld.values[ld.count - 1];
        return true;
    };

    float v1 = 0.0f, v2 = 0.0f;
    bool has1 = lastValue(0, v1);
    bool has2 = lastValue(1, v2);
    if (!has1 && !has2) return false;

    const char *unit = "";
    switch (value) {
        case GraphValue::INSIGHT_TEMP:
        case GraphValue::URBAN_TEMP:
            unit = "°C";
            break;
        case GraphValue::INSIGHT_HUM:
        case GraphValue::URBAN_HUM:
            unit = "%";
            break;
        case GraphValue::INSIGHT_CO2:
            unit = "ppm";
            break;
        case GraphValue::INSIGHT_PRESSURE:
        case GraphValue::URBAN_PRESSURE:
            unit = "mmHg";
            break;
        case GraphValue::URBAN_AIR:
            unit = "ug/m3";
            break;
        case GraphValue::URBAN_NOISE:
            unit = "dB";
            break;
        default:
            unit = "";
            break;
    }

    char s1[16] = {0};
    char s2[16] = {0};
    int precision = 0;
    if (value == GraphValue::URBAN_AIR) precision = 1;   // PM can be fractional (e.g. 2.5)
    if (has1) stringFromFloat(s1, v1, precision);
    if (has2) stringFromFloat(s2, v2, precision);

    if (has1 && has2) {
        // Two-line graphs: use descriptive labels instead of "v1|v2".
        if (value == GraphValue::URBAN_AIR) {
            // line 0 = PM10, line 1 = PM2.5 (see addLine order)
            snprintf(out, out_size, "PM10 %s, PM2.5 %s %s", s1, s2, unit);
        } else if (value == GraphValue::URBAN_NOISE) {
            // line 0 = Max, line 1 = Avg (see addLine order)
            snprintf(out, out_size, "Max %s, Avg %s %s", s1, s2, unit);
        } else {
            snprintf(out, out_size, "%s, %s %s", s1, s2, unit);
        }
    } else if (has1) {
        snprintf(out, out_size, "%s %s", s1, unit);
    } else {
        snprintf(out, out_size, "%s %s", s2, unit);
    }
    return true;
}

// Draw a single, full-width graph for the currently selected value.
// The graph is positioned to sit below the header and above the bottom navigation bar.
static void drawActiveGraph(GraphValue value, const String& urban_key, uint16_t navTop, uint16_t contentTop) {
    // Horizontal and vertical margins
    const uint16_t marginX    = 10;
    const uint16_t topMargin  = contentTop;
    const uint16_t bottomGap  = 6;   // gap between graph and nav bar separator

    if (navTop <= topMargin + bottomGap + 40) {
        return;
    }

    graphClearContentArea(topMargin, navTop);

    if (graphSdInBackoff() || g_graph_sd_gave_up) {
        drawGraphLoadingMessage(navTop, topMargin);
        return;
    }

    const uint16_t rightSidebarWidth = 29;
    const uint16_t rightMargin = 0;  // No right margin - graph goes right up to sidebar
    uint16_t graphLeft   = marginX;
    uint16_t graphWidth  = DISPLAY_WIDTH - marginX - rightSidebarWidth - rightMargin;  // Left margin + right sidebar, no extra right margin
    uint16_t graphHeight = navTop - topMargin - bottomGap;
    uint16_t graphBottom = navTop - bottomGap;

    GraphPainter graph(graphLeft, graphBottom, graphHeight, graphWidth);
    graph.setWhiteMode();

    LineData lineData[2] = {};
    int      lineCount   = 0;

    // We'll determine hours_back after checking data, but for now use 12 as max
    // This will be updated after we calculate the actual time range
    uint8_t hours_to_read = 12;
    bool sd_read_busy = false;
    
    auto addLine = [&](const char* sensor_name, const char* field_name) {
        if (lineCount >= 2 || sd_read_busy) {
            return;
        }
        LineData &ld = lineData[lineCount];
        if (!readSensorDataFromCSV(ld, sensor_name, field_name, hours_to_read, kGraphSdReadTimeoutMs)) {
            sd_read_busy = true;
            return;
        }
        if (ld.count > 0 && ld.values && ld.timestamps) {
            lineCount++;
        } else {
            if (ld.values) {
                delete[] ld.values;
                ld.values = nullptr;
            }
            if (ld.timestamps) {
                delete[] ld.timestamps;
                ld.timestamps = nullptr;
            }
            ld.count = 0;
        }
    };

    // Update metrics to get current uptime
    updateMetrics();
    
    // Determine which sensor to use for temperature and humidity based on uptime
    // First 5 minutes (300 seconds): use BME680
    // After 5 minutes: use SCD4x
    bool use_bme680_for_temp_hum = (system_metrics.uptime_sec < 300);

    switch (value) {
        case GraphValue::INSIGHT_TEMP:
            // Single-line Insight graphs: show only "Insight: <value>" in legend
            // Use BME680 for first 5 minutes, then switch to SCD4x
            if (use_bme680_for_temp_hum) {
                addLine("BME680", "temperature");
            } else {
                addLine("SCD4x", "temperature");
            }
            break;
        case GraphValue::INSIGHT_HUM:
            // Use BME680 for first 5 minutes, then switch to SCD4x
            if (use_bme680_for_temp_hum) {
                addLine("BME680", "humidity");
            } else {
                addLine("SCD4x", "humidity");
            }
            break;
        case GraphValue::INSIGHT_CO2:
            addLine("SCD4x", "co2");
            break;
        case GraphValue::INSIGHT_PRESSURE:
            addLine("BME680", "pressure");
            break;
        case GraphValue::URBAN_AIR: {
            addLine(urban_key.c_str(), "SDS_P1");
            addLine(urban_key.c_str(), "SDS_P2");
            break;
        }
        case GraphValue::URBAN_NOISE:
            addLine(urban_key.c_str(), "PCBA_noiseMax");
            addLine(urban_key.c_str(), "PCBA_noiseAvg");
            break;
        case GraphValue::URBAN_TEMP:
            addLine(urban_key.c_str(), "BME280_temperature");
            break;
        case GraphValue::URBAN_HUM:
            addLine(urban_key.c_str(), "BME280_humidity");
            break;
        case GraphValue::URBAN_PRESSURE:
            addLine(urban_key.c_str(), "BME280_pressure");
            break;
    }

    if (sd_read_busy && lineCount == 0) {
        drawGraphLoadingMessage(navTop, topMargin);
        graphNoteSdBusy();
        return;
    }

    if (lineCount > 0) {
        time_t now = time(nullptr);
        uint32_t oldest_timestamp = UINT32_MAX;
        uint32_t newest_timestamp = 0;
        int raw_point_count = 0;
        
        for (int i = 0; i < lineCount; i++) {
            if (lineData[i].count > 0 && lineData[i].timestamps) {
                raw_point_count += lineData[i].count;
                for (int j = 0; j < lineData[i].count; j++) {
                    if (lineData[i].timestamps[j] < oldest_timestamp) {
                        oldest_timestamp = lineData[i].timestamps[j];
                    }
                    if (lineData[i].timestamps[j] > newest_timestamp) {
                        newest_timestamp = lineData[i].timestamps[j];
                    }
                }
            }
        }
        
        if (oldest_timestamp != UINT32_MAX && newest_timestamp > 0) {
            markGraphDataState(value, true);
            graphNoteSdDrawSuccess();
            uint32_t data_span_seconds = newest_timestamp - oldest_timestamp;
            float total_data_hours_float = (float)data_span_seconds / 3600.0f;
            
            // Need ~1 h span in raw samples, or enough points for a meaningful curve.
            const bool enough_data =
                (total_data_hours_float >= 1.0f) || (raw_point_count >= 30);
            if (!enough_data) {
                // Show helpful message instead of graph - centered vertically in available space
                uint16_t graphCenterX = graphLeft + graphWidth / 2;
                uint16_t availableHeight = navTop - topMargin;
                uint16_t graphCenterY = topMargin + availableHeight / 2;
                
                const char* msg1 = INTL_DISP_NOT_ENOUGH_DATA_YET;
                const char* msg2 = INTL_DISP_COLLECTING_DATA;
                
                uint16_t msg1Width = Paint_GetStringWidth_Display(msg1, &Font16, &font_16_cyrillic, &font_16_ascii);
                uint16_t msg2Width = Paint_GetStringWidth_Display(msg2, &Font12, &font_12_cyrillic, &font_12_ascii);
                
                
                uint16_t line_spacing = 8;
                uint16_t total_msg_height = Font16.Height + Font12.Height + line_spacing;
                
                uint16_t msg1X = graphCenterX - msg1Width / 2;
                uint16_t msg2X = graphCenterX - msg2Width / 2;
                uint16_t msg1Y = graphCenterY - total_msg_height / 2;
                uint16_t msg2Y = msg1Y + Font16.Height + line_spacing;
                
                Paint_DrawString_Display(msg1X, msg1Y, msg1, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
                Paint_DrawString_Display(msg2X, msg2Y, msg2, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
                
                for (int i = 0; i < lineCount; i++) {
                    if (lineData[i].values) {
                        delete[] lineData[i].values;
                        lineData[i].values = nullptr;
                    }
                    if (lineData[i].timestamps) {
                        delete[] lineData[i].timestamps;
                        lineData[i].timestamps = nullptr;
                    }
                    lineData[i].count = 0;
                }
                return;
            }
            
            uint8_t total_data_hours = (uint8_t)(total_data_hours_float + 0.5f);  // Round to nearest
            
            // Dynamic time range logic:
            // - If <= 12 hours of data: show all available data (1, 2, 3... up to 12 hours)
            // - If > 12 hours: show sliding window of last 12 hours (new data appears as old data falls out)
            uint8_t display_hours;
            if (total_data_hours <= 12) {
                display_hours = total_data_hours;
            } else {
                display_hours = 12;  // Sliding window: show last 12 hours
            }
            
            if (display_hours < 1) display_hours = 1;
            
            graph.setShowHours(display_hours);

            int plotted_lines = 0;
            for (int i = 0; i < lineCount; i++) {
                if (lineData[i].count <= 0 || !lineData[i].values || !lineData[i].timestamps) {
                    continue;
                }
                filterToHourlyData(lineData[i]);
                if (lineData[i].count <= 0) {
                    continue;
                }
                const char* plot_label = "";
                GraphLineStyle plot_style;
                plot_style.style = LINE_STYLE_SOLID;
                plot_style.width = DOT_PIXEL_1X1;
                plot_style.use_main_color = true;
                switch (value) {
                    case GraphValue::INSIGHT_TEMP:
                    case GraphValue::INSIGHT_HUM:
                    case GraphValue::INSIGHT_CO2:
                    case GraphValue::INSIGHT_PRESSURE:
                    case GraphValue::URBAN_TEMP:
                    case GraphValue::URBAN_HUM:
                    case GraphValue::URBAN_PRESSURE:
                        plot_label = (value == GraphValue::INSIGHT_TEMP || value == GraphValue::INSIGHT_HUM ||
                                      value == GraphValue::INSIGHT_CO2 || value == GraphValue::INSIGHT_PRESSURE)
                                         ? "Insight"
                                         : "Urban";
                        break;
                    case GraphValue::URBAN_AIR:
                        plot_label = (plotted_lines == 0) ? "PM10" : "PM2.5";
                        if (plotted_lines == 1) {
                            plot_style.style = LINE_STYLE_DOTTED;
                        }
                        break;
                    case GraphValue::URBAN_NOISE:
                        plot_label = (plotted_lines == 0) ? "Max" : "Avg";
                        if (plotted_lines == 1) {
                            plot_style.style = LINE_STYLE_DOTTED;
                        }
                        break;
                    default:
                        break;
                }
                graph.addLineValues(lineData[i].values, lineData[i].timestamps, lineData[i].count,
                                    plot_label, plot_style);
                plotted_lines++;
            }
        } else {
            // No timestamps found - show message
            markGraphDataState(value, false);
            if (!shouldShowNoDataMessage(value)) {
                for (int i = 0; i < lineCount; i++) {
                    if (lineData[i].values) {
                        delete[] lineData[i].values;
                        lineData[i].values = nullptr;
                    }
                    if (lineData[i].timestamps) {
                        delete[] lineData[i].timestamps;
                        lineData[i].timestamps = nullptr;
                    }
                    lineData[i].count = 0;
                }
                return;
            }
            uint16_t graphCenterX = graphLeft + graphWidth / 2;
            uint16_t graphCenterY = navTop - (navTop - topMargin) / 2;
            
            const char* msg1 = INTL_DISP_NO_DATA_AVAILABLE;
            const char* msg2 = INTL_DISP_COLLECTING_DATA;
            
            uint16_t msg1Width = Paint_GetStringWidth_Display(msg1, &Font16, &font_16_cyrillic, &font_16_ascii);
            uint16_t msg2Width = Paint_GetStringWidth_Display(msg2, &Font12, &font_12_cyrillic, &font_12_ascii);
            
            uint16_t msg1X = graphCenterX - msg1Width / 2;
            uint16_t msg2X = graphCenterX - msg2Width / 2;
            uint16_t msg1Y = graphCenterY - Font16.Height / 2 - 4;
            uint16_t msg2Y = graphCenterY + Font12.Height / 2 + 4;
            
            Paint_DrawString_Display(msg1X, msg1Y, msg1, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
            Paint_DrawString_Display(msg2X, msg2Y, msg2, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
            
            // Clean up and return early
            for (int i = 0; i < lineCount; i++) {
                if (lineData[i].values) {
                    delete[] lineData[i].values;
                    lineData[i].values = nullptr;
                }
                if (lineData[i].timestamps) {
                    delete[] lineData[i].timestamps;
                    lineData[i].timestamps = nullptr;
                }
                lineData[i].count = 0;
            }
            return;
        }
        
        char update_note[48];
        if (buildGraphUpdateNote(update_note, sizeof(update_note), value, lineData, lineCount)) {
            graph.setUpdateNote(update_note);
        } else {
            graph.setUpdateNote(nullptr);
        }
        graph.drawGraph();
    } else {
        // No data lines at all - show message
        markGraphDataState(value, false);
        if (!shouldShowNoDataMessage(value)) {
            return;
        }
        uint16_t graphCenterX = graphLeft + graphWidth / 2;
        uint16_t graphCenterY = navTop - (navTop - topMargin) / 2;
        
        const char* msg1 = INTL_DISP_NO_DATA_AVAILABLE;
        const char* msg2 = INTL_DISP_COLLECTING_DATA;
        
        uint16_t msg1Width = Paint_GetStringWidth_Display(msg1, &Font16, &font_16_cyrillic, &font_16_ascii);
        uint16_t msg2Width = Paint_GetStringWidth_Display(msg2, &Font12, &font_12_cyrillic, &font_12_ascii);
        
        uint16_t msg1X = graphCenterX - msg1Width / 2;
        uint16_t msg2X = graphCenterX - msg2Width / 2;
        uint16_t msg1Y = graphCenterY - Font16.Height / 2 - 4;
        uint16_t msg2Y = graphCenterY + Font12.Height / 2 + 4;
        
        Paint_DrawString_Display(msg1X, msg1Y, msg1, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
        Paint_DrawString_Display(msg2X, msg2Y, msg2, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        return;
    }

    // Clean up allocated buffers
    for (int i = 0; i < lineCount; i++) {
        if (lineData[i].values) {
            delete[] lineData[i].values;
            lineData[i].values = nullptr;
        }
        if (lineData[i].timestamps) {
            delete[] lineData[i].timestamps;
            lineData[i].timestamps = nullptr;
        }
        lineData[i].count = 0;
    }
}
#endif

// Helper function to check if we have at least 1 hour of data
static bool hasEnoughData(bool* sd_busy_out = nullptr) {
#if defined(USE_SD_CARD)
    bool local_busy = false;
    if (!checkSDCardAvailable() || !checkDataFilesExist(&local_busy)) {
        if (sd_busy_out && local_busy) {
            *sd_busy_out = true;
        }
        return false;
    }
    
    // Try to read a small sample of data to check if we have at least 1 hour
    LineData testData = {};

    // Try reading from sensors that Insight actually logs
    static const char* kProbeSensors[] = {"SCD4x", "BME680"};
    bool sd_busy = false;
    for (const char* sensor : kProbeSensors) {
        if (!readSensorDataFromCSV(testData, sensor, "temperature", 12, kGraphProbeSdReadTimeoutMs)) {
            sd_busy = true;
            continue;
        }
        if (testData.count == 0 || !testData.timestamps) {
            if (testData.values) { delete[] testData.values; }
            if (testData.timestamps) { delete[] testData.timestamps; }
            testData = {};
            continue;
        }
        uint32_t oldest = UINT32_MAX;
        uint32_t newest = 0;
        for (int i = 0; i < testData.count; i++) {
            if (testData.timestamps[i] < oldest) oldest = testData.timestamps[i];
            if (testData.timestamps[i] > newest) newest = testData.timestamps[i];
        }
        const float span_hours =
            (oldest != UINT32_MAX && newest > oldest)
                ? (float)(newest - oldest) / 3600.0f
                : 0.0f;
        const bool ok = (span_hours >= 1.0f) || (testData.count >= 30);
        if (testData.values) { delete[] testData.values; }
        if (testData.timestamps) { delete[] testData.timestamps; }
        testData = {};
        if (ok) {
            return true;
        }
    }
    
    if (sd_busy_out && sd_busy) {
        *sd_busy_out = true;
    }
    return false;
#else
    return false;  // SD card not compiled in
#endif
}

// Public function to check if graphs are available
bool areGraphsAvailable() {
#if defined(USE_SD_CARD)
    return checkSDCardAvailable() && checkDataFilesExist() && hasEnoughData();
#else
    return false;  // SD card not compiled in
#endif
}

bool graphsNavigationCanCycle() {
    return g_graphs_navigation_can_cycle;
}

void drawGraphScreen() {
    bool show_nav_bar = false;
    bool sd_probe_busy = false;
#if defined(USE_SD_CARD)
    bool sd_busy = false;
    bool sd_ok = false;
    bool files_ok = false;
    bool data_ready = false;

    if (g_graph_sd_probe_cache_valid) {
        // Cache only skips the slow directory scan — still verify card is present.
        sd_ok = checkSDCardAvailable();
        if (!sd_ok) {
            graphInvalidateSdProbeCache();
            files_ok = false;
            data_ready = false;
            g_graphs_navigation_can_cycle = false;
        } else {
            files_ok = g_graph_sd_files_ok_cached;
            data_ready = g_graph_sd_data_ready_cached;
            g_graphs_navigation_can_cycle = files_ok && data_ready;
        }
    } else {
        sd_ok = checkSDCardAvailable();
        if (sd_ok) {
            files_ok = checkDataFilesExist(&sd_busy);
            if (files_ok) {
                bool data_busy = false;
                data_ready = hasEnoughData(&data_busy);
                if (!data_ready && data_busy) {
                    sd_busy = true;
                }
            }
        }
        const bool sd_busy_transient = sd_ok && sd_busy && (!files_ok || !data_ready);
        if (!sd_busy_transient) {
            g_graph_sd_probe_cache_valid = true;
            g_graph_sd_files_ok_cached = files_ok;
            g_graph_sd_data_ready_cached = data_ready;
            g_graphs_navigation_can_cycle = files_ok && data_ready;
            if (files_ok) {
                graphNoteSdDrawSuccess();
            }
        } else if (g_graph_sd_retry_attempts < kGraphSdMaxRetries) {
            sd_probe_busy = true;
            graphNoteSdBusy();
        } else {
            g_graphs_navigation_can_cycle = false;
        }
    }

    show_nav_bar = g_graphs_navigation_can_cycle;
#else
    g_graphs_navigation_can_cycle = false;
#endif

    // Clear screen first to prevent glitching
    Paint_Clear(WHITE);

    if (cfg::standalone) {
        const uint8_t v = (uint8_t)current_graph_value;
        if (v >= (uint8_t)GraphValue::URBAN_AIR) {
            current_graph_value = GraphValue::INSIGHT_TEMP;
        }
    }

    String screenMsg = "Set graph screen " + String(current_graph_screen);
    debug_outln_info(screenMsg);

    // === HEADER: left icon (graphs), centered title, right time ===
    struct tm timeinfo;
    const uint16_t header_top_y = 6;
    const uint16_t header_row_height = Font16.Height + 2;
    uint16_t header_bottom_border_y = header_top_y + header_row_height + 2;

    // Left: graphs screen icon 
    const uint16_t header_icon_size = 15;
    const uint16_t header_icon_x    = 4;
    const uint16_t header_icon_y    = header_top_y;
    Paint_DrawImage(chart_15x15, header_icon_x, header_icon_y, header_icon_size, header_icon_size);

    // Prepare time on the right
    bool has_time = false;
    char time_buf[8] = {0};
    int time_x = 0;
    if (getLocalTime(&timeinfo)) {
        strftime(time_buf, sizeof(time_buf), "%H:%M", &timeinfo);
        int time_width = (int)Paint_GetStringWidth_Display(time_buf, &Font16, &font_16_cyrillic, &font_16_ascii);
        const int right_margin = 4;
        time_x = DISPLAY_WIDTH - right_margin - time_width;
        has_time = true;
    }

    // Title area: graph title
    const char* graph_title = getGraphTitle(current_graph_value);
    uint16_t title_pixel_width = Paint_GetStringWidth_Display(graph_title, &Font16, &font_16_cyrillic, &font_16_ascii);

    // Start from screen-centered position
    int title_x = (DISPLAY_WIDTH - title_pixel_width) / 2;
    int min_x   = header_icon_x + header_icon_size + 4;
    if (title_x < min_x) {
        title_x = min_x;
    }
    // If we have a time label, ensure the title does not overlap it
    if (has_time) {
        int max_title_right = time_x - 4;
        int title_right = title_x + title_pixel_width;
        if (title_right > max_title_right) {
            title_x -= (title_right - max_title_right);
            if (title_x < min_x) {
                title_x = min_x;
            }
        }
    }
    uint16_t title_y = header_top_y;

    // Draw title
    Paint_DrawString_Display(title_x, title_y, graph_title, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);

    // Right: time (same display font as rest of UI)
    if (has_time) {
        int time_y = header_top_y;
        Paint_DrawString_Display(time_x, time_y, time_buf, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    }

    // Bottom border for header
    Paint_DrawLine(0, header_bottom_border_y, DISPLAY_WIDTH, header_bottom_border_y,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    const uint16_t navBarHeight = 60;
    const uint16_t navTop       = DISPLAY_HEIGHT - navBarHeight;

#if defined(USE_SD_CARD)
    // Check if SD card is available
    if (sd_probe_busy) {
        uint16_t contentTop = header_bottom_border_y + 6;
        graphClearContentArea(contentTop, navTop);
        drawGraphLoadingMessage(navTop, contentTop);
    } else if (!sd_ok) {
        // Center text vertically with proper spacing (3 lines total)
        uint16_t line_spacing = 4;
        uint16_t total_height = Font16.Height + Font12.Height * 2 + line_spacing * 2;
        uint16_t start_y = (navTop - total_height) / 2;  // Account for nav bar at bottom
        
        const char* sd1 = INTL_DISP_SD_NOT_FOUND;
        const char* sd2 = INTL_DISP_INSERT_SD;
        const char* sd3 = INTL_DISP_FAT32_FORMATTED;
        uint16_t x1 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(sd1, &Font16, &font_16_cyrillic, &font_16_ascii) / 2;
        Paint_DrawString_Display(x1, start_y, sd1, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
        uint16_t y2 = start_y + Font16.Height + line_spacing;
        uint16_t x2 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(sd2, &Font12, &font_12_cyrillic, &font_12_ascii) / 2;
        Paint_DrawString_Display(x2, y2, sd2, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        uint16_t y3 = y2 + Font12.Height + line_spacing;
        uint16_t x3 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(sd3, &Font12, &font_12_cyrillic, &font_12_ascii) / 2;
        Paint_DrawString_Display(x3, y3, sd3, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        // Continue to draw nav bar below
    }
    // Check if any data files exist (only if SD card is available)
    else if (!files_ok) {
        uint16_t line_spacing = 4;
        uint16_t total_height = Font16.Height + Font12.Height * 3 + line_spacing * 3;
        uint16_t start_y = (navTop - total_height) / 2;  
        
        const char* nd1 = INTL_DISP_NO_DATA_FILES;
        const char* nd2 = INTL_DISP_DEVICE_WILL_CREATE;
        const char* nd3 = INTL_DISP_FILES_AUTOMATICALLY;
        const char* nd4 = INTL_DISP_AFTER_COLLECTING;
        uint16_t x1 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(nd1, &Font16, &font_16_cyrillic, &font_16_ascii) / 2;
        Paint_DrawString_Display(x1, start_y, nd1, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
        uint16_t y2 = start_y + Font16.Height + line_spacing;
        uint16_t x2 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(nd2, &Font12, &font_12_cyrillic, &font_12_ascii) / 2;
        Paint_DrawString_Display(x2, y2, nd2, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        uint16_t y3 = y2 + Font12.Height + line_spacing;
        uint16_t x3 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(nd3, &Font12, &font_12_cyrillic, &font_12_ascii) / 2;
        Paint_DrawString_Display(x3, y3, nd3, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        uint16_t y4 = y3 + Font12.Height + line_spacing;
        uint16_t x4 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(nd4, &Font12, &font_12_cyrillic, &font_12_ascii) / 2;
        Paint_DrawString_Display(x4, y4, nd4, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        // Continue to draw nav bar below
    }
    else {
    
        String urban_key = ATRUIST_URBAN_SENSOR;
        uint16_t contentTop = header_bottom_border_y + 6;
        drawActiveGraph(current_graph_value, urban_key, navTop, contentTop);
    }
#else
    // SD card not available - show message
    uint16_t line_spacing = 4;
    uint16_t total_height = Font16.Height + Font12.Height * 2 + line_spacing * 2;
    uint16_t start_y = (navTop - total_height) / 2;  // Account for nav bar at bottom
    
    const char* sa1 = INTL_DISP_SD_NOT_AVAILABLE;
    const char* sa2 = INTL_DISP_GRAPHS_REQUIRE_SD;
    const char* sa3 = INTL_DISP_ENABLE_SD;
    uint16_t x1 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(sa1, &Font16, &font_16_cyrillic, &font_16_ascii) / 2;
    Paint_DrawString_Display(x1, start_y, sa1, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    uint16_t y2 = start_y + Font16.Height + line_spacing;
    uint16_t x2 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(sa2, &Font12, &font_12_cyrillic, &font_12_ascii) / 2;
    Paint_DrawString_Display(x2, y2, sa2, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    uint16_t y3 = y2 + Font12.Height + line_spacing;
    uint16_t x3 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(sa3, &Font12, &font_12_cyrillic, &font_12_ascii) / 2;
    Paint_DrawString_Display(x3, y3, sa3, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    // Nav bar will be drawn after #endif (below)
#endif

    // Bottom navigation bar (only drawn if graphs are available)
    if (show_nav_bar) {
        Paint_DrawLine(0, navTop, DISPLAY_WIDTH, navTop, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    const uint16_t paddingX = 6;

    // Icon column: full icon (15x25) centered in column and vertically in nav bar
    const uint16_t iconColWidth = 24;
    uint16_t x = paddingX;
    uint16_t iconAreaRight = x + iconColWidth;
    const uint16_t iconSizeWidth  = 15;
    const uint16_t iconSizeHeight = 25;
    uint16_t iconX = x + (iconColWidth - iconSizeWidth) / 2;
    uint16_t iconY = navTop + navBarHeight / 2 - iconSizeHeight / 2;
    Paint_DrawImage(buttons_nav_15x15, iconX, iconY, iconSizeWidth, iconSizeHeight);

    x = iconAreaRight;
    Paint_DrawLine(x, navTop, x, DISPLAY_HEIGHT - 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    x += 2;

    // Insight vs Urban: full width for Insight when Urban HTTP sensor is disabled.
    uint16_t availableWidth = DISPLAY_WIDTH - x - paddingX;
    uint16_t insightWidth   = cfg::standalone ? availableWidth : (availableWidth * 4) / 9;

    uint16_t sectionHeaderH = Font12.Height + 6;
    uint16_t headerTop      = navTop + 1;

    // --- Insight section ---
    uint16_t insightX       = x;
    uint16_t insightHeaderY = navTop + 1;

    Paint_DrawRectangle(insightX, insightHeaderY,
                        insightX + insightWidth - 2,
                        insightHeaderY + sectionHeaderH,
                        BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_Display_OnBlack(insightX + 2, insightHeaderY + 3, INTL_DISP_INSIGHT_HEADER, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);

    uint16_t insightTextY = insightHeaderY + sectionHeaderH + 4;

    if (cfg::standalone) {
        // Left-aligned row, measured label widths; wide gaps, reduced only if the strip is narrow.
        const uint16_t marginInner = 3;
        const uint16_t innerLeft  = insightX + marginInner;
        const uint16_t innerRight = (uint16_t)(insightX + insightWidth - 1 - marginInner);

        const char* tempLabel  = INTL_DISP_TEMPERATURE;
        const char* humLabel   = INTL_DISP_HUMIDITY;
        const char* co2Label   = INTL_CO2;
        const char* pressLabel = INTL_DISP_PRESSURE;

        uint16_t wTemp  = Paint_GetStringWidth_Display(tempLabel, &Font12, &font_12_cyrillic, &font_12_ascii);
        uint16_t wHum   = Paint_GetStringWidth_Display(humLabel, &Font12, &font_12_cyrillic, &font_12_ascii);
        uint16_t wCo2   = Paint_GetStringWidth_Display(co2Label, &Font12, &font_12_cyrillic, &font_12_ascii);
        uint16_t wPress = Paint_GetStringWidth_Display(pressLabel, &Font12, &font_12_cyrillic, &font_12_ascii);

        // Left-aligned row; prefer a wide gap, shrink only if the strip is too narrow.
        uint16_t gapPx = 10;
        int      avail = (int)innerRight - (int)innerLeft + 1;
        uint32_t packedW =
            (uint32_t)wTemp + (uint32_t)wHum + (uint32_t)wCo2 + (uint32_t)wPress + 3u * (uint32_t)gapPx;
        while (packedW > (uint32_t)avail && gapPx > 1) {
            gapPx--;
            packedW =
                (uint32_t)wTemp + (uint32_t)wHum + (uint32_t)wCo2 + (uint32_t)wPress + 3u * (uint32_t)gapPx;
        }

        const uint16_t rowStart = innerLeft;
        uint16_t       xT       = rowStart;
        uint16_t       xH       = (uint16_t)(xT + wTemp + gapPx);
        uint16_t       xC       = (uint16_t)(xH + wHum + gapPx);
        uint16_t       xP       = (uint16_t)(xC + wCo2 + gapPx);

        auto drawPackedInsight = [&](uint16_t lx, const char* lab, uint16_t lw, GraphValue gv) {
            if ((uint32_t)lx + (uint32_t)lw > (uint32_t)innerRight) {
                lx = (uint16_t)((int)innerRight - (int)lw + 1);
                if (lx < innerLeft) {
                    lx = innerLeft;
                }
            }
            bool active = (current_graph_value == gv);
            if (active) {
                Paint_DrawString_Display(lx, insightTextY, lab, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
                uint16_t cx        = (uint16_t)((uint32_t)lx + (uint32_t)lw / 2u);
                uint16_t arrowTopY = insightTextY + Font12.Height + 1;
                if (arrowTopY + 4 > DISPLAY_HEIGHT - 1) {
                    arrowTopY = DISPLAY_HEIGHT - 1 - 4;
                }
                uint16_t tipY  = arrowTopY;
                uint16_t baseY = arrowTopY + 4;
                Paint_DrawLine(cx, tipY, cx - 3, baseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
                Paint_DrawLine(cx, tipY, cx + 3, baseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
                Paint_DrawLine(cx - 3, baseY, cx + 3, baseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            } else {
                Paint_DrawString_Display(lx, insightTextY, lab, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
            }
        };

        drawPackedInsight(xT, tempLabel, wTemp, GraphValue::INSIGHT_TEMP);
        drawPackedInsight(xH, humLabel, wHum, GraphValue::INSIGHT_HUM);
        drawPackedInsight(xC, co2Label, wCo2, GraphValue::INSIGHT_CO2);
        drawPackedInsight(xP, pressLabel, wPress, GraphValue::INSIGHT_PRESSURE);
    } else {
        uint16_t textX = insightX + 4;

        // Temperature
        const char* tempLabel = INTL_DISP_TEMPERATURE;
        uint16_t    tempWidth = Paint_GetStringWidth_Display(tempLabel, &Font12, &font_12_cyrillic, &font_12_ascii);
        bool        tempActive = (current_graph_value == GraphValue::INSIGHT_TEMP);
        if (tempActive) {
            Paint_DrawString_Display(textX, insightTextY, tempLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
            uint16_t tempCenterX = textX + tempWidth / 2;
            uint16_t tempArrowTopY = insightTextY + Font12.Height + 1;
            if (tempArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
                tempArrowTopY = DISPLAY_HEIGHT - 1 - 4;
            }
            uint16_t tempTipY  = tempArrowTopY;
            uint16_t tempBaseY = tempArrowTopY + 4;
            Paint_DrawLine(tempCenterX, tempTipY, tempCenterX - 3, tempBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(tempCenterX, tempTipY, tempCenterX + 3, tempBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(tempCenterX - 3, tempBaseY, tempCenterX + 3, tempBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        } else {
            Paint_DrawString_Display(textX, insightTextY, tempLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        }

        // Humidity
        textX += tempWidth + Font12.Width;
        const char* humLabel = INTL_DISP_HUMIDITY;
        bool        humActive = (current_graph_value == GraphValue::INSIGHT_HUM);
        if (humActive) {
            Paint_DrawString_Display(textX, insightTextY, humLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
            uint16_t humWidth   = Paint_GetStringWidth_Display(humLabel, &Font12, &font_12_cyrillic, &font_12_ascii);
            uint16_t humCenterX = textX + humWidth / 2;
            uint16_t humArrowTopY = insightTextY + Font12.Height + 1;
            if (humArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
                humArrowTopY = DISPLAY_HEIGHT - 1 - 4;
            }
            uint16_t humTipY  = humArrowTopY;
            uint16_t humBaseY = humArrowTopY + 4;
            Paint_DrawLine(humCenterX, humTipY, humCenterX - 3, humBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(humCenterX, humTipY, humCenterX + 3, humBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(humCenterX - 3, humBaseY, humCenterX + 3, humBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        } else {
            Paint_DrawString_Display(textX, insightTextY, humLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        }

        // CO2
        uint16_t insightTextY2 = insightTextY + Font12.Height + 8;
        const char* co2Label = INTL_CO2;
        uint16_t    co2X = insightX + 4;
        bool        co2Active = (current_graph_value == GraphValue::INSIGHT_CO2);
        if (co2Active) {
            Paint_DrawString_Display(co2X, insightTextY2, co2Label, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
            uint16_t co2Width   = Paint_GetStringWidth_Display(co2Label, &Font12, &font_12_cyrillic, &font_12_ascii);
            uint16_t co2CenterX = co2X + co2Width / 2;
            uint16_t co2ArrowTopY = insightTextY2 + Font12.Height + 1;
            if (co2ArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
                co2ArrowTopY = DISPLAY_HEIGHT - 1 - 4;
            }
            uint16_t co2TipY  = co2ArrowTopY;
            uint16_t co2BaseY = co2ArrowTopY + 4;
            Paint_DrawLine(co2CenterX, co2TipY, co2CenterX - 3, co2BaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(co2CenterX, co2TipY, co2CenterX + 3, co2BaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(co2CenterX - 3, co2BaseY, co2CenterX + 3, co2BaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        } else {
            Paint_DrawString_Display(co2X, insightTextY2, co2Label, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        }

        // Pressure
        const char* pressLabel = INTL_DISP_PRESSURE;
        uint16_t    co2LabelWidth = Paint_GetStringWidth_Display(co2Label, &Font12, &font_12_cyrillic, &font_12_ascii);
        const uint16_t co2PressGap = 2 * Font12.Width;
        uint16_t    pressX = co2X + co2LabelWidth + co2PressGap;
        bool        pressActive = (current_graph_value == GraphValue::INSIGHT_PRESSURE);
        if (pressActive) {
            Paint_DrawString_Display(pressX, insightTextY2, pressLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
            uint16_t pressWidth   = Paint_GetStringWidth_Display(pressLabel, &Font12, &font_12_cyrillic, &font_12_ascii);
            uint16_t pressCenterX = pressX + pressWidth / 2;
            uint16_t pressArrowTopY = insightTextY2 + Font12.Height + 1;
            if (pressArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
                pressArrowTopY = DISPLAY_HEIGHT - 1 - 4;
            }
            uint16_t pressTipY  = pressArrowTopY;
            uint16_t pressBaseY = pressArrowTopY + 4;
            Paint_DrawLine(pressCenterX, pressTipY, pressCenterX - 3, pressBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(pressCenterX, pressTipY, pressCenterX + 3, pressBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(pressCenterX - 3, pressBaseY, pressCenterX + 3, pressBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        } else {
            Paint_DrawString_Display(pressX, insightTextY2, pressLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        }
    }

    if (!cfg::standalone) {
    const uint16_t urbanWidth = availableWidth - insightWidth;
    // --- Urban section ---
    uint16_t urbanX        = insightX + insightWidth;
    uint16_t urbanHeaderY  = navTop + 1;

    // Vertical divider between Insight and Urban sections
    Paint_DrawLine(urbanX - 1, navTop, urbanX - 1, DISPLAY_HEIGHT - 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    Paint_DrawRectangle(urbanX, urbanHeaderY,
                        urbanX + urbanWidth - 2,
                        urbanHeaderY + sectionHeaderH,
                        BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_Display_OnBlack(urbanX + 2, urbanHeaderY + 3, INTL_DISP_URBAN_HEADER, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);

    uint16_t urbanTextY = urbanHeaderY + sectionHeaderH + 4;
    uint16_t uTextX     = urbanX + 4;

    // Air
    const char* airLabel = INTL_DISP_AIR;
    bool        airActive = (current_graph_value == GraphValue::URBAN_AIR);
    if (airActive) {
        Paint_DrawString_Display(uTextX,     urbanTextY, airLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    } else {
        Paint_DrawString_Display(uTextX, urbanTextY, airLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    }
    uint16_t airWidth   = Paint_GetStringWidth_Display(airLabel, &Font12, &font_12_cyrillic, &font_12_ascii);
    uint16_t airCenterX = uTextX + airWidth / 2;
    if (airActive) {
        uint16_t airArrowTopY  = urbanTextY + Font12.Height + 1;
        if (airArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
            airArrowTopY = DISPLAY_HEIGHT - 1 - 4;
        }
        uint16_t airTipY   = airArrowTopY;
        uint16_t airBaseY  = airArrowTopY + 4;
        Paint_DrawLine(airCenterX,     airTipY,  airCenterX - 3, airBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(airCenterX,     airTipY,  airCenterX + 3, airBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(airCenterX - 3, airBaseY, airCenterX + 3, airBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }

    // Noise
    uTextX += airWidth + Font12.Width;
    const char* noiseLabel = INTL_DISP_NOISE;
    uint16_t    noiseWidth = Paint_GetStringWidth_Display(noiseLabel, &Font12, &font_12_cyrillic, &font_12_ascii);
    bool        noiseActive = (current_graph_value == GraphValue::URBAN_NOISE);
    if (noiseActive) {
        Paint_DrawString_Display(uTextX,     urbanTextY, noiseLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        uint16_t noiseCenterX = uTextX + noiseWidth / 2;
        uint16_t noiseArrowTopY  = urbanTextY + Font12.Height + 1;
        if (noiseArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
            noiseArrowTopY = DISPLAY_HEIGHT - 1 - 4;
        }
        uint16_t noiseTipY   = noiseArrowTopY;
        uint16_t noiseBaseY  = noiseArrowTopY + 4;
        Paint_DrawLine(noiseCenterX,     noiseTipY,  noiseCenterX - 3, noiseBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(noiseCenterX,     noiseTipY,  noiseCenterX + 3, noiseBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(noiseCenterX - 3, noiseBaseY, noiseCenterX + 3, noiseBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    } else {
        Paint_DrawString_Display(uTextX, urbanTextY, noiseLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    }
    uTextX += noiseWidth + Font12.Width;
    // Temperature
    const char* urbanTempLabel = INTL_DISP_TEMPERATURE;
    bool        urbanTempActive = (current_graph_value == GraphValue::URBAN_TEMP);
    if (urbanTempActive) {
        Paint_DrawString_Display(uTextX,     urbanTextY, urbanTempLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        uint16_t urbanTempWidth   = Paint_GetStringWidth_Display(urbanTempLabel, &Font12, &font_12_cyrillic, &font_12_ascii);
        uint16_t urbanTempCenterX = uTextX + urbanTempWidth / 2;
        uint16_t urbanTempArrowTopY  = urbanTextY + Font12.Height + 1;
        if (urbanTempArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
            urbanTempArrowTopY = DISPLAY_HEIGHT - 1 - 4;
        }
        uint16_t urbanTempTipY   = urbanTempArrowTopY;
        uint16_t urbanTempBaseY  = urbanTempArrowTopY + 4;
        Paint_DrawLine(urbanTempCenterX,     urbanTempTipY,  urbanTempCenterX - 3, urbanTempBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(urbanTempCenterX,     urbanTempTipY,  urbanTempCenterX + 3, urbanTempBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(urbanTempCenterX - 3, urbanTempBaseY, urbanTempCenterX + 3, urbanTempBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    } else {
        Paint_DrawString_Display(uTextX, urbanTextY, urbanTempLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    }

    // Second row
    uint16_t urbanTextY2 = urbanTextY + Font12.Height + 8;
    // Humidity
    const char* urbanHumLabel = INTL_DISP_HUMIDITY;
    uint16_t    urbanHumX     = urbanX + 4;
    uint16_t    urbanHumWidth = Paint_GetStringWidth_Display(urbanHumLabel, &Font12, &font_12_cyrillic, &font_12_ascii);
    bool        urbanHumActive = (current_graph_value == GraphValue::URBAN_HUM);
    if (urbanHumActive) {
        Paint_DrawString_Display(urbanHumX,     urbanTextY2, urbanHumLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        uint16_t urbanHumCenterX = urbanHumX + urbanHumWidth / 2;
        uint16_t urbanHumArrowTopY  = urbanTextY2 + Font12.Height + 1;
        if (urbanHumArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
            urbanHumArrowTopY = DISPLAY_HEIGHT - 1 - 4;
        }
        uint16_t urbanHumTipY   = urbanHumArrowTopY;
        uint16_t urbanHumBaseY  = urbanHumArrowTopY + 4;
        Paint_DrawLine(urbanHumCenterX,     urbanHumTipY,  urbanHumCenterX - 3, urbanHumBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(urbanHumCenterX,     urbanHumTipY,  urbanHumCenterX + 3, urbanHumBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(urbanHumCenterX - 3, urbanHumBaseY, urbanHumCenterX + 3, urbanHumBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    } else {
        Paint_DrawString_Display(urbanHumX, urbanTextY2, urbanHumLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    }

    // Pressure (gap after Humidity = Font12.Width, same as other label spacing)
    const char* urbanPressLabel = INTL_DISP_PRESSURE;
    uint16_t    urbanPressX     = urbanHumX + urbanHumWidth + Font12.Width;
    bool        urbanPressActive = (current_graph_value == GraphValue::URBAN_PRESSURE);
    if (urbanPressActive) {
        Paint_DrawString_Display(urbanPressX,     urbanTextY2, urbanPressLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        uint16_t urbanPressWidth   = Paint_GetStringWidth_Display(urbanPressLabel, &Font12, &font_12_cyrillic, &font_12_ascii);
        uint16_t urbanPressCenterX = urbanPressX + urbanPressWidth / 2;
        uint16_t urbanPressArrowTopY  = urbanTextY2 + Font12.Height + 1;
        if (urbanPressArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
            urbanPressArrowTopY = DISPLAY_HEIGHT - 1 - 4;
        }
        uint16_t urbanPressTipY   = urbanPressArrowTopY;
        uint16_t urbanPressBaseY  = urbanPressArrowTopY + 4;
        Paint_DrawLine(urbanPressCenterX,     urbanPressTipY,  urbanPressCenterX - 3, urbanPressBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(urbanPressCenterX,     urbanPressTipY,  urbanPressCenterX + 3, urbanPressBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(urbanPressCenterX - 3, urbanPressBaseY, urbanPressCenterX + 3, urbanPressBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    } else {
        Paint_DrawString_Display(urbanPressX, urbanTextY2, urbanPressLabel, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    }
    }
    } 
}

void setNextGraphScreen() {
    if (current_graph_screen < 3) {
        current_graph_screen++;
    } else {
        current_graph_screen = 1;
    }
}

void setPrevGraphScreen() {
    if (current_graph_screen > 1) {
        current_graph_screen--;
    } else {
        current_graph_screen = 3;
    }
}

bool setNextGraphValue() {
    if (cfg::standalone) {
        if (current_graph_value == GraphValue::INSIGHT_PRESSURE) {
            return true;
        }
        switch (current_graph_value) {
            case GraphValue::INSIGHT_TEMP:
                current_graph_value = GraphValue::INSIGHT_HUM;
                break;
            case GraphValue::INSIGHT_HUM:
                current_graph_value = GraphValue::INSIGHT_CO2;
                break;
            case GraphValue::INSIGHT_CO2:
                current_graph_value = GraphValue::INSIGHT_PRESSURE;
                break;
            default:
                current_graph_value = GraphValue::INSIGHT_TEMP;
                break;
        }
        return false;
    }

    // Check if we're at the last graph (URBAN_PRESSURE)
    if (current_graph_value == GraphValue::URBAN_PRESSURE) {
        // At last graph - return true to indicate we should switch screens
        return true;
    }
    
    switch (current_graph_value) {
        case GraphValue::INSIGHT_TEMP:
            current_graph_value = GraphValue::INSIGHT_HUM;
            break;
        case GraphValue::INSIGHT_HUM:
            current_graph_value = GraphValue::INSIGHT_CO2;
            break;
        case GraphValue::INSIGHT_CO2:
            current_graph_value = GraphValue::INSIGHT_PRESSURE;
            break;
        case GraphValue::INSIGHT_PRESSURE:
            current_graph_value = GraphValue::URBAN_AIR;
            break;
        case GraphValue::URBAN_AIR:
            current_graph_value = GraphValue::URBAN_NOISE;
            break;
        case GraphValue::URBAN_NOISE:
            current_graph_value = GraphValue::URBAN_TEMP;
            break;
        case GraphValue::URBAN_TEMP:
            current_graph_value = GraphValue::URBAN_HUM;
            break;
        case GraphValue::URBAN_HUM:
            current_graph_value = GraphValue::URBAN_PRESSURE;
            break;
        case GraphValue::URBAN_PRESSURE:
            // Should not reach here due to check above, but handle it anyway
            return true;
    }
    return false;  // Not at last graph, continue cycling
}

bool setPrevGraphValue() {
    if (cfg::standalone) {
        if (current_graph_value == GraphValue::INSIGHT_TEMP) {
            return true;
        }
        switch (current_graph_value) {
            case GraphValue::INSIGHT_HUM:
                current_graph_value = GraphValue::INSIGHT_TEMP;
                break;
            case GraphValue::INSIGHT_CO2:
                current_graph_value = GraphValue::INSIGHT_HUM;
                break;
            case GraphValue::INSIGHT_PRESSURE:
                current_graph_value = GraphValue::INSIGHT_CO2;
                break;
            default:
                current_graph_value = GraphValue::INSIGHT_TEMP;
                break;
        }
        return false;
    }

    // Check if we're at the first graph 
    if (current_graph_value == GraphValue::INSIGHT_TEMP) {
        // At first graph - return true to indicate we should switch screens
        return true;
    }
    
    switch (current_graph_value) {
        case GraphValue::INSIGHT_TEMP:
            // Should not reach here due to check above, but handle it anyway
            return true;
        case GraphValue::INSIGHT_HUM:
            current_graph_value = GraphValue::INSIGHT_TEMP;
            break;
        case GraphValue::INSIGHT_CO2:
            current_graph_value = GraphValue::INSIGHT_HUM;
            break;
        case GraphValue::INSIGHT_PRESSURE:
            current_graph_value = GraphValue::INSIGHT_CO2;
            break;
        case GraphValue::URBAN_AIR:
            current_graph_value = GraphValue::INSIGHT_PRESSURE;
            break;
        case GraphValue::URBAN_NOISE:
            current_graph_value = GraphValue::URBAN_AIR;
            break;
        case GraphValue::URBAN_TEMP:
            current_graph_value = GraphValue::URBAN_NOISE;
            break;
        case GraphValue::URBAN_HUM:
            current_graph_value = GraphValue::URBAN_TEMP;
            break;
        case GraphValue::URBAN_PRESSURE:
            current_graph_value = GraphValue::URBAN_HUM;
            break;
    }
    return false;  // Not at first graph, continue cycling
}

#endif
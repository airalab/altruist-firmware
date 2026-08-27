#ifdef ALTRUIST_INSIGHT

#include "analytics.h"
#include "../driver/EPD.h"
#include "../paint_driver/GUI_Paint.h"
#include "../icons/icons/icons_15x15.h"
#include "../paint_driver/fonts/fonts.h"
#include "../utils.h"
#include "../../utils.h"
#include "../../defines.h"
#include "../../intl.h"
#include "../../config_manager/config_helpers.h"
#include <qrcode.h>
#if defined(USE_SD_CARD) && defined(ALTRUIST_BUILD_DEBUG)
#include "../../sd_card/sd_card.h"
extern SDCard sdCardLogger;
#endif
#include <Preferences.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

namespace {
constexpr float kNoTempData = -1000.0f;
constexpr float kNoData = -1.0f;
constexpr float kEpsilon = 0.1f;
constexpr float kPi = 3.14159265f;
analytics_view_t g_analytics_view = analytics_view_t::OVERVIEW_24H;

#if defined(INTL_RU)
#define A_TXT(en, ru) ru
#else
#define A_TXT(en, ru) en
#endif

struct RollingHourBucket {
    uint32_t hour_key = 0; // local hour key (day_key*24 + hour)
    float sum_v = 0.0f;
    float max_v = 0.0f;    // peak sample in this hour (used for noise >45 dB)
    uint16_t count = 0;
};

struct RollingHourHistory {
    RollingHourBucket buckets[48]; // keep ~2 days of hourly bins
};

RollingHourHistory g_temp_hour_hist;
RollingHourHistory g_hum_hour_hist;
RollingHourHistory g_dew_hour_hist;
RollingHourHistory g_pm25_hour_hist;
RollingHourHistory g_co2_hour_hist;
RollingHourHistory g_noise_hour_hist;

constexpr uint8_t kAnalyticsHistVersion = 5;
constexpr uint8_t kAnalyticsHistVersionPrev = 4; // pre-max_v buckets
constexpr float kNoisePeakThresholdDb = 45.0f;
constexpr uint32_t kAnalyticsPersistIntervalMs = 60UL * 60UL * 1000UL; // 1 hour
bool g_hist_loaded = false;
bool g_hist_dirty = false;
uint32_t g_hist_last_save_ms = 0;
uint32_t g_hist_last_day_key = 0;
uint32_t g_hist_first_save_ts = 0;
uint32_t g_hist_last_save_ts = 0;
uint32_t g_hist_save_count = 0;
bool g_hist_last_save_forced = false;
#if defined(USE_SD_CARD) && defined(ALTRUIST_BUILD_DEBUG)
uint32_t g_dev_sd_last_dump_hour_key = 0;
uint32_t g_dev_last_ingest_hour_key = 0;
static void dumpAnalyticsToSdDev(uint32_t now_ts, bool forced_save);
#endif
#if defined(ALTRUIST_BUILD_DEBUG)
uint32_t g_dev_last_status_log_ms = 0;
#endif
static uint32_t currentLocalHourKey(time_t now);
static void formatHourKey(char *out, size_t out_sz, uint32_t hour_key);
static void saveRollingHistoryIfNeeded(bool force);

/** v4 bucket layout (no max_v). Natural align → 12 bytes on ESP32. */
struct RollingHourBucketV4 {
    uint32_t hour_key = 0;
    float sum_v = 0.0f;
    uint16_t count = 0;
};
struct RollingHourHistoryV4 {
    RollingHourBucketV4 buckets[48];
};
static_assert(sizeof(RollingHourBucketV4) == 12, "v4 analytics bucket size");
static_assert(sizeof(RollingHourBucket) == 16, "v5 analytics bucket size");

static void migrateHourHistoryV4ToV5(RollingHourHistory &dst, const RollingHourHistoryV4 &src) {
    for (int i = 0; i < 48; i++) {
        const RollingHourBucketV4 &o = src.buckets[i];
        RollingHourBucket &n = dst.buckets[i];
        n.hour_key = o.hour_key;
        n.sum_v = o.sum_v;
        n.count = o.count;
        // No true peak stored in v4 — use hourly average as provisional max until
        // fresh samples overwrite (noise peaks may under-count for one night).
        n.max_v = (o.count > 0) ? (o.sum_v / (float)o.count) : 0.0f;
    }
}

static bool loadHourHistoryV5(Preferences &prefs, const char *key, RollingHourHistory &hist) {
    if (prefs.getBytesLength(key) != sizeof(hist)) return false;
    prefs.getBytes(key, &hist, sizeof(hist));
    return true;
}

static bool loadHourHistoryMigratingV4(Preferences &prefs, const char *key, RollingHourHistory &hist) {
    RollingHourHistoryV4 old{};
    if (prefs.getBytesLength(key) != sizeof(old)) return false;
    prefs.getBytes(key, &old, sizeof(old));
    migrateHourHistoryV4ToV5(hist, old);
    return true;
}

static void loadRollingHistoryIfNeeded() {
    if (g_hist_loaded) return;
    g_hist_loaded = true;

    Preferences prefs;
    // Use read-write open here so first boot can create namespace without noisy NOT_FOUND logs.
    if (!prefs.begin("analytics", false)) {
        return;
    }

    const uint8_t version = prefs.getUChar("ver", 0);
    if (version != kAnalyticsHistVersion && version != kAnalyticsHistVersionPrev) {
        prefs.end();
        return;
    }
    g_hist_first_save_ts = prefs.getULong("fst_ts", 0);
    g_hist_last_save_ts = prefs.getULong("lst_ts", 0);

    bool migrated = false;
    if (version == kAnalyticsHistVersion) {
        loadHourHistoryV5(prefs, "h_temp", g_temp_hour_hist);
        loadHourHistoryV5(prefs, "h_hum", g_hum_hour_hist);
        loadHourHistoryV5(prefs, "h_dew", g_dew_hour_hist);
        loadHourHistoryV5(prefs, "h_pm25", g_pm25_hour_hist);
        loadHourHistoryV5(prefs, "h_co2", g_co2_hour_hist);
        loadHourHistoryV5(prefs, "h_noise", g_noise_hour_hist);
    } else {
        // v4 → v5: keep averages; fill max_v from avg (provisional for noise peaks).
        migrated |= loadHourHistoryMigratingV4(prefs, "h_temp", g_temp_hour_hist);
        migrated |= loadHourHistoryMigratingV4(prefs, "h_hum", g_hum_hour_hist);
        migrated |= loadHourHistoryMigratingV4(prefs, "h_dew", g_dew_hour_hist);
        migrated |= loadHourHistoryMigratingV4(prefs, "h_pm25", g_pm25_hour_hist);
        migrated |= loadHourHistoryMigratingV4(prefs, "h_co2", g_co2_hour_hist);
        migrated |= loadHourHistoryMigratingV4(prefs, "h_noise", g_noise_hour_hist);
    }
    prefs.end();

    if (migrated) {
        g_hist_dirty = true;
        saveRollingHistoryIfNeeded(true);
#if defined(ALTRUIST_BUILD_DEBUG)
        debug_outln_info(F("[ANALYTICS] migrated hour history v4→v5"));
#endif
    }
}

static void saveRollingHistoryIfNeeded(bool force) {
    if (!g_hist_loaded || !g_hist_dirty) return;

    const uint32_t now_ms = millis();
    // After reboot, g_hist_last_save_ms is 0. Do not throttle the first save in this boot.
    if (!force && g_hist_last_save_ms != 0U &&
        (now_ms - g_hist_last_save_ms) < kAnalyticsPersistIntervalMs) {
        return;
    }

    bool persisted = false;
    const uint32_t now_ts = (uint32_t)time(nullptr);
    Preferences prefs;
    if (prefs.begin("analytics", false)) {
        prefs.putUChar("ver", kAnalyticsHistVersion);
        if (now_ts > 0 && g_hist_first_save_ts == 0) {
            g_hist_first_save_ts = now_ts;
        }
        if (now_ts > 0) {
            g_hist_last_save_ts = now_ts;
        }
        prefs.putULong("fst_ts", g_hist_first_save_ts);
        prefs.putULong("lst_ts", g_hist_last_save_ts);
        prefs.putBytes("h_temp", &g_temp_hour_hist, sizeof(g_temp_hour_hist));
        prefs.putBytes("h_hum", &g_hum_hour_hist, sizeof(g_hum_hour_hist));
        prefs.putBytes("h_dew", &g_dew_hour_hist, sizeof(g_dew_hour_hist));
        prefs.putBytes("h_pm25", &g_pm25_hour_hist, sizeof(g_pm25_hour_hist));
        prefs.putBytes("h_co2", &g_co2_hour_hist, sizeof(g_co2_hour_hist));
        prefs.putBytes("h_noise", &g_noise_hour_hist, sizeof(g_noise_hour_hist));
        prefs.end();
        persisted = true;
    }

    if (!persisted) return;
#if defined(USE_SD_CARD) && defined(ALTRUIST_BUILD_DEBUG)
    dumpAnalyticsToSdDev(now_ts, force);
#endif
    g_hist_dirty = false;
    g_hist_last_save_ms = now_ms;
    g_hist_save_count++;
    g_hist_last_save_forced = force;
#if defined(ALTRUIST_BUILD_DEBUG)
    const uint32_t saved_hour_key = currentLocalHourKey((time_t)now_ts);
    char saved_hour_buf[24];
    formatHourKey(saved_hour_buf, sizeof(saved_hour_buf), saved_hour_key);
    debug_outln_info(F("[ANALYTICS][DEV] persisted NVS hour_key"), String(saved_hour_key));
    debug_outln_info(F("[ANALYTICS][DEV] persisted NVS local hour"), String(saved_hour_buf));
    debug_outln_info(F("[ANALYTICS][DEV] persisted reason"), String(force ? "day-change" : "interval"));
#endif
}

static bool rollingHistoryHasAnyData(const RollingHourHistory &hist) {
    for (int i = 0; i < 48; i++) {
        if (hist.buckets[i].count > 0) return true;
    }
    return false;
}

static uint8_t rollingHistoryEntryCount(const RollingHourHistory &hist) {
    uint8_t n = 0;
    for (int i = 0; i < 48; i++) {
        if (hist.buckets[i].count > 0) n++;
    }
    return n;
}

static bool rollingHistoryLastEntry(const RollingHourHistory &hist, uint32_t &hour_key, float &avg_v, uint16_t &count) {
    bool found = false;
    uint32_t best = 0;
    for (int i = 0; i < 48; i++) {
        const RollingHourBucket &b = hist.buckets[i];
        if (b.count == 0) continue;
        if (!found || b.hour_key > best) {
            found = true;
            best = b.hour_key;
            avg_v = b.sum_v / (float)b.count;
            count = b.count;
        }
    }
    hour_key = best;
    return found;
}

static void formatLogTs(char *out, size_t out_sz, uint32_t ts) {
    if (!out || out_sz == 0) return;
    if (ts == 0) {
        snprintf(out, out_sz, "-");
        return;
    }
    time_t t = (time_t)ts;
    struct tm tm_v;
    localtime_r(&t, &tm_v);
    strftime(out, out_sz, "%Y-%m-%d %H:%M:%S", &tm_v);
}

static bool isValidTemp(float v) {
    return (v > -40.0f && v < 80.0f && fabsf(v - kNoTempData) > kEpsilon);
}

static bool isValidHumidity(float v) {
    return (v >= 0.0f && v <= 100.0f && fabsf(v - kNoData) > kEpsilon);
}

static bool isValidCO2(float v) {
    return (v >= 150.0f && fabsf(v - kNoData) > kEpsilon);
}

static bool isValidPM25(float v) {
    return (v >= 0.0f && v <= 800.0f && fabsf(v - kNoData) > kEpsilon);
}

static bool isValidPM10(float v) {
    return (v >= 0.0f && v <= 1500.0f && fabsf(v - kNoData) > kEpsilon);
}

static bool isValidNoise(float v) {
    return (v >= 0.0f && v <= 120.0f && fabsf(v - kNoData) > kEpsilon);
}

// Civil-date day serial (Howard Hinnant's days_from_civil).
// Returns a key that increments by 1 each local calendar day.
static int32_t daysFromCivil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);                   // [0, 399]
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; // [0, 365]
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;       // [0, 146096]
    return era * 146097 + (int)doe - 719468; // 1970-01-01 => 0
}

static uint32_t currentLocalDayKey(time_t now) {
    struct tm local_tm;
    localtime_r(&now, &local_tm);
    const int y = local_tm.tm_year + 1900;
    const unsigned m = (unsigned)local_tm.tm_mon + 1U;
    const unsigned d = (unsigned)local_tm.tm_mday;
    const int32_t key = daysFromCivil(y, m, d);
    return key < 0 ? 0U : (uint32_t)key;
}

static uint32_t currentLocalHourKey(time_t now) {
    struct tm local_tm;
    localtime_r(&now, &local_tm);
    const uint32_t day_key = currentLocalDayKey(now);
    return day_key * 24U + (uint32_t)local_tm.tm_hour;
}

static bool shouldPersistForCurrentHour(time_t now, uint32_t current_hour_key) {
    if (now <= 0) return false;
    if (g_hist_last_save_ts == 0U) return true; // first save after boot / empty history

    const uint32_t last_saved_hour_key = currentLocalHourKey((time_t)g_hist_last_save_ts);
    if (last_saved_hour_key == current_hour_key) return false; // already persisted this hour

    struct tm local_tm;
    localtime_r(&now, &local_tm);
    const bool near_hour_start = (local_tm.tm_min <= 10);
    const bool overdue = ((uint32_t)now > g_hist_last_save_ts) &&
                         (((uint32_t)now - g_hist_last_save_ts) >= 60U * 60U);

    // Prefer saves near the top of the hour, but if we missed the window/rebooted,
    // persist as soon as we are overdue.
    return near_hour_start || overdue;
}

// Inverse of daysFromCivil (Howard Hinnant's civil_from_days).
static void civilFromDays(int32_t z, int &y, unsigned &m, unsigned &d) {
    z += 719468;
    const int era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = (unsigned)(z - era * 146097);                    // [0, 146096]
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
    y = (int)yoe + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);         // [0, 365]
    const unsigned mp = (5 * doy + 2) / 153;                              // [0, 11]
    d = doy - (153 * mp + 2) / 5 + 1;                                     // [1, 31]
    m = mp + (mp < 10 ? 3 : -9);                                          // [1, 12]
    y += (m <= 2);
}

static void formatHourKey(char *out, size_t out_sz, uint32_t hour_key) {
    if (!out || out_sz == 0) return;
    const int32_t day_key = (int32_t)(hour_key / 24U);
    const unsigned hour = (unsigned)(hour_key % 24U);
    int y = 0;
    unsigned m = 1, d = 1;
    civilFromDays(day_key, y, m, d);
    snprintf(out, out_sz, "%04d-%02u-%02u %02u:00", y, m, d, hour);
}

#if defined(USE_SD_CARD) && defined(ALTRUIST_BUILD_DEBUG)
static bool ensureAnalyticsDevSdDir() {
    if (!sdCardLock(1500)) return false;
    bool ok = true;
    if (!SD.exists(ROLLUP_ROOT_FOLDER)) ok = SD.mkdir(ROLLUP_ROOT_FOLDER);
    if (ok && !SD.exists("/sensors_rollup/dev")) ok = SD.mkdir("/sensors_rollup/dev");
    if (ok && !SD.exists("/sensors_rollup/dev/analytics")) ok = SD.mkdir("/sensors_rollup/dev/analytics");
    sdCardUnlock();
    return ok;
}

static void appendHourlyDump(String &out, const char *name, const RollingHourHistory &hist) {
    out += "\n[hourly:";
    out += name;
    out += "]\n";
    out += "hour_key,day_key,hour,avg,max,count\n";
    for (int i = 0; i < 48; i++) {
        const RollingHourBucket &b = hist.buckets[i];
        if (b.count == 0) continue;
        const uint32_t day_key = b.hour_key / 24U;
        const uint8_t hour = (uint8_t)(b.hour_key % 24U);
        out += String((uint32_t)b.hour_key);
        out += ",";
        out += String(day_key);
        out += ",";
        out += String((uint32_t)hour);
        out += ",";
        out += String((b.count > 0) ? (b.sum_v / (float)b.count) : 0.0f, 3);
        out += ",";
        out += String(b.max_v, 3);
        out += ",";
        out += String((uint32_t)b.count);
        out += "\n";
    }
}

static void dumpAnalyticsToSdDev(uint32_t now_ts, bool forced_save) {
    if (!sdCardLogger.checkInserted()) return;
    if (!ensureAnalyticsDevSdDir()) return;

    const uint32_t hour_key = (now_ts > 0)
        ? currentLocalHourKey((time_t)now_ts)
        : currentLocalHourKey(time(nullptr));
    if (!forced_save && g_dev_sd_last_dump_hour_key == hour_key) return;

    String content;
    content.reserve(8192);
    content += "ts=";
    content += String((uint32_t)now_ts);
    content += "\n";
    content += "hour_key=";
    content += String((uint32_t)hour_key);
    content += "\n";
    content += "reason=";
    content += forced_save ? "day-change" : "interval";
    content += "\n";

    appendHourlyDump(content, "temp", g_temp_hour_hist);
    appendHourlyDump(content, "hum", g_hum_hour_hist);
    appendHourlyDump(content, "dew", g_dew_hour_hist);
    appendHourlyDump(content, "pm25", g_pm25_hour_hist);
    appendHourlyDump(content, "co2", g_co2_hour_hist);
    appendHourlyDump(content, "noise", g_noise_hour_hist);

    const String path = String("/sensors_rollup/dev/analytics/analytics_") + String((uint32_t)hour_key) + ".txt";
    if (sdCardLogger.writeTextFile(path, content)) {
        g_dev_sd_last_dump_hour_key = hour_key;
    }
}
#endif

static void updateRollingHourMetric(RollingHourHistory &hist, float value, uint32_t hour_key) {
    const uint32_t idx = hour_key % 48U;
    RollingHourBucket &b = hist.buckets[idx];
    if (b.count == 0 || b.hour_key != hour_key) {
        b.hour_key = hour_key;
        b.sum_v = value;
        b.max_v = value;
        b.count = 1;
        return;
    }
    b.sum_v += value;
    if (value > b.max_v) {
        b.max_v = value;
    }
    b.count++;
}

static bool applyHourlyMedianForDay(analytics_metric_t &metric, const RollingHourHistory &hist, uint32_t day_key) {
    float vals[24];
    int n = 0;
    for (int i = 0; i < 48; i++) {
        const RollingHourBucket &b = hist.buckets[i];
        if (b.count == 0) continue;
        if ((b.hour_key / 24U) != day_key) continue;
        vals[n++] = b.sum_v / (float)b.count;
        if (n >= 24) break;
    }
    if (n == 0) return false;

    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (vals[j] < vals[i]) {
                float t = vals[i];
                vals[i] = vals[j];
                vals[j] = t;
            }
        }
    }
    float med = (n & 1) ? vals[n / 2] : (vals[n / 2 - 1] + vals[n / 2]) * 0.5f;
    if (!metric.has_24h) {
        metric.min24h = med;
        metric.max24h = med;
        metric.has_24h = true;
    }
    metric.avg24h = med; // robust "typical day" value
    return true;
}

static bool readHourlyDayValuesFromHistory(const RollingHourHistory &hist, uint32_t day_key, float vals[24], bool has[24]) {
    for (int i = 0; i < 24; i++) {
        vals[i] = 0.0f;
        has[i] = false;
    }
    bool any = false;
    for (int i = 0; i < 48; i++) {
        const RollingHourBucket &b = hist.buckets[i];
        if (b.count == 0) continue;
        if ((b.hour_key / 24U) != day_key) continue;
        const uint8_t h = (uint8_t)(b.hour_key % 24U);
        vals[h] = b.sum_v / (float)b.count;
        has[h] = true;
        any = true;
    }
    return any;
}

/** Per-hour peak (max_v) for the given local day — used for noise peak counting. */
static bool readHourlyDayMaxFromHistory(const RollingHourHistory &hist, uint32_t day_key, float vals[24], bool has[24]) {
    for (int i = 0; i < 24; i++) {
        vals[i] = 0.0f;
        has[i] = false;
    }
    bool any = false;
    for (int i = 0; i < 48; i++) {
        const RollingHourBucket &b = hist.buckets[i];
        if (b.count == 0) continue;
        if ((b.hour_key / 24U) != day_key) continue;
        const uint8_t h = (uint8_t)(b.hour_key % 24U);
        vals[h] = b.max_v;
        has[h] = true;
        any = true;
    }
    return any;
}

static float computeDewPoint(float temp_c, float humidity_pct) {
    if (!isValidTemp(temp_c) || !isValidHumidity(humidity_pct) || humidity_pct <= 0.0f) {
        return kNoTempData;
    }
    const float a = 17.62f;
    const float b = 243.12f;
    const float gamma = logf(humidity_pct / 100.0f) + (a * temp_c) / (b + temp_c);
    return (b * gamma) / (a - gamma);
}

static float clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static float normalizeDeg(float deg) {
    while (deg < 0.0f) deg += 360.0f;
    while (deg >= 360.0f) deg -= 360.0f;
    return deg;
}

static bool angleInSweep(float angle_deg, float start_deg, float sweep_deg) {
    const float sweep = fabsf(sweep_deg);
    if (sweep >= 359.9f) return true;
    const float a = normalizeDeg(angle_deg);
    const float s = normalizeDeg(start_deg);
    if (sweep_deg >= 0.0f) {
        const float d = normalizeDeg(a - s);
        return d <= sweep;
    }
    const float d = normalizeDeg(s - a);
    return d <= sweep;
}

static void drawArcBandColor(int cx, int cy, int outer_r, int inner_r,
                             float start_deg, float sweep_deg, uint16_t color) {
    if (outer_r < inner_r) {
        int t = outer_r;
        outer_r = inner_r;
        inner_r = t;
    }
    if (outer_r <= 0) return;
    if (fabsf(sweep_deg) < 0.01f) return;

    const int o2 = outer_r * outer_r;
    const int i2 = inner_r * inner_r;
    const int x0 = cx - outer_r;
    const int x1 = cx + outer_r;
    const int y0 = cy - outer_r;
    const int y1 = cy + outer_r;

    // Pixel-accurate mask fill: no overdraw patterns, cleaner on e-ink.
    for (int y = y0; y <= y1; ++y) {
        const int dy = y - cy;
        for (int x = x0; x <= x1; ++x) {
            const int dx = x - cx;
            const int r2 = dx * dx + dy * dy;
            if (r2 > o2 || r2 < i2) continue;
            const float a = atan2f((float)dy, (float)dx) * (180.0f / kPi);
            if (!angleInSweep(a, start_deg, sweep_deg)) continue;
            Paint_DrawPoint(x, y, color, DOT_PIXEL_1X1, DOT_STYLE_DFT);
        }
    }
}

static void drawDashedRingBorder(int cx, int cy, int outer_r, int inner_r,
                                 float start_deg, float sweep_deg,
                                 float dash_deg, float gap_deg) {
    if (outer_r < inner_r) {
        const int t = outer_r;
        outer_r = inner_r;
        inner_r = t;
    }
    if (dash_deg <= 0.0f) return;
    if (gap_deg < 0.0f) gap_deg = 0.0f;
    const float total = fabsf(sweep_deg);
    if (total < 0.01f) return;
    const float cycle = dash_deg + gap_deg;
    if (cycle <= 0.0f) return;
    const float dir = (sweep_deg >= 0.0f) ? 1.0f : -1.0f;
    const int n = (int)ceilf(total / cycle);
    for (int i = 0; i < n; ++i) {
        const float prog = (float)i * cycle;
        const float rem = total - prog;
        if (rem <= 0.0f) break;
        const float seg = (rem < dash_deg) ? rem : dash_deg;
        const float a0 = start_deg + dir * prog;
        drawArcBandColor(cx, cy, outer_r, inner_r, a0, dir * seg, BLACK);
    }
}

static void fillRingSolidFast(int cx, int cy, int outer_r, int inner_r, uint16_t color) {
    if (outer_r < inner_r) {
        const int t = outer_r;
        outer_r = inner_r;
        inner_r = t;
    }
    drawArcBandColor(cx, cy, outer_r, inner_r, -90.0f, 360.0f, color);
}

static void drawArcBand(int cx, int cy, int outer_r, int inner_r,
                        float start_deg, float sweep_deg, float normalized) {
    const float fill = clamp01(normalized);
    if (fill <= 0.0f) return;
    drawArcBandColor(cx, cy, outer_r, inner_r, start_deg, sweep_deg * fill, BLACK);
}

static void drawRingBorders(int cx, int cy, int outer_r, int inner_r) {
    Paint_DrawCircle(cx, cy, outer_r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    if (outer_r > 2) {
        Paint_DrawCircle(cx, cy, outer_r - 1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    }
    Paint_DrawCircle(cx, cy, inner_r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(cx, cy, inner_r + 1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
}

static int metricScoreLinearHighWorse(float value, float good_limit, float bad_limit) {
    if (value <= good_limit) return 100;
    if (value >= bad_limit) return 0;
    float t = (value - good_limit) / (bad_limit - good_limit);
    return (int)(100.0f * (1.0f - t));
}

static int metricScoreRangeBest(float value, float best_low, float best_high, float hard_low, float hard_high) {
    if (value <= hard_low || value >= hard_high) return 0;
    if (value >= best_low && value <= best_high) return 100;
    if (value < best_low) {
        float t = (value - hard_low) / (best_low - hard_low);
        return (int)(100.0f * t);
    }
    float t = (hard_high - value) / (hard_high - best_high);
    return (int)(100.0f * t);
}

} // namespace

static String g_analytics_qr_url;
static unsigned char *g_analytics_qr_bmp = nullptr;
static int g_analytics_qr_w = 0;
static int g_analytics_qr_h = 0;

static constexpr char ANALYTICS_SLEEP_BLOG_URL[] =
    "https://sensors.social/blog/insight-sleeping-analytics";

bool analyticsHistoryPersistenceEnabled() {
    return true;
}

bool analyticsHistoryIsLoaded() {
    loadRollingHistoryIfNeeded();
    return g_hist_loaded;
}

bool analyticsHistoryHasData() {
    loadRollingHistoryIfNeeded();
    const bool indoor = rollingHistoryHasAnyData(g_temp_hour_hist) ||
                        rollingHistoryHasAnyData(g_hum_hour_hist) ||
                        rollingHistoryHasAnyData(g_dew_hour_hist) ||
                        rollingHistoryHasAnyData(g_co2_hour_hist);
    if (cfg::standalone) {
        return indoor || rollingHistoryHasAnyData(g_pm25_hour_hist);
    }
    // Paired Insight has no local PM; PM/noise in sleep analytics only when Urban is enabled.
    if (cfg::analytics_sleep_add_urban) {
        return indoor || rollingHistoryHasAnyData(g_pm25_hour_hist) ||
               rollingHistoryHasAnyData(g_noise_hour_hist);
    }
    return indoor;
}

analytics_view_t analyticsGetView() {
    return g_analytics_view;
}

bool analyticsNextViewAtEdge() {
    g_analytics_view = analytics_view_t::OVERVIEW_24H;
    return true;
}

bool analyticsPrevViewAtEdge() {
    g_analytics_view = analytics_view_t::OVERVIEW_24H;
    return true;
}

const char* analyticsViewLabel() {
    return A_TXT("Night", "Ночь");
}

static const char* analyticsViewLabelTitle() {
    return A_TXT("Sleep analytics", "Аналитика сна");
}

void extractAnalyticsScreenValues(const DynamicJsonDocument &doc, analytics_screen_values_t &values) {
    loadRollingHistoryIfNeeded();

    values = analytics_screen_values_t{};
    JsonObjectConst data = doc.as<JsonObjectConst>();

    updateMetrics();
    bool use_bme680_for_temp_hum = (system_metrics.uptime_sec < 360);

    if (data.containsKey("SCD4x")) {
        JsonObjectConst scd = data["SCD4x"].as<JsonObjectConst>();
        if (scd.containsKey("co2")) {
            float co2 = scd["co2"]["value"].as<float>();
            if (isValidCO2(co2)) {
                values.co2.current = co2;
                values.co2.has_current = true;
            }
        }
        if (!use_bme680_for_temp_hum) {
            if (scd.containsKey("temperature")) {
                float t = scd["temperature"]["value"].as<float>();
                if (isValidTemp(t)) {
                    values.temp_indoor.current = t;
                    values.temp_indoor.has_current = true;
                }
            }
            if (scd.containsKey("humidity")) {
                float h = scd["humidity"]["value"].as<float>();
                if (isValidHumidity(h)) {
                    values.hum_indoor.current = h;
                    values.hum_indoor.has_current = true;
                }
            }
        }
    }

    if (data.containsKey("BME680")) {
        JsonObjectConst bme = data["BME680"].as<JsonObjectConst>();
        if (use_bme680_for_temp_hum) {
            if (bme.containsKey("temperature")) {
                float t = bme["temperature"]["value"].as<float>();
                if (isValidTemp(t)) {
                    values.temp_indoor.current = t;
                    values.temp_indoor.has_current = true;
                }
            }
            if (bme.containsKey("humidity")) {
                float h = bme["humidity"]["value"].as<float>();
                if (isValidHumidity(h)) {
                    values.hum_indoor.current = h;
                    values.hum_indoor.has_current = true;
                }
            }
        }
    }

    if (!cfg::standalone) {
        const char* urban_key = ATRUIST_URBAN_SENSOR;
        if (data.containsKey(urban_key)) {
            JsonObjectConst urban = data[urban_key].as<JsonObjectConst>();
            if (urban.containsKey("BME280_temperature")) {
                float t_urban = urban["BME280_temperature"]["value"].as<float>();
                if (isValidTemp(t_urban)) {
                    values.temp_urban.current = t_urban;
                    values.temp_urban.has_current = true;
                }
            }
            if (urban.containsKey("BME280_humidity")) {
                float h_urban = urban["BME280_humidity"]["value"].as<float>();
                if (isValidHumidity(h_urban)) {
                    values.hum_urban.current = h_urban;
                    values.hum_urban.has_current = true;
                }
            }
            if (urban.containsKey("SDS_P2")) {
                float pm25 = urban["SDS_P2"]["value"].as<float>();
                if (isValidPM25(pm25)) {
                    values.pm25.current = pm25;
                    values.pm25.has_current = true;
                }
            }
            if (urban.containsKey("PCBA_noiseAvg")) {
                float noise = urban["PCBA_noiseAvg"]["value"].as<float>();
                if (isValidNoise(noise)) {
                    values.noise_avg.current = noise;
                    values.noise_avg.has_current = true;
                }
            }
            if (urban.containsKey("PCBA_noiseMax")) {
                float noise_max = urban["PCBA_noiseMax"]["value"].as<float>();
                if (isValidNoise(noise_max)) {
                    values.noise_max.current = noise_max;
                    values.noise_max.has_current = true;
                }
            }
        }
    }

    // Insight local PM values (if present in local SDS011 payload).
    if (data.containsKey("SDS011")) {
        JsonObjectConst sds = data["SDS011"].as<JsonObjectConst>();
        if (sds.containsKey("P1")) {
            float pm10_i = sds["P1"]["value"].as<float>();
            if (isValidPM10(pm10_i)) {
                values.pm10_insight.current = pm10_i;
                values.pm10_insight.has_current = true;
            }
        }
        if (sds.containsKey("P2")) {
            float pm25_i = sds["P2"]["value"].as<float>();
            if (isValidPM25(pm25_i)) {
                values.pm25_insight.current = pm25_i;
                values.pm25_insight.has_current = true;
            }
        }
    }

    if (values.temp_indoor.has_current && values.hum_indoor.has_current) {
        float dew = computeDewPoint(values.temp_indoor.current, values.hum_indoor.current);
        if (isValidTemp(dew)) {
            values.dew_indoor.current = dew;
            values.dew_indoor.has_current = true;
        }
    }

}

void analyticsIngestHourSample(const analytics_screen_values_t &values) {
    loadRollingHistoryIfNeeded();

    // Update low-memory rolling aggregates (48 hour buckets).
    time_t now = time(nullptr);
    bool updated = false;
    if (now > 0) {
        uint32_t day_key = currentLocalDayKey(now);
        uint32_t hour_key = currentLocalHourKey(now);
        uint8_t updated_metrics = 0;
        if (values.temp_indoor.has_current) {
            updateRollingHourMetric(g_temp_hour_hist, values.temp_indoor.current, hour_key);
            updated = true;
            updated_metrics++;
        }
        if (values.hum_indoor.has_current) {
            updateRollingHourMetric(g_hum_hour_hist, values.hum_indoor.current, hour_key);
            updated = true;
            updated_metrics++;
        }
        if (values.dew_indoor.has_current) {
            updateRollingHourMetric(g_dew_hour_hist, values.dew_indoor.current, hour_key);
            updated = true;
            updated_metrics++;
        }
        if (!cfg::standalone) {
            if (cfg::analytics_sleep_add_urban) {
                if (values.pm25.has_current) {
                    updateRollingHourMetric(g_pm25_hour_hist, values.pm25.current, hour_key);
                    updated = true;
                    updated_metrics++;
                }
                if (values.noise_max.has_current) {
                    // Peak model uses hourly max of Urban noiseMax samples.
                    updateRollingHourMetric(g_noise_hour_hist, values.noise_max.current, hour_key);
                    updated = true;
                    updated_metrics++;
                } else if (values.noise_avg.has_current) {
                    updateRollingHourMetric(g_noise_hour_hist, values.noise_avg.current, hour_key);
                    updated = true;
                    updated_metrics++;
                }
            }
        } else if (values.pm25_insight.has_current) {
            updateRollingHourMetric(g_pm25_hour_hist, values.pm25_insight.current, hour_key);
            updated = true;
            updated_metrics++;
        }
        if (values.co2.has_current) {
            updateRollingHourMetric(g_co2_hour_hist, values.co2.current, hour_key);
            updated = true;
            updated_metrics++;
        }

        if (updated) {
            g_hist_dirty = true;
            const bool day_changed = (g_hist_last_day_key != 0U && g_hist_last_day_key != day_key);
            g_hist_last_day_key = day_key;
            const bool hour_persist_due = shouldPersistForCurrentHour(now, hour_key);
            saveRollingHistoryIfNeeded(day_changed || hour_persist_due);
#if defined(ALTRUIST_BUILD_DEBUG)
            if (g_dev_last_ingest_hour_key != hour_key) {
                char ingest_hour_buf[24];
                formatHourKey(ingest_hour_buf, sizeof(ingest_hour_buf), hour_key);
                g_dev_last_ingest_hour_key = hour_key;
                debug_outln_info(F("[ANALYTICS][DEV] hourly ingest hour_key"), String(hour_key));
                debug_outln_info(F("[ANALYTICS][DEV] hourly ingest local hour"), String(ingest_hour_buf));
                debug_outln_info(F("[ANALYTICS][DEV] hourly ingest metrics count"), String((int)updated_metrics));
                debug_outln_info(F("[ANALYTICS][DEV] save count"), String((unsigned)g_hist_save_count));
                debug_outln_info(F("[ANALYTICS][DEV] save forced"), String(g_hist_last_save_forced ? "yes" : "no"));
                debug_outln_info(F("[ANALYTICS][DEV] save due hour"), String(hour_persist_due ? "yes" : "no"));
            }
#endif
        }
    }
}

void analyticsClearUrbanNightHistory() {
    loadRollingHistoryIfNeeded();
    memset(&g_pm25_hour_hist, 0, sizeof(g_pm25_hour_hist));
    memset(&g_noise_hour_hist, 0, sizeof(g_noise_hour_hist));
    g_hist_dirty = true;
    saveRollingHistoryIfNeeded(true);
    debug_outln_info(F("[ANALYTICS] Cleared Urban PM/noise night history"));
}

void analyticsDevLogStatus15m() {
#if defined(ALTRUIST_BUILD_DEBUG)
    const uint32_t now_ms = millis();
    if ((now_ms - g_dev_last_status_log_ms) < (15UL * 60UL * 1000UL)) return;
    g_dev_last_status_log_ms = now_ms;

    loadRollingHistoryIfNeeded();

    char last_save_buf[24];
    formatLogTs(last_save_buf, sizeof(last_save_buf), g_hist_last_save_ts);
    debug_outln_info(F("[ANALYTICS][DEV][15m] NVS last save"), String(last_save_buf));

    auto log_metric = [&](const char *name, const RollingHourHistory &hist) {
        const uint8_t entries = rollingHistoryEntryCount(hist);
        uint32_t hk = 0;
        float avg = 0.0f;
        uint16_t cnt = 0;
        if (rollingHistoryLastEntry(hist, hk, avg, cnt)) {
            String line = String(name) + " entries=" + String((unsigned)entries) +
                          " last_hour_key=" + String((unsigned long)hk) +
                          " avg=" + String(avg, 2) +
                          " cnt=" + String((unsigned)cnt);
            debug_outln_info(F("[ANALYTICS][DEV][15m]"), line);
        } else {
            String line = String(name) + " entries=0 last=none";
            debug_outln_info(F("[ANALYTICS][DEV][15m]"), line);
        }
    };

    log_metric("co2", g_co2_hour_hist);
    log_metric("pm25", g_pm25_hour_hist);
    log_metric("noise", g_noise_hour_hist);
    log_metric("temp", g_temp_hour_hist);
    log_metric("hum", g_hum_hour_hist);
#endif
}

void populateAnalyticsPeriodStats(analytics_screen_values_t &values) {
    loadRollingHistoryIfNeeded();

    // "24H" is derived from hourly history: prefer completed local day, fallback to current day.
    time_t now = time(nullptr);
    if (now > 0) {
        uint32_t today = currentLocalDayKey(now);
        uint32_t completed_day = (today > 0U) ? (today - 1U) : 0U;
        bool ok = false;
        if (!values.temp_indoor.has_24h) {
            ok = applyHourlyMedianForDay(values.temp_indoor, g_temp_hour_hist, completed_day);
            if (!ok) applyHourlyMedianForDay(values.temp_indoor, g_temp_hour_hist, today);
        }
        if (!values.hum_indoor.has_24h) {
            ok = applyHourlyMedianForDay(values.hum_indoor, g_hum_hour_hist, completed_day);
            if (!ok) applyHourlyMedianForDay(values.hum_indoor, g_hum_hour_hist, today);
        }
        if (!values.dew_indoor.has_24h) {
            ok = applyHourlyMedianForDay(values.dew_indoor, g_dew_hour_hist, completed_day);
            if (!ok) applyHourlyMedianForDay(values.dew_indoor, g_dew_hour_hist, today);
        }
        if (!cfg::standalone) {
            if (cfg::analytics_sleep_add_urban) {
                if (!values.pm25.has_24h) {
                    ok = applyHourlyMedianForDay(values.pm25, g_pm25_hour_hist, completed_day);
                    if (!ok) applyHourlyMedianForDay(values.pm25, g_pm25_hour_hist, today);
                }
                if (!values.noise_avg.has_24h) {
                    ok = applyHourlyMedianForDay(values.noise_avg, g_noise_hour_hist, completed_day);
                    if (!ok) applyHourlyMedianForDay(values.noise_avg, g_noise_hour_hist, today);
                }
            }
        } else if (!values.pm25_insight.has_24h) {
            ok = applyHourlyMedianForDay(values.pm25_insight, g_pm25_hour_hist, completed_day);
            if (!ok) applyHourlyMedianForDay(values.pm25_insight, g_pm25_hour_hist, today);
        }
        if (!values.co2.has_24h) {
            ok = applyHourlyMedianForDay(values.co2, g_co2_hour_hist, completed_day);
            if (!ok) applyHourlyMedianForDay(values.co2, g_co2_hour_hist, today);
        }
    }
}

static int clampScore(int v) {
    if (v < 0) return 0;
    if (v > 100) return 100;
    return v;
}

static float impactAbove(float value, float threshold, float step, float pct_per_step) {
    if (value <= threshold || step <= 0.0f) return 0.0f;
    return -pct_per_step * ((value - threshold) / step);
}

static float nightImpactCO2(float co2, bool bio) {
    return bio
        ? impactAbove(co2, 600.0f, 100.0f, 0.8f)
        : impactAbove(co2, 750.0f, 100.0f, 0.52f);
}

static float nightImpactPM25(float pm25, bool bio) {
    return bio
        ? impactAbove(pm25, 3.0f, 10.0f, 0.5f)
        : impactAbove(pm25, 5.0f, 10.0f, 0.3f);
}

static float nightImpactNoisePeaks(uint16_t peaks, bool bio) {
    const uint16_t allowance = bio ? 1U : 5U;
    if (peaks <= allowance) return 0.0f;
    const float excess = (float)(peaks - allowance);
    const float pct_per_peak = bio ? 3.0f : 2.0f;
    return -pct_per_peak * excess;
}

static float nightImpactTemp(float temp, bool bio) {
    // Comfort bands for score 100 :
    // General 19–22°C, Biohacking 17–20°C. Penalize distance outside the band.
    float lo = bio ? 17.0f : 19.0f;
    float hi = bio ? 20.0f : 22.0f;
    float outside = 0.0f;
    if (temp < lo) outside = lo - temp;
    else if (temp > hi) outside = temp - hi;
    return -1.5f * outside; // −1.5% per °C outside band
}

static float nightImpactHum(float hum, bool bio) {
    float outside = 0.0f;
    if (!bio) {
        // Conservative: penalize humidity outside 40..60 with softer slope.
        if (hum < 40.0f) outside = 40.0f - hum;
        else if (hum > 60.0f) outside = hum - 60.0f;
        return -0.2f * (outside / 10.0f);
    }
    // Biohacking: narrower comfort band 40..50 with stronger slope.
    if (hum < 40.0f) outside = 40.0f - hum;
    else if (hum > 50.0f) outside = hum - 50.0f;
    return -0.4f * (outside / 10.0f);
}

static int nightScoreCO2(float co2, bool bio) {
    // Methodology: sleep impact decreases above threshold, then score = 100 + impact*2.
    const float impact = nightImpactCO2(co2, bio);
    return clampScore((int)lroundf(100.0f + impact * 2.0f));
}

static int nightScorePM25(float pm25, bool bio) {
    const float impact = nightImpactPM25(pm25, bio);
    return clampScore((int)lroundf(100.0f + impact * 2.0f));
}

static int nightScoreNoisePeaks(uint16_t peaks, bool bio) {
    const float impact = nightImpactNoisePeaks(peaks, bio);
    return clampScore((int)lroundf(100.0f + impact * 2.0f));
}

static int nightScoreTemp(float temp, bool bio) {
    const float impact = nightImpactTemp(temp, bio);
    return clampScore((int)lroundf(100.0f + impact * 2.0f));
}

static int nightScoreHum(float hum, bool bio) {
    const float impact = nightImpactHum(hum, bio);
    return clampScore((int)lroundf(100.0f + impact * 2.0f));
}

static char gradeLetter(int score) {
    if (score >= 90) return 'A';
    if (score >= 80) return 'B';
    if (score >= 70) return 'C';
    if (score >= 60) return 'D';
    if (score >= 50) return 'E';
    return 'F';
}


/** NVS keys remain analytics_night_*_hour; value is minutes 0..1439, or legacy 0..23 meaning that whole hour. */
static uint16_t nightCfgStartMinutes() {
    unsigned v = cfg::analytics_night_start_hour;
    if (v <= 23u) {
        return (uint16_t)(v * 60u);
    }
    if (v > 1439u) {
        return (uint16_t)(22 * 60);
    }
    return (uint16_t)v;
}

static uint16_t nightCfgEndMinutes() {
    unsigned v = cfg::analytics_night_end_hour;
    if (v <= 23u) {
        return (uint16_t)(v * 60u);
    }
    if (v > 1439u) {
        return (uint16_t)(7 * 60);
    }
    return (uint16_t)v;
}

/** Hour h covers [h*60, h*60+60); night window uses half-open ranges, end exclusive. */
static bool hourTouchesNightWindow(uint8_t h, uint16_t start_m, uint16_t end_m) {
    if (start_m == end_m) {
        return true;
    }
    const uint32_t hb = (uint32_t)h * 60U;
    const uint32_t he = hb + 60U;
    auto overlap = [](uint32_t a0, uint32_t a1, uint32_t b0, uint32_t b1) -> bool { return a0 < b1 && b0 < a1; };
    if (start_m < end_m) {
        return overlap(hb, he, (uint32_t)start_m, (uint32_t)end_m);
    }
    return overlap(hb, he, (uint32_t)start_m, 1440u) || overlap(hb, he, 0u, (uint32_t)end_m);
}

static uint16_t buildNightHourListMinutes(uint16_t start_m, uint16_t end_m, uint8_t out_hours[24]) {
    if (start_m == end_m) {
        uint16_t n = 0;
        for (uint8_t hh = 0; hh < 24; hh++) {
            out_hours[n++] = hh;
        }
        return n;
    }
    uint16_t n = 0;
    const bool wrap = (start_m > end_m);
    if (!wrap) {
        const uint8_t h0 = (uint8_t)(start_m / 60u);
        const uint8_t h1 = (end_m > 0u) ? (uint8_t)((end_m - 1u) / 60u) : 0u;
        for (uint8_t hh = h0; hh <= h1; hh++) {
            if (hourTouchesNightWindow(hh, start_m, end_m)) {
                out_hours[n++] = hh;
            }
        }
        return n;
    }
    for (uint8_t hh = (uint8_t)(start_m / 60u); hh < 24u; hh++) {
        if (hourTouchesNightWindow(hh, start_m, end_m)) {
            out_hours[n++] = hh;
        }
    }
    if (end_m > 0u) {
        const uint8_t h1 = (uint8_t)((end_m - 1u) / 60u);
        for (uint8_t hh = 0; hh <= h1; hh++) {
            if (hourTouchesNightWindow(hh, start_m, end_m)) {
                out_hours[n++] = hh;
            }
        }
    }
    return n;
}

static uint32_t resolveLastCompletedNightEndDay(uint16_t /*start_m*/, uint16_t end_m) {
    time_t now = time(nullptr);
    if (now <= 0) return 0U;
    struct tm lt;
    localtime_r(&now, &lt);
    const uint16_t now_m = (uint16_t)lt.tm_hour * 60U + (uint16_t)lt.tm_min;
    uint32_t today = currentLocalDayKey(now);

    // Night analytics progressive recompute window:
    // from 06:00 until configured end time, use today's end-day
    // so the report is updated hourly as new night hours appear.
    // Outside this window before 06:00, keep showing the previously completed night.
    const uint16_t progressive_recalc_start_m = 6 * 60;
    if (end_m > 0u && now_m >= end_m) return today;
    if (end_m > progressive_recalc_start_m && now_m >= progressive_recalc_start_m) return today;

    return (today > 0U) ? (today - 1U) : 0U;
}

static void drawTrackArc(int cx, int cy, int outer_r, int inner_r, float start_deg, float sweep_deg, float fill_norm) {
    if (outer_r <= inner_r) return;
    if (fill_norm < 0.0f) fill_norm = 0.0f;
    if (fill_norm > 1.0f) fill_norm = 1.0f;
    // Avoid ultra-tiny residual gaps/stubs on e-ink, but keep normal
    // high/low values (e.g. 95%) visually distinct from full/empty.
    if (fill_norm > 0.995f) fill_norm = 1.0f;
    if (fill_norm < 0.005f) fill_norm = 0.0f;

    const float span = fabsf(sweep_deg);
    const bool main_ring = span > 300.0f;
    int segments = main_ring ? 42 : (int)(span / 4.8f);
    if (segments < 7) segments = 7;
    const float seg_span = span / (float)segments;
    // Keep a visible gap so the shape reads as rotated rectangles/ticks.
    const float gap_deg = main_ring ? 1.1f : 1.4f;

    int filled_segments = (int)lroundf(fill_norm * (float)segments);
    if (filled_segments < 0) filled_segments = 0;
    if (filled_segments > segments) filled_segments = segments;

    auto drawTick = [&](float a0_deg, float a1_deg, bool filled) {
        if (a1_deg <= a0_deg) return;

        const float a0 = a0_deg * (kPi / 180.0f);
        const float a1 = a1_deg * (kPi / 180.0f);
        const int x0i = cx + (int)roundf(cosf(a0) * (float)inner_r);
        const int y0i = cy + (int)roundf(sinf(a0) * (float)inner_r);
        const int x0o = cx + (int)roundf(cosf(a0) * (float)outer_r);
        const int y0o = cy + (int)roundf(sinf(a0) * (float)outer_r);
        const int x1i = cx + (int)roundf(cosf(a1) * (float)inner_r);
        const int y1i = cy + (int)roundf(sinf(a1) * (float)inner_r);
        const int x1o = cx + (int)roundf(cosf(a1) * (float)outer_r);
        const int y1o = cy + (int)roundf(sinf(a1) * (float)outer_r);

        // Rectangle outline (always visible, including unfilled ticks).
        Paint_DrawLine(x0i, y0i, x0o, y0o, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(x1i, y1i, x1o, y1o, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(x0i, y0i, x1i, y1i, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(x0o, y0o, x1o, y1o, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

        if (!filled) return;

        // Fill tick body by sweeping thin radial lines between its side borders.
        int fill_steps = (int)(fabsf(a1_deg - a0_deg) * 1.8f);
        if (fill_steps < 3) fill_steps = 3;
        for (int s = 0; s <= fill_steps; ++s) {
            const float t = (float)s / (float)fill_steps;
            const float ad = a0_deg + (a1_deg - a0_deg) * t;
            const float ar = ad * (kPi / 180.0f);
            const int xi = cx + (int)roundf(cosf(ar) * (float)inner_r);
            const int yi = cy + (int)roundf(sinf(ar) * (float)inner_r);
            const int xo = cx + (int)roundf(cosf(ar) * (float)outer_r);
            const int yo = cy + (int)roundf(sinf(ar) * (float)outer_r);
            Paint_DrawLine(xi, yi, xo, yo, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        }
    };

    for (int i = 0; i < segments; ++i) {
        const float t0 = (float)i / (float)segments;
        const float t1 = (float)(i + 1) / (float)segments;
        float a0 = start_deg + sweep_deg * t0;
        float a1 = start_deg + sweep_deg * t1;
        if (sweep_deg >= 0.0f) {
            a0 += gap_deg * 0.5f;
            a1 -= gap_deg * 0.5f;
        } else {
            a0 -= gap_deg * 0.5f;
            a1 += gap_deg * 0.5f;
        }
        drawTick(a0, a1, i < filled_segments);
    }
}

static void drawTiltedBarTrack(int cx, int cy,
                               float anchor_deg,
                               int anchor_radius,
                               float tangent_deg,
                               int bar_len,
                               int bar_thickness,
                               float fill_norm) {
    fill_norm = clamp01(fill_norm);
    const float ar = anchor_deg * (kPi / 180.0f);
    const float tr = tangent_deg * (kPi / 180.0f);
    const float ux = cosf(tr);
    const float uy = sinf(tr);
    const float vx = -sinf(tr);
    const float vy = cosf(tr);

    const float ax = (float)cx + cosf(ar) * (float)anchor_radius;
    const float ay = (float)cy + sinf(ar) * (float)anchor_radius;
    const float half_len = (float)bar_len * 0.5f;
    const float half_th = (float)bar_thickness * 0.5f;

    struct Pt { float x; float y; };

    auto fillQuad = [&](const Pt &p0, const Pt &p1, const Pt &p2, const Pt &p3, uint16_t color) {
        float min_xf = p0.x, max_xf = p0.x, min_yf = p0.y, max_yf = p0.y;
        const Pt pts[4] = {p0, p1, p2, p3};
        for (int i = 1; i < 4; ++i) {
            if (pts[i].x < min_xf) min_xf = pts[i].x;
            if (pts[i].x > max_xf) max_xf = pts[i].x;
            if (pts[i].y < min_yf) min_yf = pts[i].y;
            if (pts[i].y > max_yf) max_yf = pts[i].y;
        }

        const int min_x = (int)floorf(min_xf) - 1;
        const int max_x = (int)ceilf(max_xf) + 1;
        const int min_y = (int)floorf(min_yf) - 1;
        const int max_y = (int)ceilf(max_yf) + 1;

        auto edge = [](const Pt &a, const Pt &b, float px, float py) -> float {
            return (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x);
        };

        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                const float px = (float)x + 0.5f;
                const float py = (float)y + 0.5f;
                const float e0 = edge(p0, p1, px, py);
                const float e1 = edge(p1, p2, px, py);
                const float e2 = edge(p2, p3, px, py);
                const float e3 = edge(p3, p0, px, py);
                const bool all_pos = (e0 >= -0.01f && e1 >= -0.01f && e2 >= -0.01f && e3 >= -0.01f);
                const bool all_neg = (e0 <= 0.01f && e1 <= 0.01f && e2 <= 0.01f && e3 <= 0.01f);
                if (all_pos || all_neg) {
                    Paint_DrawPoint(x, y, color, DOT_PIXEL_1X1, DOT_STYLE_DFT);
                }
            }
        }
    };

    auto drawQuadBorder = [&](const Pt &p0, const Pt &p1, const Pt &p2, const Pt &p3) {
        Paint_DrawLine((int)roundf(p0.x), (int)roundf(p0.y), (int)roundf(p1.x), (int)roundf(p1.y), BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine((int)roundf(p1.x), (int)roundf(p1.y), (int)roundf(p2.x), (int)roundf(p2.y), BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine((int)roundf(p2.x), (int)roundf(p2.y), (int)roundf(p3.x), (int)roundf(p3.y), BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine((int)roundf(p3.x), (int)roundf(p3.y), (int)roundf(p0.x), (int)roundf(p0.y), BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    };

    auto makeQuad = [&](float t0, float t1, Pt &q0, Pt &q1, Pt &q2, Pt &q3) {
        const float c0x = ax + ux * t0;
        const float c0y = ay + uy * t0;
        const float c1x = ax + ux * t1;
        const float c1y = ay + uy * t1;
        q0 = {c0x - vx * half_th, c0y - vy * half_th};
        q1 = {c0x + vx * half_th, c0y + vy * half_th};
        q2 = {c1x + vx * half_th, c1y + vy * half_th};
        q3 = {c1x - vx * half_th, c1y - vy * half_th};
    };

    // Draw a clean bar: one outer border + one filled part.
    Pt b0, b1, b2, b3;
    makeQuad(-half_len, half_len, b0, b1, b2, b3);
    fillQuad(b0, b1, b2, b3, WHITE);      // clear interior first
    drawQuadBorder(b0, b1, b2, b3);       // stable black outline

    const float filled_len = (float)bar_len * fill_norm;
    if (filled_len > 0.5f) {
        Pt f0, f1, f2, f3;
        const float ft0 = -half_len;
        const float ft1 = -half_len + filled_len;
        makeQuad(ft0, ft1, f0, f1, f2, f3);
        fillQuad(f0, f1, f2, f3, BLACK);

        // Separator between filled and unfilled area.
        if (fill_norm < 0.999f) {
            const float sx = ax + ux * ft1;
            const float sy = ay + uy * ft1;
            const int x0 = (int)roundf(sx - vx * half_th);
            const int y0 = (int)roundf(sy - vy * half_th);
            const int x1 = (int)roundf(sx + vx * half_th);
            const int y1 = (int)roundf(sy + vy * half_th);
            Paint_DrawLine(x0, y0, x1, y1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        }
    }
}

static void drawAnalyticsBlogQrSmall(int x, int y) {
    const char *qr_url = ANALYTICS_SLEEP_BLOG_URL;

    if (g_analytics_qr_bmp != nullptr && g_analytics_qr_url == qr_url &&
        g_analytics_qr_w > 0 && g_analytics_qr_h > 0) {
        Paint_DrawImage(g_analytics_qr_bmp, x, y, g_analytics_qr_w, g_analytics_qr_h);
        return;
    }

    QRCode qr;
    const uint8_t qr_version = 8;
    uint8_t qrcodeData[qrcode_getBufferSize(qr_version)];
    qrcode_initText(&qr, qrcodeData, qr_version, ECC_LOW, qr_url);

    const int scale = 1;
    const int quiet = 2;
    const int total_w = qr.size * scale + quiet * 2;
    const int total_h = qr.size * scale + quiet * 2;
    const int row_bytes = (total_w + 7) / 8;
    const int buf_sz = total_h * row_bytes;
    unsigned char *bmp = (unsigned char *)malloc(buf_sz);
    if (!bmp) return;
    memset(bmp, 0x00, buf_sz);

    for (uint8_t qy = 0; qy < qr.size; qy++) {
        for (uint8_t qx = 0; qx < qr.size; qx++) {
            if (!qrcode_getModule(&qr, qx, qy)) continue;
            for (int sy = 0; sy < scale; sy++) {
                for (int sx = 0; sx < scale; sx++) {
                    int px = quiet + qx * scale + sx;
                    int py = quiet + qy * scale + sy;
                    if (px >= 0 && px < total_w && py >= 0 && py < total_h) {
                        const int bi = py * row_bytes + (px / 8);
                        bmp[bi] |= (0x80 >> (px % 8));
                    }
                }
            }
        }
    }

    if (g_analytics_qr_bmp != nullptr) {
        free(g_analytics_qr_bmp);
    }
    g_analytics_qr_bmp = bmp;
    g_analytics_qr_url = qr_url;
    g_analytics_qr_w = total_w;
    g_analytics_qr_h = total_h;
    Paint_DrawImage(g_analytics_qr_bmp, x, y, g_analytics_qr_w, g_analytics_qr_h);
}

static void drawNightSinglePage(int content_left, int content_top, int content_width,
                                const analytics_screen_values_t &values) {
    float co2 = 0.0f, pm25 = 0.0f, temp = 0.0f, hum = 0.0f;
    uint16_t noise_peaks = 0;
    bool has_co2 = false, has_pm25 = false, has_noise = false, has_temp = false, has_hum = false;

    const int content_bottom = DISPLAY_HEIGHT - 6;
    const int content_height = content_bottom - content_top;
    const int col_gap = 10;
    int left_col_w = (content_width * 60) / 100;
    if (left_col_w < 198) left_col_w = 198;
    if (left_col_w > 232) left_col_w = 232;
    const int right_col_w = content_width - left_col_w - col_gap;
    const int left_panel_x = content_left;
    const int right_panel_x = content_left + left_col_w + col_gap;

    // Two-column layout: left = score circle + QR, right = metric cards.
    const int cx = left_panel_x + left_col_w / 2 + 6;
    const int cy = content_top + content_height / 2 + 19;
    const int r = 92;

    const uint16_t night_start_m = nightCfgStartMinutes();
    const uint16_t night_end_m = nightCfgEndMinutes();
    const uint8_t night_first_hour = (uint8_t)(night_start_m / 60U);
    uint8_t night_hours[24];
    const uint16_t n_hours = buildNightHourListMinutes(night_start_m, night_end_m, night_hours);
    const uint32_t end_day = resolveLastCompletedNightEndDay(night_start_m, night_end_m);
    const bool full_day_window = (night_start_m == night_end_m);
    const bool cross_midnight = !full_day_window && (night_start_m > night_end_m);

    float co2_day[24] = {0}, pm25_day[24] = {0}, noise_max_day[24] = {0}, temp_day[24] = {0}, hum_day[24] = {0};
    bool co2_has_day[24] = {false}, pm25_has_day[24] = {false}, noise_has_day[24] = {false}, temp_has_day[24] = {false}, hum_has_day[24] = {false};
    float co2_prev[24] = {0}, pm25_prev[24] = {0}, noise_max_prev[24] = {0}, temp_prev[24] = {0}, hum_prev[24] = {0};
    bool co2_has_prev[24] = {false}, pm25_has_prev[24] = {false}, noise_has_prev[24] = {false}, temp_has_prev[24] = {false}, hum_has_prev[24] = {false};
    readHourlyDayValuesFromHistory(g_co2_hour_hist, end_day, co2_day, co2_has_day);
    const bool sleep_include_pm25 = cfg::standalone || cfg::analytics_sleep_add_urban;
    const bool sleep_use_urban_pm_noise = !cfg::standalone && cfg::analytics_sleep_add_urban;
    if (sleep_include_pm25) {
        readHourlyDayValuesFromHistory(g_pm25_hour_hist, end_day, pm25_day, pm25_has_day);
    }
    if (sleep_use_urban_pm_noise) {
        readHourlyDayMaxFromHistory(g_noise_hour_hist, end_day, noise_max_day, noise_has_day);
    }
    readHourlyDayValuesFromHistory(g_temp_hour_hist, end_day, temp_day, temp_has_day);
    readHourlyDayValuesFromHistory(g_hum_hour_hist, end_day, hum_day, hum_has_day);
    if (cross_midnight && end_day > 0U) {
        readHourlyDayValuesFromHistory(g_co2_hour_hist, end_day - 1U, co2_prev, co2_has_prev);
        if (sleep_include_pm25) {
            readHourlyDayValuesFromHistory(g_pm25_hour_hist, end_day - 1U, pm25_prev, pm25_has_prev);
        }
        if (sleep_use_urban_pm_noise) {
            readHourlyDayMaxFromHistory(g_noise_hour_hist, end_day - 1U, noise_max_prev, noise_has_prev);
        }
        readHourlyDayValuesFromHistory(g_temp_hour_hist, end_day - 1U, temp_prev, temp_has_prev);
        readHourlyDayValuesFromHistory(g_hum_hour_hist, end_day - 1U, hum_prev, hum_has_prev);
    }

    uint16_t hours_with_any_data = 0;
    float co2_sum = 0.0f, pm25_sum = 0.0f, temp_sum = 0.0f, hum_sum = 0.0f;
    uint16_t co2_count = 0, pm25_count = 0, noise_hour_count = 0, temp_count = 0, hum_count = 0;
    for (uint16_t i = 0; i < n_hours; i++) {
        const uint8_t h = night_hours[i];
        const bool use_prev = !full_day_window && cross_midnight && (h >= night_first_hour);
        const bool c_has = use_prev ? co2_has_prev[h] : co2_has_day[h];
        const bool p_has =
            sleep_include_pm25 && (use_prev ? pm25_has_prev[h] : pm25_has_day[h]);
        const bool n_has =
            sleep_use_urban_pm_noise && (use_prev ? noise_has_prev[h] : noise_has_day[h]);
        const bool t_has = use_prev ? temp_has_prev[h] : temp_has_day[h];
        const bool hum_has = use_prev ? hum_has_prev[h] : hum_has_day[h];
        if (c_has || p_has || n_has || t_has || hum_has) hours_with_any_data++;

        if (c_has) { co2_sum += use_prev ? co2_prev[h] : co2_day[h]; co2_count++; }
        if (p_has) { pm25_sum += use_prev ? pm25_prev[h] : pm25_day[h]; pm25_count++; }
        if (n_has) {
            noise_hour_count++;
            const float hour_max = use_prev ? noise_max_prev[h] : noise_max_day[h];
            if (hour_max > kNoisePeakThresholdDb) {
                noise_peaks++;
            }
        }
        if (t_has) { temp_sum += use_prev ? temp_prev[h] : temp_day[h]; temp_count++; }
        if (hum_has) { hum_sum += use_prev ? hum_prev[h] : hum_day[h]; hum_count++; }
    }
    has_co2 = (co2_count > 0U);
    has_pm25 = (pm25_count > 0U);
    has_noise = (noise_hour_count > 0U);
    has_temp = (temp_count > 0U);
    has_hum = (hum_count > 0U);
    if (has_co2) co2 = co2_sum / (float)co2_count;
    if (has_pm25) pm25 = pm25_sum / (float)pm25_count;
    if (has_temp) temp = temp_sum / (float)temp_count;
    if (has_hum) hum = hum_sum / (float)hum_count;

    // Until enough Urban night buckets exist, approximate peaks from live noiseMax.
    if (sleep_use_urban_pm_noise && !has_noise) {
        if (values.noise_max.has_current) {
            has_noise = true;
            noise_peaks = (values.noise_max.current > kNoisePeakThresholdDb) ? 1U : 0U;
        } else if (values.noise_avg.has_current) {
            has_noise = true;
            noise_peaks = (values.noise_avg.current > kNoisePeakThresholdDb) ? 1U : 0U;
        }
    }

    const uint16_t min_hours_for_score = (n_hours >= 3) ? (uint16_t)((n_hours * 2U + 2U) / 3U) : n_hours;
    const bool enough_night_data = (hours_with_any_data >= min_hours_for_score);
    if (!enough_night_data) {
        char collect1[80], collect2[80];
        snprintf(collect1, sizeof(collect1), A_TXT("Night data is collecting", "Ночные данные собираются"));
        snprintf(collect2, sizeof(collect2), A_TXT("Hours: %u/%u", "Часов: %u/%u"),
                 (unsigned)hours_with_any_data, (unsigned)min_hours_for_score);
        uint16_t w1 = Paint_GetStringWidth_Display(collect1, &Font16, &font_16_cyrillic, &font_16_ascii);
        uint16_t w2 = Paint_GetStringWidth_Display(collect2, &Font16, &font_16_cyrillic, &font_16_ascii);
        const int block_h = Font16.Height * 2 + 6;
        const int msg_y = content_top + (DISPLAY_HEIGHT - content_top - block_h) / 2 - 18;
        Paint_DrawString_Display(content_left + (content_width - (int)w1) / 2, msg_y, collect1,
                                 &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
        Paint_DrawString_Display(content_left + (content_width - (int)w2) / 2, msg_y + Font16.Height + 6, collect2,
                                 &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
        return;
    }

    int s_cons_co2 = 0, s_cons_pm25 = 0, s_cons_noise = 0, s_cons_temp = 0, s_cons_hum = 0;
    int s_bio_co2 = 0, s_bio_pm25 = 0, s_bio_noise = 0, s_bio_temp = 0, s_bio_hum = 0;
    float i_cons_co2 = 0.0f, i_cons_pm25 = 0.0f, i_cons_noise = 0.0f, i_cons_temp = 0.0f, i_cons_hum = 0.0f;
    float i_bio_co2 = 0.0f, i_bio_pm25 = 0.0f, i_bio_noise = 0.0f, i_bio_temp = 0.0f, i_bio_hum = 0.0f;
    if (has_co2) {
        i_cons_co2 = nightImpactCO2(co2, false); i_bio_co2 = nightImpactCO2(co2, true);
        s_cons_co2 = nightScoreCO2(co2, false); s_bio_co2 = nightScoreCO2(co2, true);
    }
    if (has_pm25) {
        i_cons_pm25 = nightImpactPM25(pm25, false); i_bio_pm25 = nightImpactPM25(pm25, true);
        s_cons_pm25 = nightScorePM25(pm25, false); s_bio_pm25 = nightScorePM25(pm25, true);
    }
    if (has_noise) {
        i_cons_noise = nightImpactNoisePeaks(noise_peaks, false); i_bio_noise = nightImpactNoisePeaks(noise_peaks, true);
        s_cons_noise = nightScoreNoisePeaks(noise_peaks, false); s_bio_noise = nightScoreNoisePeaks(noise_peaks, true);
    }
    if (has_temp) {
        i_cons_temp = nightImpactTemp(temp, false); i_bio_temp = nightImpactTemp(temp, true);
        s_cons_temp = nightScoreTemp(temp, false); s_bio_temp = nightScoreTemp(temp, true);
    }
    if (has_hum) {
        i_cons_hum = nightImpactHum(hum, false); i_bio_hum = nightImpactHum(hum, true);
        s_cons_hum = nightScoreHum(hum, false); s_bio_hum = nightScoreHum(hum, true);
    }

    const float total_cons_impact =
        (has_co2 ? i_cons_co2 : 0.0f) +
        (has_pm25 ? i_cons_pm25 : 0.0f) +
        (has_noise ? i_cons_noise : 0.0f) +
        (has_temp ? i_cons_temp : 0.0f) +
        (has_hum ? i_cons_hum : 0.0f);
    const float total_bio_impact =
        (has_co2 ? i_bio_co2 : 0.0f) +
        (has_pm25 ? i_bio_pm25 : 0.0f) +
        (has_noise ? i_bio_noise : 0.0f) +
        (has_temp ? i_bio_temp : 0.0f) +
        (has_hum ? i_bio_hum : 0.0f);
    const int cons_score = clampScore((int)lroundf(100.0f + total_cons_impact * 2.0f));
    const int bio_score = clampScore((int)lroundf(100.0f + total_bio_impact * 2.0f));
    const int circle_score = cons_score;

    // QR → Sleep Analytics guide on sensors.social
    drawAnalyticsBlogQrSmall(left_panel_x + 4, content_top + 4);

    // Draw main score ring with solid black fill (faster redraw).
    // ================= MAIN SCORE RING =================

    const int main_outer = r + 4;
    const int main_inner = r - 12;
    const float main_fill = clamp01((float)circle_score / 100.0f);
    fillRingSolidFast(cx, cy, main_outer, main_inner, BLACK);
    // Keep visible score gap (e.g. 94% must not look like 100%).
    if (main_fill < 0.999f) {
        const float tail_start = -90.0f + 360.0f * main_fill;
        const float tail_sweep = 360.0f * (1.0f - main_fill);
        drawArcBandColor(cx, cy, main_outer, main_inner, tail_start, tail_sweep, WHITE);
    }

    // ================= METRIC FILL FUNCTION =================

    auto metricFill = [&](float c_impact, float b_impact, bool has_data) -> float {

        if (!has_data || !enough_night_data) return 0.0f;

        const float impact_avg = 0.5f * (c_impact + b_impact);

        float fill = 1.0f + (impact_avg / 10.0f);

        if (fill < 0.0f) fill = 0.0f;
        if (fill > 1.0f) fill = 1.0f;

        return fill;
    };

    const float f_co2   = metricFill(i_cons_co2,   i_bio_co2,   has_co2);
    const float f_pm25  = metricFill(i_cons_pm25,  i_bio_pm25,  has_pm25);
    const float f_noise = metricFill(i_cons_noise, i_bio_noise, has_noise);
    const float f_temp  = metricFill(i_cons_temp,  i_bio_temp,  has_temp);
    const float f_hum   = metricFill(i_cons_hum,   i_bio_hum,   has_hum);

    // ================= OUTER METRIC CARDS =================
    // Card style close to reference: title, value, and bottom fill bar.
    auto drawMetricCard = [&](int x, int y, int w, int h, const char *title, const char *value, const char *unit, float fill, int value_y_shift, bool small_title, int sep_shift, bool draw_border, bool /*draw_bar_border*/, const char *bottom_tag = nullptr) {
        if (w < 40 || h < 28) return;
        (void)fill; // value shown on card is already the night summary
        const bool is_temp_card = (strcmp(title, "T°") == 0);

        // Outer card.
        if (draw_border) {
            Paint_DrawRectangle(x, y, x + w, y + h, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
        }

        // Title.
        sFONT *title_font = small_title ? &Font12 : &Font16;
        const Font *title_cyr = small_title ? &font_12_cyrillic : &font_16_cyrillic;
        const Font *title_ascii = small_title ? &font_12_ascii : &font_16_ascii;
        uint16_t tw = Paint_GetStringWidth_Display(title, title_font, title_cyr, title_ascii);
        // Lift labels a bit (except the compact T° block which has its own tuning).
        int title_y = y + (is_temp_card ? 3 : 1);
        if (is_temp_card) title_y -= 5;
        Paint_DrawString_Display(x + (w - (int)tw) / 2, title_y, title, title_font, title_cyr, title_ascii, WHITE, BLACK);

        // Divider line.
        const int sep_y = y + (is_temp_card ? 17 : 15) + sep_shift;
        Paint_DrawLine(x + 16, sep_y, x + w - 16, sep_y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

        // Value + unit (primary).
        uint16_t vw = Paint_GetStringWidth_Display(value, &Font20, &font_20_cyrillic, &font_20_ascii);
        uint16_t uw = 0;
        if (unit && unit[0] != '\0') {
            uw = Paint_GetStringWidth_Display(unit, &Font12, &font_12_cyrillic, &font_12_ascii);
        }
        const int gap = (uw > 0) ? 3 : 0;
        const int total_w = (int)vw + (int)uw + gap;
        const int vx = x + (w - total_w) / 2;
        // Lift the value a bit to open space above the bottom tag (except T°).
        const int vy = y + (is_temp_card ? 24 : 21) + value_y_shift;
        Paint_DrawString_Display(vx, vy, value, &Font20, &font_20_cyrillic, &font_20_ascii, WHITE, BLACK);
        if (uw > 0) {
            Paint_DrawString_Display(vx + (int)vw + gap, vy + 5, unit, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        }

        // Bottom label: night average by default; noise uses peaks wording.
#if defined(INTL_RU)
        const char *avg_tag = bottom_tag ? bottom_tag : "ср. за ночь";
        uint16_t aw = Paint_GetStringWidth_Display(avg_tag, &Font12, &font_12_cyrillic, &font_12_ascii);
        Paint_DrawString_Display(x + (w - (int)aw) / 2, y + h - Font12.Height - 2,
                                 avg_tag, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
#else
        const char *avg_tag = bottom_tag ? bottom_tag : "night avg";
        uint16_t aw = Paint_GetStringWidth_Display(avg_tag, &Font8, &font_8_cyrillic, &font_8_ascii);
        Paint_DrawString_Display(x + (w - (int)aw) / 2, y + h - Font8.Height - 2,
                                 avg_tag, &Font8, &font_8_cyrillic, &font_8_ascii, WHITE, BLACK);
#endif
    };

    // ================= SCORE TEXT =================

    char score_txt[8];
    snprintf(score_txt, sizeof(score_txt), "%d", circle_score);

    uint16_t sw = Paint_GetStringWidth_Display(
        score_txt,
        &Font24,
        &font_48_cyrillic,
        &font_48_ascii
    );

    const int score_x = cx - (int)sw / 2;
    const int score_y = cy - 50;

    Paint_DrawString_Display(
        score_x,
        score_y,
        score_txt,
        &Font24,
        &font_48_cyrillic,
        &font_48_ascii,
        WHITE,
        BLACK
    );

    // ================= TOP TEXT =================

    const char *top_txt_line1 = A_TXT("Total score", "Общий показатель");
    const char *top_txt_line2 = A_TXT("general", "для всех");

    uint16_t tw1 = Paint_GetStringWidth_Display(
        top_txt_line1,
        &Font12,
        &font_12_cyrillic,
        &font_12_ascii
    );

    Paint_DrawString_Display(
        cx - (int)tw1 / 2,
        cy - 64,
        top_txt_line1,
        &Font12,
        &font_12_cyrillic,
        &font_12_ascii,
        WHITE,
        BLACK
    );

    uint16_t tw2 = Paint_GetStringWidth_Display(
        top_txt_line2,
        &Font12,
        &font_12_cyrillic,
        &font_12_ascii
    );
    const int general_y = cy + 2; // directly after main score, before biohacking block
    Paint_DrawString_Display(
        cx - (int)tw2 / 2,
        general_y,
        top_txt_line2,
        &Font12,
        &font_12_cyrillic,
        &font_12_ascii,
        WHITE,
        BLACK
    );

    // ================= BOTTOM TEXT =================
    // Keep number visually dominant and place label below it.
    char bio_score_txt[8];
    snprintf(bio_score_txt, sizeof(bio_score_txt), "%d", bio_score);
    const char *bio_label = A_TXT("biohacking", "для биохакеров");
    uint16_t nsw = Paint_GetStringWidth_Display(
        bio_score_txt,
        &Font20,
        &font_20_cyrillic,
        &font_20_ascii
    );
    uint16_t blw = Paint_GetStringWidth_Display(
        bio_label,
        &Font12,
        &font_12_cyrillic,
        &font_12_ascii
    );
    if (blw > (uint16_t)(r * 2 - 12)) {
        bio_label = A_TXT("biohacking", "для биохак.");
        blw = Paint_GetStringWidth_Display(
            bio_label,
            &Font12,
            &font_12_cyrillic,
            &font_12_ascii
        );
    }

    const int bio_num_y = cy + 24;
    const int sep_w = (int)((nsw > blw) ? nsw : blw) + 8;
    const int sep_y = bio_num_y + Font20.Height + 2;
    const int bio_label_y = sep_y + 3;

    Paint_DrawString_Display(
        cx - (int)nsw / 2,
        bio_num_y,
        bio_score_txt,
        &Font20,
        &font_20_cyrillic,
        &font_20_ascii,
        WHITE,
        BLACK
    );
    Paint_DrawLine(
        cx - sep_w / 2,
        sep_y,
        cx + sep_w / 2,
        sep_y,
        BLACK,
        DOT_PIXEL_1X1,
        LINE_STYLE_SOLID
    );
    Paint_DrawString_Display(
        cx - (int)blw / 2,
        bio_label_y,
        bio_label,
        &Font12,
        &font_12_cyrillic,
        &font_12_ascii,
        WHITE,
        BLACK
    );

    // ================= METRIC TEXTS =================
    char co2v[16] = "--";
    char pmv[16]  = "--";
    char nv[16]   = "--";
    char tv[16]   = "--";
    char hv[16]   = "--";

    if (has_co2)   snprintf(co2v, sizeof(co2v), "%.0f", co2);
    if (has_pm25)  snprintf(pmv,  sizeof(pmv),  "%.0f", pm25);
    if (has_noise) snprintf(nv,   sizeof(nv),   "%u", (unsigned)noise_peaks);
    if (has_temp)  snprintf(tv,   sizeof(tv),   "%.0f", temp);
    if (has_hum)   snprintf(hv,   sizeof(hv),   "%.0f%%", hum);

    // Top row: two cards (RH near QR + CO2 on right).
    const int top_row_y = content_top + 2;
    const int top_rh_w = 64;
    const int top_rh_h = 52;

    // Right-column metric stack (compact cards, unequal columns).
    const int card_x = right_panel_x + 9;
    const int card_w = right_col_w - 22;
    const int card_h = 54;
    const int card_gap = 7;
    const int cards_top = top_row_y + 8;
    const int top_rh_x = card_x - top_rh_w - 10;

    // Try layout variant: Temp small top block (near QR), RH in right metric stack.
    drawMetricCard(top_rh_x, top_row_y, top_rh_w, top_rh_h, "T°", tv, "°C", f_temp, -4, false, 0, false, true);

    // No extra label here; the header clarifies night average semantics.

    int metric_row = 0;
    drawMetricCard(card_x, cards_top + metric_row * (card_h + card_gap), card_w, card_h, "CO2", co2v, "ppm", f_co2, -1, false, 4, true, true);
    metric_row++;
    if (has_pm25) {
        drawMetricCard(card_x, cards_top + metric_row * (card_h + card_gap), card_w, card_h, "PM2.5", pmv, "µg/m³", f_pm25, -1, false, 4, true, true);
        metric_row++;
    }
    if (has_noise) {
        drawMetricCard(card_x, cards_top + metric_row * (card_h + card_gap), card_w, card_h,
                       A_TXT("Noise", "Шум"), nv, A_TXT("peaks", "пиков"), f_noise, -1, false, 4, true, true,
                       A_TXT(">45 dB/night", ">45 дБ/ночь"));
        metric_row++;
    }
    drawMetricCard(card_x, cards_top + metric_row * (card_h + card_gap), card_w, card_h, "RH", hv, "", f_hum, -1, false, 4, true, true);

    // circle/cards-only screen
    return;

}

void showAnalyticsPage(UBYTE *BlackImage, const analytics_screen_values_t &values) {
    (void)BlackImage;
    Paint_Clear(WHITE);

    struct tm timeinfo;
    const uint16_t header_top_y = 6;
    const uint16_t header_row_height = Font16.Height + 2;
    uint16_t header_bottom_border_y = header_top_y + header_row_height + 2;

    const uint16_t header_icon_size = 15;
    const uint16_t header_icon_x = 4;
    const uint16_t header_icon_y = header_top_y;
    Paint_DrawImage(chart_pie_15x15, header_icon_x, header_icon_y, header_icon_size, header_icon_size);

    if (getLocalTime(&timeinfo)) {
        char date_buf[12], time_buf[8];
        strftime(date_buf, sizeof(date_buf), "%m/%d/%Y", &timeinfo);
        strftime(time_buf, sizeof(time_buf), "%H:%M", &timeinfo);

        int time_width = (int)Paint_GetStringWidth_Display(time_buf, &Font16, &font_16_cyrillic, &font_16_ascii);
        int time_x = (DISPLAY_WIDTH - time_width) / 2;
        Paint_DrawString_Display(time_x, header_top_y, time_buf, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);

        int date_width = (int)Paint_GetStringWidth_Display(date_buf, &Font16, &font_16_cyrillic, &font_16_ascii);
        const int right_margin = 4;
        int date_x = DISPLAY_WIDTH - right_margin - date_width;
        int date_y = header_top_y;
        Paint_DrawString_Display(date_x, date_y, date_buf, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    }

    char period_text[96];
    snprintf(period_text, sizeof(period_text), "%s", analyticsViewLabelTitle());
    Paint_DrawString_Display(26, header_top_y + 2, period_text, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);

    Paint_DrawLine(0, header_bottom_border_y, DISPLAY_WIDTH, header_bottom_border_y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    const int sidebar_width = 26;
    const int content_left = 8;
    const int content_right = DISPLAY_WIDTH - sidebar_width - 6;
    const int content_width = content_right - content_left;
    const int content_top = header_bottom_border_y + 12;

    drawNightSinglePage(content_left, content_top, content_width, values);
    return;

}

#endif

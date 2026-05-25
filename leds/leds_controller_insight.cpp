#ifdef ALTRUIST_INSIDE

#include "leds_controller_insight.h"
#include <string.h>
#include "../utils.h"
#include "../config_manager/config_helpers.h"
#include "../defines.h"

static uint32_t scaleColor(uint32_t color, uint8_t percent) {
    if (percent >= 100) return color;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    r = (uint8_t)((r * percent) / 100);
    g = (uint8_t)((g * percent) / 100);
    b = (uint8_t)((b * percent) / 100);
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}


void LedControllerInsight::init() {
    if (LED_PIN != -1 && cfg::leds_on) {
        pixels.begin();
        pixels.clear();
        uint8_t brightness;
        // Scale user setting to 30% max: if user sets 50%, actual brightness is 15% (50% of 30%)
        uint8_t scaled_brightness = (cfg::leds_brightness * 30) / 100;
        if (scaled_brightness > 100) scaled_brightness = 100;
        if (scaled_brightness * 255 / 100 < 0) {
            brightness = 0;
        } else if (scaled_brightness * 255 / 100 > 255) {
            brightness = 255;
        } else {
            brightness = scaled_brightness * 255 / 100;
        }
        pixels.setBrightness(brightness);
        pixels.show();
        debug_outln_info(F("Setup leds on pin "), LED_PIN);
    } else {
        debug_outln_info(F("Will not setup leds on pin "), LED_PIN);
    }
    last_refresh_time = millis() - REFRESH_INTERVAL; 
}

void LedControllerInsight::scheduleNextLedRefresh(unsigned long delay_ms) {
    if (delay_ms < 500UL) {
        delay_ms = 500UL;
    }
    if (delay_ms > REFRESH_INTERVAL - 2000UL) {
        delay_ms = REFRESH_INTERVAL - 2000UL;
    }
    const unsigned long now = millis();
    last_refresh_time = now + delay_ms - REFRESH_INTERVAL - 1UL;
}

void LedControllerInsight::process() {
    if (LED_PIN == -1 || !cfg::leds_on) {
        return;
    }
    if (sleep_mode) {
        pixels.clear();
        pixels.show();
        return;
    }
    
    // Check if it's night time - if so, turn off LEDs completely
    if (_isNightTime()) {
        pixels.clear();
        pixels.show();
        return;
    }
    
    if (msSince(last_refresh_time) > REFRESH_INTERVAL) {
        static unsigned long mutex_diag_window_start_ms = 0;
        static unsigned long mutex_diag_last_warn_ms = 0;
        static uint32_t mutex_diag_fails_in_window = 0;
        static uint32_t mutex_diag_success_in_window = 0;
        static uint32_t mutex_diag_fail_streak = 0;
        static unsigned long last_forced_on_ms = 0;
        const unsigned long mutex_diag_window_ms = 10000UL;       // 10s summary window
        const unsigned long mutex_diag_warn_cooldown_ms = 5000UL; // throttle detailed warnings
        const unsigned long led_force_on_after_ms = 5UL * 60UL * 1000UL;   // 5 min without successful update
        const unsigned long led_force_on_cooldown_ms = 60UL * 1000UL;      // max one forced fallback per 60s

        // Calculate brightness:
        // 1. Scale user setting to 30% max (so 100% user = 30% actual)
        // 2. Apply time-based dimming as a percentage of that
        uint8_t scaled_brightness = (cfg::leds_brightness * 30) / 100;
        if (scaled_brightness > 100) scaled_brightness = 100;
        uint8_t time_percent = _calculateTimeBrightness(); // 0-100 percent
        uint8_t final_brightness = (scaled_brightness * 255 / 100); // Convert to 0-255
        final_brightness = (final_brightness * time_percent) / 100;  // Apply time dimming
        if (final_brightness > 255) final_brightness = 255;
        pixels.setBrightness(final_brightness);
        
        // Calculate percentage for logging
        float brightness_percent = (final_brightness * 100.0f) / 255.0f;
        debug_outln_verbose(F("LED brightness: user_setting="), String(cfg::leds_brightness) + F("% -> scaled=") + 
                        String(scaled_brightness) + F("% -> time=") + String(time_percent) + F("% -> final=") + String(brightness_percent, 1) + F("%"));
        
        // Acquire mutex while reading sensors_data to prevent race conditions
        // If we can't get it (display is updating), skip this cycle - try again next time
        if (!xSemaphoreTake(mutex, pdMS_TO_TICKS(100))) {
            const unsigned long now_ms = millis();
            if (mutex_diag_window_start_ms == 0) {
                mutex_diag_window_start_ms = now_ms;
            }
            mutex_diag_fails_in_window++;
            mutex_diag_fail_streak++;

            // Immediate warning for long fail streaks, throttled.
            if (mutex_diag_fail_streak >= 20 &&
                (now_ms - mutex_diag_last_warn_ms) > mutex_diag_warn_cooldown_ms) {
                mutex_diag_last_warn_ms = now_ms;
                debug_outln_info(F("[LED][WARN] mutex busy streak"),
                    String(mutex_diag_fail_streak) + F(" (100ms timeout each)"));
            }

            // Periodic summary to correlate with "LEDs stuck OFF" reports.
            if ((now_ms - mutex_diag_window_start_ms) >= mutex_diag_window_ms) {
                debug_outln_info(F("[LED][DIAG] mutex window"),
                    String(mutex_diag_window_ms / 1000) + F("s fails=") + String(mutex_diag_fails_in_window) +
                    F(" ok=") + String(mutex_diag_success_in_window) +
                    F(" streak=") + String(mutex_diag_fail_streak));
                mutex_diag_window_start_ms = now_ms;
                mutex_diag_fails_in_window = 0;
                mutex_diag_success_in_window = 0;
            }

            // Daytime safety fallback:
            // If we cannot update LEDs for a long time,
            // force a neutral ON state without touching shared sensor data.
            // This prevents "stuck OFF after night" behavior.
            if (final_brightness > 0 &&
                mutex_diag_fail_streak >= 30 &&
                msSince(last_refresh_time) > led_force_on_after_ms &&
                msSince(last_forced_on_ms) > led_force_on_cooldown_ms) {
                pixels.setBrightness(final_brightness);
                _setAllPixels(pixels.Color(255, 255, 255));
                pixels.show();
                last_forced_on_ms = now_ms;
                debug_outln_info(F("[LED][FALLBACK] Forced neutral ON"),
                    String(F("mutex busy, age_ms=")) + String(msSince(last_refresh_time)) +
                    String(F(", fail_streak=")) + String(mutex_diag_fail_streak));
            }
            // Couldn't get mutex, skip LED update this cycle (don't flash white)
            return;
        }

        // Successful lock acquisition: update diagnostics.
        const unsigned long now_ms = millis();
        if (mutex_diag_window_start_ms == 0) {
            mutex_diag_window_start_ms = now_ms;
        }
        mutex_diag_success_in_window++;
        mutex_diag_fail_streak = 0;
        if ((now_ms - mutex_diag_window_start_ms) >= mutex_diag_window_ms) {
            debug_outln_info(F("[LED][DIAG] mutex window"),
                String(mutex_diag_window_ms / 1000) + F("s fails=") + String(mutex_diag_fails_in_window) +
                F(" ok=") + String(mutex_diag_success_in_window) +
                F(" streak=") + String(mutex_diag_fail_streak));
            mutex_diag_window_start_ms = now_ms;
            mutex_diag_fails_in_window = 0;
            mutex_diag_success_in_window = 0;
        }
        
        // Build target colors in the same reading order as the main screen:
        // Noise(avg,max) -> PM(pm10,pm2.5) -> CO2 -> Temp(U/I) -> Hum(U/I) -> Pressure(U/I)
        uint32_t white = pixels.Color(255, 255, 255);
        uint32_t urban_temp_color = white;
        uint32_t insight_temp_color = white;
        uint32_t urban_humidity_color = white;
        uint32_t insight_humidity_color = white;
        uint32_t co2_color = white;
        uint32_t noise_avg_color = white;
        uint32_t noise_max_color = white;
        uint32_t pm10_color = white;
        uint32_t pm25_color = white;
        uint32_t urban_pressure_color = white;
        uint32_t insight_pressure_color = white;

        enum class urban_link_state_t : uint8_t { FRESH, STALE, OFFLINE };
        urban_link_state_t urban_link_state = urban_link_state_t::FRESH;
        if (!cfg::standalone) {
        // Apply the same TTL semantics as the main screen (with stale debounce).
        // UX requirement: show WHITE only when Urban is OFFLINE (not just stale).
        {
            static uint8_t urban_led_stale_confirm = 0;
            static uint32_t urban_led_tracked_last_ok_ms = 0xFFFFFFFFu;

            uint32_t last_ok_ms = 0;
            if (sensors_data.containsKey("service_data")) {
                JsonObjectConst service = sensors_data["service_data"].as<JsonObjectConst>();
                if (!service.isNull() && service.containsKey("urban_last_ok_ms")) {
                    last_ok_ms = service["urban_last_ok_ms"].as<uint32_t>();
                }
            }
            if (last_ok_ms != urban_led_tracked_last_ok_ms) {
                urban_led_stale_confirm = 0;
                urban_led_tracked_last_ok_ms = last_ok_ms;
            }

            const uint32_t now_ms = (uint32_t)millis();
            if (last_ok_ms != 0 && last_ok_ms <= now_ms) {
                const uint32_t age_ms = (uint32_t)(now_ms - last_ok_ms);
                if (age_ms > URBAN_OFFLINE_AFTER_MS) {
                    urban_led_stale_confirm = 0;
                    urban_link_state = urban_link_state_t::OFFLINE;
                } else if (age_ms > URBAN_STALE_AFTER_MS) {
                    if (urban_led_stale_confirm < URBAN_STALE_CONFIRMATIONS_REQUIRED) {
                        urban_led_stale_confirm++;
                    }
                    urban_link_state =
                        (urban_led_stale_confirm < URBAN_STALE_CONFIRMATIONS_REQUIRED)
                            ? urban_link_state_t::FRESH
                            : urban_link_state_t::STALE;
                } else {
                    urban_led_stale_confirm = 0;
                    urban_link_state = urban_link_state_t::FRESH;
                }
            } else if (last_ok_ms > now_ms) {
                // Invalid timestamp; don't force "disconnected" instantly.
                urban_link_state = urban_link_state_t::FRESH;
            }
        }

        const bool allow_urban_values = (urban_link_state != urban_link_state_t::OFFLINE);
        if (allow_urban_values && sensors_data.containsKey(ATRUIST_URBAN_SENSOR)) {
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("SDS_P1")) {
                pm10_color = _getColorByThresholds(
                    sensors_data[ATRUIST_URBAN_SENSOR]["SDS_P1"]["value"].as<float>(),
                    SensorConfigs::pm10_thresholds,
                    SensorConfigs::pm_colors,
                    5
                );
            }
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("SDS_P2")) {
                pm25_color = _getColorByThresholds(
                    sensors_data[ATRUIST_URBAN_SENSOR]["SDS_P2"]["value"].as<float>(),
                    SensorConfigs::pm25_thresholds,
                    SensorConfigs::pm_colors,
                    5
                );
            }
            
            // Noise is split to match main screen: avg first, then max.
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("PCBA_noiseAvg")) {
                noise_avg_color = _getNoiseColor(sensors_data[ATRUIST_URBAN_SENSOR]["PCBA_noiseAvg"]["value"].as<float>());
                debug_outln_verbose(F("Set Noise AVG color "), getColorName(noise_avg_color));
            }
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("PCBA_noiseMax")) {
                noise_max_color = _getNoiseColor(sensors_data[ATRUIST_URBAN_SENSOR]["PCBA_noiseMax"]["value"].as<float>());
                debug_outln_verbose(F("Set Noise MAX color "), getColorName(noise_max_color));
            }
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("BME280_temperature")) {
                urban_temp_color = _getTempColor(sensors_data[ATRUIST_URBAN_SENSOR]["BME280_temperature"]["value"].as<float>());
                debug_outln_verbose(F("Set U Temp color "), getColorName(urban_temp_color));
            }
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("BME280_humidity")) {
                urban_humidity_color = _getHumidityColor(sensors_data[ATRUIST_URBAN_SENSOR]["BME280_humidity"]["value"].as<float>());
                debug_outln_verbose(F("Set U Humidity color "), getColorName(urban_humidity_color));
            }
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("BME280_pressure")) {
                urban_pressure_color = _getPressureColor(sensors_data[ATRUIST_URBAN_SENSOR]["BME280_pressure"]["value"].as<float>() * 0.0075);
                debug_outln_verbose(F("Set U Pressure color "), getColorName(urban_pressure_color));
            }
        }
        }
        if (sensors_data.containsKey("BME680")) {
            insight_temp_color = _getTempColor(sensors_data["BME680"]["temperature"]["value"].as<float>());
            debug_outln_verbose(F("Set Temp color "), getColorName(insight_temp_color));
            insight_humidity_color = _getHumidityColor(sensors_data["BME680"]["humidity"]["value"].as<float>());
            debug_outln_verbose(F("Set Humidity color "), getColorName(insight_humidity_color));
            insight_pressure_color = _getPressureColor(sensors_data["BME680"]["pressure"]["value"].as<float>() * 0.0075);
            debug_outln_verbose(F("Set Pressure color "), getColorName(insight_pressure_color));
        }
        
        if (sensors_data.containsKey("SCD4x")) {
            co2_color = _getCO2Color(sensors_data["SCD4x"]["co2"]["value"].as<float>());
            debug_outln_verbose(F("Set CO2 color "), getColorName(co2_color));
        }

        if (cfg::standalone) {
            // No Urban stream: reuse Insight + CO2 semantics so the bar is coherent (not long white runs).
            pm10_color = co2_color;
            pm25_color = co2_color;
            noise_avg_color = co2_color;
            noise_max_color = co2_color;
            urban_temp_color = insight_temp_color;
            urban_humidity_color = insight_humidity_color;
            urban_pressure_color = insight_pressure_color;
        }
        xSemaphoreGive(mutex);

        // If Urban link is stale (not offline), dim Urban-driven segments instead of turning them white.
        // White is reserved for OFFLINE only.
        if (urban_link_state == urban_link_state_t::STALE) {
            const uint8_t stale_dim_percent = 35;
            pm10_color = scaleColor(pm10_color, stale_dim_percent);
            pm25_color = scaleColor(pm25_color, stale_dim_percent);
            noise_avg_color = scaleColor(noise_avg_color, stale_dim_percent);
            noise_max_color = scaleColor(noise_max_color, stale_dim_percent);
            urban_temp_color = scaleColor(urban_temp_color, stale_dim_percent);
            urban_humidity_color = scaleColor(urban_humidity_color, stale_dim_percent);
            urban_pressure_color = scaleColor(urban_pressure_color, stale_dim_percent);
        }

        // Map metrics to physical LED ranges so the bar order is:
        // Noise -> PM -> CO2 -> Temp -> Hum -> Pressure
        const uint8_t seg_start[SEGMENT_COUNT] = {
            1,  // Noise avg (Urban)
            4,  // Noise max (Urban)
            7,  // PM10 (Urban)
            10, // PM2.5 (Urban)
            13, // CO2 (Insight)
            16, // Temp (Urban)
            17, // Temp (Insight)
            19, // Hum (Urban)
            20, // Hum (Insight)
            22, // Pressure (Urban)
            26  // Pressure (Insight)
        };
        const uint8_t seg_end[SEGMENT_COUNT] = {
            3,
            6,
            9,
            12,
            15,
            16,
            18,
            19,
            21,
            25,
            28
        };
        const uint32_t seg_color[SEGMENT_COUNT] = {
            noise_avg_color,
            noise_max_color,
            pm10_color,
            pm25_color,
            co2_color,
            urban_temp_color,
            insight_temp_color,
            urban_humidity_color,
            insight_humidity_color,
            urban_pressure_color,
            insight_pressure_color
        };

        const uint8_t pulse_percent = 40;  
        const uint16_t pulse_ms = 25;     
        bool has_changed_segments = false;
        bool changed_segments[SEGMENT_COUNT] = {false};

        for (uint8_t i = 0; i < SEGMENT_COUNT; i++) {
            bool changed = segment_initialized[i] && segment_last_color[i] != seg_color[i];
            changed_segments[i] = changed;
            if (changed) {
                has_changed_segments = true;
            }
            segment_last_color[i] = seg_color[i];
            segment_initialized[i] = true;
            uint32_t initial_color = changed ? scaleColor(seg_color[i], pulse_percent) : seg_color[i];
            _setPartColor(seg_start[i], seg_end[i], initial_color);
        }

        // Subtle one-shot dim pulse only for segments that changed color.
        if (has_changed_segments) {
            pixels.show();
            delay(pulse_ms);
            for (uint8_t i = 0; i < SEGMENT_COUNT; i++) {
                if (changed_segments[i]) {
                    _setPartColor(seg_start[i], seg_end[i], seg_color[i]);
                }
            }
        }
        pixels.show();
        last_refresh_time = millis();  // Only mark as refreshed after successful update
    }
}

void LedControllerInsight::setSleepMode(bool enabled) {
    sleep_mode = enabled;
    if (LED_PIN == -1 || !cfg::leds_on) {
        return;
    }
    if (sleep_mode) {
        pixels.clear();
        pixels.show();
    }
}

void LedControllerInsight::_setAllPixels(uint32_t color) {
    for (int pixel = 0; pixel < LED_COUNT; pixel++) {
        pixels.setPixelColor(pixel, color);
    }
}

void LedControllerInsight::_setPartColor(uint8_t start_led, uint8_t end_led, uint32_t color) {
    for (int pixel = start_led - 1; pixel < end_led; pixel++) {
        pixels.setPixelColor(pixel, color);
    }
}

// Sensor threshold configurations - definitions (declared in header)
namespace SensorConfigs {
    // Noise thresholds (dB)
    const float noise_thresholds[] = {50, 70, 85, 100};
    const ColorName noise_colors[] = {ColorName::GREEN_LED, ColorName::BLUE_LED, ColorName::YELLOW_LED, ColorName::ORANGE_LED, ColorName::RED_LED};
    
    // CO2 thresholds (ppm)
    const float co2_thresholds[] = {1000, 2000, 5000};
    const ColorName co2_colors[] = {ColorName::GREEN_LED, ColorName::YELLOW_LED, ColorName::ORANGE_LED, ColorName::RED_LED};
    
    // Temperature thresholds (°C) 
    const float temp_thresholds[] = {1, 10, 27, 35};
    const ColorName temp_colors[] = {ColorName::DARKBLUE_LED, ColorName::BLUE_LED, ColorName::GREEN_LED, ColorName::YELLOW_LED, ColorName::ORANGE_LED};
    
    // PM thresholds 
    // PM10 thresholds (μg/m³)
    const float pm10_thresholds[] = {50, 100, 250, 350};
    // PM2.5 thresholds (μg/m³)
    const float pm25_thresholds[] = {30, 55, 110, 250};
    const ColorName pm_colors[] = {ColorName::GREEN_LED, ColorName::BLUE_LED, ColorName::YELLOW_LED, ColorName::ORANGE_LED, ColorName::RED_LED};
    
    // Humidity thresholds (%)
    const float humidity_thresholds[] = {30, 40, 60, 70};
    const ColorName humidity_colors[] = {ColorName::ORANGE_LED, ColorName::YELLOW_LED, ColorName::GREEN_LED, ColorName::BLUE_LED, ColorName::DARKBLUE_LED};
    
    // Pressure thresholds (mmHg)
    const float pressure_thresholds[] = {747, 768, 775};
    const ColorName pressure_colors[] = {ColorName::BLUE_LED, ColorName::GREEN_LED, ColorName::YELLOW_LED, ColorName::ORANGE_LED};
}

// Generic function to get color based on value and thresholds
// thresholds array should be in ascending order, colors array should have count elements
// (one color for each threshold range, and the last color is used for values above all thresholds)
uint32_t LedControllerInsight::_getColorByThresholds(float value, const float* thresholds, const ColorName* colors, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        if (value < thresholds[i]) {
            return getColor(colors[i]);
        }
    }
    // Value is above all thresholds, return the last color from the array
    return getColor(colors[count - 1]);
}

// PM color function
uint32_t LedControllerInsight::_getPMColor(float pm10, float pm25) {
    // PM color requires checking both PM10 and PM2.5 values simultaneously
    // Arrays have 4 thresholds (indices 0-3) and 5 colors (indices 0-4)
    const uint8_t threshold_count = 4;
    const uint8_t color_count = 5;
    
    // Check for invalid/negative values
    if (pm10 < 0 || pm25 < 0) {
        return getColor(SensorConfigs::pm_colors[0]); // Return GREEN for invalid data
    }
    
    if (pm10 >= SensorConfigs::pm10_thresholds[threshold_count - 1] || 
        pm25 >= SensorConfigs::pm25_thresholds[threshold_count - 1]) {
        return getColor(SensorConfigs::pm_colors[color_count - 1]); // RED
    }
    
    uint8_t worst_level = 0;
    for (uint8_t i = 0; i < threshold_count; i++) {
        if (pm10 >= SensorConfigs::pm10_thresholds[i] || pm25 >= SensorConfigs::pm25_thresholds[i]) {
            worst_level = i + 1; // Next color level (since we exceeded this threshold)
        }
    }
    
    // Clamp to valid color index (0-4)
    if (worst_level >= color_count) {
        worst_level = color_count - 1;
    }
    
    return getColor(SensorConfigs::pm_colors[worst_level]);
}

uint32_t LedControllerInsight::_getNoiseColor(float noise) {
    return _getColorByThresholds(noise, SensorConfigs::noise_thresholds, SensorConfigs::noise_colors, 5);
}

uint32_t LedControllerInsight::_getCO2Color(float co2) {
    return _getColorByThresholds(co2, SensorConfigs::co2_thresholds, SensorConfigs::co2_colors, 4);
}

uint32_t LedControllerInsight::_getTempColor(float temperature) {
    // Use SensorConfigs thresholds and colors directly
    return _getColorByThresholds(temperature, SensorConfigs::temp_thresholds, SensorConfigs::temp_colors, 5);
}

uint32_t LedControllerInsight::_getHumidityColor(float humidity) {
    return _getColorByThresholds(humidity, SensorConfigs::humidity_thresholds, SensorConfigs::humidity_colors, 5);
}

uint32_t LedControllerInsight::_getPressureColor(float pressure) {
    return _getColorByThresholds(pressure, SensorConfigs::pressure_thresholds, SensorConfigs::pressure_colors, 4);
}

// Calculate brightness based on time of day
// Returns 0-100 (percentage of brightness to apply)
uint8_t LedControllerInsight::_calculateTimeBrightness() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        // If we can't get time, use full brightness
        return 100;
    }

    const int minutes_since_midnight = (timeinfo.tm_hour * 60) + timeinfo.tm_min;
    const int off_hour = (cfg::leds_off_hour <= 23) ? (int)cfg::leds_off_hour : 0;
    const int on_hour = (cfg::leds_on_hour <= 23) ? (int)cfg::leds_on_hour : 6;

    // Equal values mean "no nightly auto-off period".
    if (off_hour == on_hour) {
        return 100;
    }

    // Smooth dimming starts 2 hours before configured off hour.
    const int dim_duration_minutes = 120;
    const int off_minutes = off_hour * 60;
    const int dim_start_minutes = (off_minutes - dim_duration_minutes + 1440) % 1440;

    bool in_dimming_window = false;
    int minutes_into_dimming = 0;
    if (dim_start_minutes <= off_minutes) {
        in_dimming_window = (minutes_since_midnight >= dim_start_minutes) &&
                            (minutes_since_midnight < off_minutes);
        minutes_into_dimming = minutes_since_midnight - dim_start_minutes;
    } else {
        // Window wraps midnight (e.g. off at 01:00 => dim starts at 23:00).
        in_dimming_window = (minutes_since_midnight >= dim_start_minutes) ||
                            (minutes_since_midnight < off_minutes);
        if (minutes_since_midnight >= dim_start_minutes) {
            minutes_into_dimming = minutes_since_midnight - dim_start_minutes;
        } else {
            minutes_into_dimming = (1440 - dim_start_minutes) + minutes_since_midnight;
        }
    }

    if (in_dimming_window) {
        int percent = 100 - ((100 * minutes_into_dimming) / dim_duration_minutes);
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        return (uint8_t)percent;
    }

    // Outside dimming window and outside night-time => full brightness.
    return 100;
}

// Check if it's night time (complete LED turn-off period)
bool LedControllerInsight::_isNightTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        // If we can't get time, don't turn off LEDs for safety
        return false;
    }

    const int hour = timeinfo.tm_hour;
    const int off_hour = (cfg::leds_off_hour <= 23) ? (int)cfg::leds_off_hour : 0;
    const int on_hour = (cfg::leds_on_hour <= 23) ? (int)cfg::leds_on_hour : 6;

    // Equal values mean "no nightly auto-off period".
    if (off_hour == on_hour) {
        return false;
    }

    // Night-time period: [off_hour, on_hour), handling midnight wrap.
    if (off_hour < on_hour) {
        return (hour >= off_hour && hour < on_hour);
    }
    return (hour >= off_hour || hour < on_hour);
}

#endif
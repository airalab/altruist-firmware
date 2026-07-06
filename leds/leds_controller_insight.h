#ifdef ALTRUIST_INSIGHT

#ifndef __LEDS_CONTROLLER_INSIDE_H__
#define __LEDS_CONTROLLER_INSIDE_H__

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <freertos/semphr.h>
#include "../defines.h"

#define LED_COUNT 28
#define MAX_BLINK_COUNT 3

#define REFRESH_INTERVAL 30000

enum class ColorName {
    RED_LED,
    GREEN_LED,
    BLUE_LED,
    ORANGE_LED,
    YELLOW_LED,
    DARKBLUE_LED
};

// Sensor threshold configurations
namespace SensorConfigs {
    // Noise thresholds (dB)
    extern const float noise_thresholds[];
    extern const ColorName noise_colors[];
    
    // CO2 thresholds (ppm)
    extern const float co2_thresholds[];
    extern const ColorName co2_colors[];
    
    // Temperature thresholds (°C) - special case: different order
    extern const float temp_thresholds[];
    extern const ColorName temp_colors[];
    
    // PM thresholds - requires both PM10 and PM2.5
    // PM10 thresholds (μg/m³)
    extern const float pm10_thresholds[];
    // PM2.5 thresholds (μg/m³)
    extern const float pm25_thresholds[];
    extern const ColorName pm_colors[];
    
    // Humidity thresholds (%)
    extern const float humidity_thresholds[];
    extern const ColorName humidity_colors[];
    
    // Pressure thresholds (mmHg)
    extern const float pressure_thresholds[];
    extern const ColorName pressure_colors[];
}

class LedControllerInsight {
    public:
        LedControllerInsight(const JsonDocument &_data, SemaphoreHandle_t _mutex) 
            : sensors_data(_data), mutex(_mutex), pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800) {}
        void init();
        void process();
        /** After a synchronous boot paint, allow another refresh after delay_ms (then normal REFRESH_INTERVAL cadence). */
        void scheduleNextLedRefresh(unsigned long delay_ms);
        void setSleepMode(bool enabled);

    private:
        static const uint8_t SEGMENT_COUNT = 11;
        const JsonDocument &sensors_data;
        SemaphoreHandle_t mutex;
        Adafruit_NeoPixel pixels;
        uint32_t last_refresh_time = 0;
        uint8_t current_time_brightness = 255;
        bool sleep_mode = false;
        bool segment_initialized[SEGMENT_COUNT] = {false};
        uint32_t segment_last_color[SEGMENT_COUNT] = {0};

        void _setAllPixels(uint32_t color);
        uint32_t getColor(ColorName c) {
            switch (c) {
                case ColorName::RED_LED:    return pixels.Color(255, 0, 0);
                case ColorName::GREEN_LED:  return pixels.Color(0, 255, 0);
                case ColorName::BLUE_LED:   return pixels.Color(1, 234, 234);
                case ColorName::DARKBLUE_LED:   return pixels.Color(1, 110, 227);
                case ColorName::YELLOW_LED: return pixels.Color(255, 255, 0);
                case ColorName::ORANGE_LED: return pixels.Color(245, 102, 7);
            }
            return 0;
        }
        String getColorName(uint32_t color) {
            if (color == pixels.Color(255, 0, 0)) return "RED";
            if (color == pixels.Color(0, 255, 0)) return "GREEN";
            if (color == pixels.Color(1, 234, 234)) return "BLUE";
            if (color == pixels.Color(1, 110, 227)) return "DARKBLUE";
            if (color == pixels.Color(245, 102, 7)) return "ORANGE";
            if (color == pixels.Color(255, 255, 0)) return "YELLOW";
            return "NONE";
        }
        // Sensor threshold configuration structure
        struct SensorThresholdConfig {
            const float* thresholds;
            const ColorName* colors;
            uint8_t threshold_count;
            bool is_special_case; // For sensors that need custom logic (like PM, Temp)
        };
        
        // Generic function to get color based on value and thresholds
        uint32_t _getColorByThresholds(float value, const float* thresholds, const ColorName* colors, uint8_t count);
        // Sensor-specific color functions
        uint32_t _getPMColor(float pm10, float pm25);
        uint32_t _getNoiseColor(float noise);
        uint32_t _getCO2Color(float co2);
        uint32_t _getTempColor(float temperature);
        uint32_t _getHumidityColor(float humidity);
        uint32_t _getPressureColor(float pressure);
        void _setPartColor(uint8_t start_led, uint8_t end_led, uint32_t color);
        uint8_t _calculateTimeBrightness();
        bool _isNightTime();
};

#endif

#endif
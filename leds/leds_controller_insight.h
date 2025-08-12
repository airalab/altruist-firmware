#ifdef ALTRUIST_INSIDE

#ifndef __LEDS_CONTROLLER_INSIDE_H__
#define __LEDS_CONTROLLER_INSIDE_H__

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "../defines.h"

#define LED_COUNT 12
#define MAX_BLINK_COUNT 3

#define REFRESH_INTERVAL 30000

enum class ColorName {
    RED_LED,
    GREEN_LED,
    BLUE_LED,
    ORANGE_LED,
    PURPLE_LED
};

class LedControllerInsight {
    public:
        LedControllerInsight(const JsonDocument &_data) : sensors_data(_data), pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800) {}
        void init();
        void process();

    private:
        const JsonDocument &sensors_data;
        Adafruit_NeoPixel pixels;
        uint32_t last_refresh_time = 0;

        void _setAllPixels(uint32_t color);
        uint32_t getColor(ColorName c) {
            switch (c) {
                case ColorName::RED_LED:    return pixels.Color(255, 0, 0);
                case ColorName::GREEN_LED:  return pixels.Color(0, 255, 0);
                case ColorName::BLUE_LED:   return pixels.Color(0, 0, 255);
                case ColorName::ORANGE_LED: return pixels.Color(255, 150, 0);
                case ColorName::PURPLE_LED: return pixels.Color(150, 0, 255);
            }
            return 0;
        }
        String getColorName(uint32_t color) {
            if (color == pixels.Color(255, 0, 0)) return "RED";
            if (color == pixels.Color(0, 255, 0)) return "GREEN";
            if (color == pixels.Color(0, 0, 255)) return "BLUE";
            if (color == pixels.Color(255, 150, 0)) return "ORANGE";
            if (color == pixels.Color(150, 0, 255)) return "PURPLE";
            return "NONE";
        }
        uint32_t _getPMColor(float pm10, float pm25);
        uint32_t _getNoiseColor(float noise);
        uint32_t _getCO2Color(float co2);
        uint32_t _getTempColor(float temperature);
        void _setPartColor(uint8_t start_led, uint8_t end_led, uint32_t color);
};

#endif

#endif
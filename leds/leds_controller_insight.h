#ifdef ALTRUIST_INSIDE

#ifndef __LEDS_CONTROLLER_INSIDE_H__
#define __LEDS_CONTROLLER_INSIDE_H__

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "../defines.h"

#define LED_COUNT 12
#define MAX_BLINK_COUNT 3

enum class ColorName {
    RED,
    GREEN,
    BLUE,
    ORANGE,
    PURPLE
};

class LedControllerInsight {
    public:
        LedControllerInsight(const JsonDocument &_data) : sensors_data(_data), pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800) {}
        void init();
        void process();

    private:
        const JsonDocument &sensors_data;
        Adafruit_NeoPixel pixels;

        void _setAllPixels(uint32_t color);
        uint32_t getColor(ColorName c) {
            switch (c) {
                case ColorName::RED:    return pixels.Color(255, 0, 0);
                case ColorName::GREEN:  return pixels.Color(0, 255, 0);
                case ColorName::BLUE:   return pixels.Color(0, 0, 255);
                case ColorName::ORANGE: return pixels.Color(255, 150, 0);
                case ColorName::PURPLE: return pixels.Color(150, 0, 255);
            }
            return 0;
        }
        void _setPartColor(uint8_t start_led, uint8_t end_led, uint32_t color);
};

#endif

#endif
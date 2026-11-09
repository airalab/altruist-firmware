#ifdef ALTRUIST_URBAN_HW_UI

#ifndef __LEDS_CONTROLLER_URBAN_H__
#define __LEDS_CONTROLLER_URBAN_H__

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_COUNT 2
#if defined(CONFIG_IDF_TARGET_ESP32C6)
#define URBAN_BOARD_RGB_LED_PIN 8
#else
#define URBAN_BOARD_RGB_LED_PIN -1
#endif
#define URBAN_BOARD_RGB_LED_COUNT 1

enum class LedMode {
    NONE,
    BLUE,
    PROVISIONING,
    RESETTING,
    GREEN,
    RED
};

class LedControllerUrban {
    public:
        LedControllerUrban();
        void init();
        void setMode(LedMode mode);
        void process();

    private:
        LedMode current_mode = LedMode::NONE;
        bool mode_changed = false;
        bool pixels_initialized = false;
        bool board_initialized = false;
        Adafruit_NeoPixel pixels;
        Adafruit_NeoPixel board_pixels;

        bool _hasBoardRgbHardware();
        bool _hasBoardRgbLed();
        String _modeName(LedMode mode);
        void _forceOff();
        void _applyBrightness();
        uint8_t _brightnessFromConfig();
        void _setSolidColor(uint8_t red, uint8_t green, uint8_t blue);
        void _setAllPixels(uint32_t color);
};

#endif

#endif
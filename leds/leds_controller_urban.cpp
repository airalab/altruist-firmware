#ifdef ALTRUIST_URBAN

#include "leds_controller_urban.h"
#include "../defines.h"
#include "../utils.h"

LedControllerUrban::LedControllerUrban():
    pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800) {}

void LedControllerUrban::init() {
    if (LED_PIN != -1) {
        pixels.begin();
        pixels.clear();
        pixels.show();
        debug_outln_info(F("Setup leds on pin "), LED_PIN);
    } else {
        debug_outln_info(F("Will not setup leds on pin "), LED_PIN);
    }
}

void LedControllerUrban::setMode(LedMode mode) {
    mode_changed = true;
    current_mode = mode;
}

void LedControllerUrban::process() {
    if (LED_PIN == -1) {
        return;
    }
    if (mode_changed) {
        mode_changed = false;
        switch (current_mode) {
            case LedMode::NONE:
                pixels.clear();
            case LedMode::BLUE:
                _setAllPixels(pixels.Color(0, 0, 255));
            case LedMode::BLINK_RED:
                for (int blink_count = 0; blink_count < MAX_BLINK_COUNT; blink_count++) {
                    _setAllPixels(pixels.Color(255, 0, 0));
                    delay(500);
                    _setAllPixels(pixels.Color(0, 0, 0));
                    delay(500);
                }
            case LedMode::BLINK_GREEN:
                for (int blink_count = 0; blink_count < MAX_BLINK_COUNT; blink_count++) {
                    _setAllPixels(pixels.Color(0, 255, 0));
                    delay(500);
                    _setAllPixels(pixels.Color(0, 0, 0));
                    delay(500);
                }
        }
        pixels.show();
    }
}

void LedControllerUrban::_setAllPixels(uint32_t color) {
    for (int pixel = 0; pixel < LED_COUNT; pixel++) {
        pixels.setPixelColor(pixel, color);
    }
}

#endif
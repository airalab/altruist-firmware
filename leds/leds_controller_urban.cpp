#ifdef ALTRUIST_URBAN

#include "leds_controller_urban.h"
#include "../defines.h"
#include "../utils.h"
#include "../config_manager/config_helpers.h"

LedControllerUrban::LedControllerUrban():
    pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800),
    board_pixels(URBAN_BOARD_RGB_LED_COUNT, URBAN_BOARD_RGB_LED_PIN, NEO_GRB + NEO_KHZ800) {}

void LedControllerUrban::init() {
    uint8_t brightness = cfg::leds_brightness * 255 / 100;
    if (LED_PIN != -1 && cfg::leds_on) {
        pixels.begin();
        pixels.clear();
        pixels.setBrightness(brightness);
        pixels.show();
        debug_outln_info(F("Setup leds on pin "), LED_PIN);
    } else {
        debug_outln_info(F("Will not setup leds on pin "), LED_PIN);
    }
    if (_hasBoardRgbLed()) {
        board_pixels.begin();
        board_pixels.clear();
        board_pixels.setBrightness(brightness);
        board_pixels.show();
        debug_outln_info(F("Setup board RGB led on pin "), String(URBAN_BOARD_RGB_LED_PIN));
    }
}

void LedControllerUrban::setMode(LedMode mode) {
    if (current_mode == mode && !mode_changed) {
        return;
    }
    mode_changed = true;
    current_mode = mode;
    debug_outln_info(F("[Urban LED] mode: "), _modeName(mode));
}

void LedControllerUrban::process() {
    if (LED_PIN == -1 || !cfg::leds_on) {
        return;
    }
    mode_changed = false;
    switch (current_mode) {
        case LedMode::NONE:
            pixels.clear();
            if (_hasBoardRgbLed()) {
                board_pixels.clear();
            }
            break;
        case LedMode::BLUE:
        case LedMode::PROVISIONING:
        case LedMode::RESETTING:
            _setSolidColor(0, 0, 255);
            break;
        case LedMode::GREEN:
            _setSolidColor(0, 255, 0);
            break;
        case LedMode::RED:
            _setSolidColor(255, 0, 0);
            break;
    }
    pixels.show();
    if (_hasBoardRgbLed()) {
        board_pixels.show();
    }
}

bool LedControllerUrban::_hasBoardRgbLed() {
    return cfg::leds_on && URBAN_BOARD_RGB_LED_PIN != -1 && URBAN_BOARD_RGB_LED_PIN != LED_PIN;
}

String LedControllerUrban::_modeName(LedMode mode) {
    switch (mode) {
        case LedMode::NONE:
            return F("NONE");
        case LedMode::BLUE:
            return F("BLUE");
        case LedMode::PROVISIONING:
            return F("PROVISIONING");
        case LedMode::RESETTING:
            return F("RESETTING");
        case LedMode::GREEN:
            return F("GREEN");
        case LedMode::RED:
            return F("RED");
    }
    return F("UNKNOWN");
}

void LedControllerUrban::_setSolidColor(uint8_t red, uint8_t green, uint8_t blue) {
    _setAllPixels(pixels.Color(red, green, blue));
}

void LedControllerUrban::_setAllPixels(uint32_t color) {
    for (int pixel = 0; pixel < LED_COUNT; pixel++) {
        pixels.setPixelColor(pixel, color);
    }
    if (_hasBoardRgbLed()) {
        board_pixels.setPixelColor(0, color);
    }
}

#endif
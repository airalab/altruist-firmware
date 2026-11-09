#ifdef ALTRUIST_URBAN_HW_UI

#include "leds_controller_urban.h"
#include "../defines.h"
#include "../utils.h"
#include "../config_manager/config_helpers.h"

LedControllerUrban::LedControllerUrban():
    pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800),
    board_pixels(URBAN_BOARD_RGB_LED_COUNT, URBAN_BOARD_RGB_LED_PIN, NEO_GRB + NEO_KHZ800) {}

void LedControllerUrban::init() {
    if (LED_PIN == -1 || !cfg::leds_on) {
        pixels_initialized = false;
        board_initialized = false;
        _forceOff();
        debug_outln_info(F("LEDs disabled; strip forced off"));
        return;
    }
    uint8_t brightness = _brightnessFromConfig();
    pixels.begin();
    pixels.clear();
    pixels.setBrightness(brightness);
    pixels.show();
    pixels_initialized = true;
    debug_outln_info(F("Setup leds on pin "), LED_PIN);
    if (_hasBoardRgbHardware()) {
        board_pixels.begin();
        board_pixels.clear();
        board_pixels.setBrightness(brightness);
        board_pixels.show();
        board_initialized = true;
        debug_outln_info(F("Setup board RGB led on pin "), String(URBAN_BOARD_RGB_LED_PIN));
    } else {
        board_initialized = false;
    }
}

void LedControllerUrban::setMode(LedMode mode) {
    // If LEDs are disabled in config, do not spam mode logs
    if (!cfg::leds_on || LED_PIN == -1) {
        current_mode = LedMode::NONE;
        mode_changed = false;
        return;
    }
    if (current_mode == mode && !mode_changed) {
        return;
    }
    mode_changed = true;
    current_mode = mode;
    debug_outln_info(F("[Urban LED] mode: "), _modeName(mode));
}

void LedControllerUrban::process() {
    if (LED_PIN == -1 || !cfg::leds_on) {
        _forceOff();
        return;
    }
    _applyBrightness();
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

bool LedControllerUrban::_hasBoardRgbHardware() {
    return URBAN_BOARD_RGB_LED_PIN != -1 && URBAN_BOARD_RGB_LED_PIN != LED_PIN;
}

bool LedControllerUrban::_hasBoardRgbLed() {
    return cfg::leds_on && _hasBoardRgbHardware();
}

void LedControllerUrban::_forceOff() {
    if (LED_PIN != -1) {
        if (!pixels_initialized) {
            pixels.begin();
            pixels_initialized = true;
        }
        pixels.clear();
        pixels.show();
    }
    if (_hasBoardRgbHardware()) {
        if (!board_initialized) {
            board_pixels.begin();
            board_initialized = true;
        }
        board_pixels.clear();
        board_pixels.show();
    }
    mode_changed = false;
    current_mode = LedMode::NONE;
}

uint8_t LedControllerUrban::_brightnessFromConfig() {
    unsigned pct = cfg::leds_brightness;
    if (pct > 100) {
        pct = 100;
    }
    return (uint8_t)((pct * 255UL) / 100UL);
}

void LedControllerUrban::_applyBrightness() {
    if (LED_PIN == -1) {
        return;
    }
    const uint8_t brightness = _brightnessFromConfig();
    if (!pixels_initialized) {
        pixels.begin();
        pixels_initialized = true;
    }
    pixels.setBrightness(brightness);
    if (_hasBoardRgbHardware()) {
        if (!board_initialized) {
            board_pixels.begin();
            board_initialized = true;
        }
        board_pixels.setBrightness(brightness);
    }
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
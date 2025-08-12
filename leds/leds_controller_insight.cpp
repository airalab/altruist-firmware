#ifdef ALTRUIST_INSIDE

#include "leds_controller_insight.h"
#include "../utils.h"


void LedControllerInsight::init() {
    if (LED_PIN != -1) {
        pixels.begin();
        pixels.clear();
        debug_outln_info(F("Setup leds on pin "), LED_PIN);
    } else {
        debug_outln_info(F("Will not setup leds on pin "), LED_PIN);
    }
}

void LedControllerInsight::process() {
    if (LED_PIN == -1) {
        return;
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

#endif
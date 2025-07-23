#ifdef ALTRUIST_INSIDE

#include <Arduino.h>
#include "button.h"
#include "../../defines.h"
#include "../../utils.h"

void ButtonController::init() {
    pinMode(_pin, INPUT_PULLUP);
}

PressType ButtonController::process() {
    uint8_t current_state = digitalRead(_pin);
    // debug_outln_info(F("Pin: "), _pin);
    // debug_outln_info(F("Current state: "), current_state);
    // debug_outln_info(F("Last state: "), last_state);
    PressType res = PressType::NONE;
    if (current_state == PRESSED_STATE) {
        if (last_state == NOT_PRESSED_STATE) {
            pressed_time = millis();
        } else {
            if (msSince(pressed_time) > LONG_PRESS_TIMEOUT && !long_press) {
                long_press = true;
                res = PressType::LONG;
            }
        }
    } else {
        if (last_state == PRESSED_STATE) {
            if (msSince(pressed_time) > SHORT_PRESS_TIMEOUT && !long_press) {
                res = PressType::SHORT;
            }
            pressed_time = 0;
            long_press = false;
        }
    }
    last_state = current_state;
    return res;
}

#endif
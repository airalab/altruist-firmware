#include <Arduino.h>
#include "button.h"
#include "../defines.h"
#include "../utils.h"

void ButtonController::init() {
    if (_pin != -1) {
#ifdef PULLUP_BUTTONS
        pinMode(_pin, INPUT_PULLUP);
#else
        pinMode(_pin, INPUT);
#endif
    } else {
        debug_outln_info(F("Can't setup button pin "), _pin);
    }
}

PressType ButtonController::process() {
    PressType res = PressType::NONE;
    if (_pin == -1) {
        return res;
    }
    uint8_t current_state = digitalRead(_pin);
    // debug_outln_info(F("Pin: "), _pin);
    // debug_outln_info(F("Current state: "), current_state);
    // debug_outln_info(F("Last state: "), last_state);
    // debug_outln_info(F("Pressed time: "), pressed_time);
    if (current_state == PRESSED_STATE) {
        if (last_state == NOT_PRESSED_STATE) {
            pressed_time = millis();
            debug_outln_info(F("Set press time "), pressed_time);
        } else {
            if (msSince(pressed_time) > LONG_PRESS_TIMEOUT && !long_press) {
                debug_outln_info(F("Press time long "), pressed_time);
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
            debug_outln_info(F("Release press time "), pressed_time);
        }
    }
    last_state = current_state;
    return res;
}

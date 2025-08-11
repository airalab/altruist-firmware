#ifndef BUTTON_ONE_H
#define BUTTON_ONE_H

#include <Arduino.h>

enum class PressType {
    NONE,
    LONG,
    SHORT
};

#define PRESSED_STATE 0
#define NOT_PRESSED_STATE 1

#define LONG_PRESS_TIMEOUT 4000
#define SHORT_PRESS_TIMEOUT 100

class ButtonController {
public:
    ButtonController(int pin) : _pin(pin) {}
    void init();
    PressType process();
    uint8_t get_last_state() const {
        return last_state;
    }

private:
    int _pin;
    uint8_t last_state = NOT_PRESSED_STATE;
    unsigned long pressed_time = 0;
    bool long_press = false;
};

#endif // BUTTON_ONE_H
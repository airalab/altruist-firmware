#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include "button.h"
#include "../defines.h"

enum class ButtonNum {
    NONE,
    UP,
    SET,
    DOWN
};

struct button_pressed_t {
    bool pressed = false;
    bool double_long = false;
    ButtonNum button_num = ButtonNum::NONE;
    ButtonNum second_button_num = ButtonNum::NONE;
    PressType press_type = PressType::NONE;
};

class ButtonManager {
public:
    ButtonManager(); 
    void init();
    button_pressed_t process();
    uint8_t get_button_state(ButtonNum button_num);

private:
#ifdef ALTRUIST_INSIGHT
    ButtonController up_button;
    ButtonController down_button;
    ButtonController set_button;
    unsigned long combo_hold_start_ms = 0;
    bool combo_wifi_reset_fired = false;
#elif defined(ALTRUIST_URBAN_HW_UI)
    ButtonController set_button;
#endif

};

#endif
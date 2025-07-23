#ifdef ALTRUIST_INSIDE

#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include "button.h"
#include "../../defines.h"

enum class ButtonNum {
    NONE,
    UP,
    SET,
    DOWN
};

struct button_pressed_t {
    bool pressed = false;
    ButtonNum button_num = ButtonNum::NONE;
    PressType press_type = PressType::NONE;
};

class ButtonManager {
public:
    ButtonManager(); 
    void init();
    button_pressed_t process();

private:
    ButtonController up_button;
    ButtonController down_button;
    ButtonController set_button;

};

#endif
#endif
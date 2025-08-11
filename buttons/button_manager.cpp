#include "button_manager.h"
#include "../utils.h"

void print_button_pressed(button_pressed_t &res) {
    String name;
    String type;
    if (res.button_num == ButtonNum::DOWN) {
        name = "DOWN";
    } else if (res.button_num == ButtonNum::UP) {
        name = "UP";
    } else if (res.button_num == ButtonNum::SET) {
        name = "SET";
    }
    if (res.press_type == PressType::SHORT) {
        type = "SHORT";
    } else if (res.press_type == PressType::LONG) {
        type = "LONG";
    }
    String message = "Button " + name + " was pressed " + type;
    if (res.double_long) {
        message += " DOUBLE"; 
    }
    debug_outln_info(F("[Button] "), message);
}

#ifdef ALTRUIST_INSIDE
ButtonManager::ButtonManager()
    : up_button(BTN_UP_PIN),
      down_button(BTN_DOWN_PIN),
      set_button(BTN_SET_PIN) {}
#endif
#ifdef ALTRUIST_URBAN
ButtonManager::ButtonManager()
    : set_button(BTN_SET_PIN) {}
#endif

void ButtonManager::init() {
#ifdef ALTRUIST_INSIDE
    up_button.init();
    down_button.init();
#endif
    set_button.init();
}

button_pressed_t ButtonManager::process() {
    button_pressed_t res;
#ifdef ALTRUIST_INSIDE
    PressType up_press = up_button.process();
    if (up_press != PressType::NONE) {
        res.pressed = true;
        res.button_num = ButtonNum::UP;
        res.press_type = up_press;
    }

    PressType down_press = down_button.process();
    if (down_press != PressType::NONE) {
        res.pressed = true;
        res.button_num = ButtonNum::DOWN;
        res.press_type = down_press;
    }
#endif
    PressType set_press = set_button.process();
    if (set_press != PressType::NONE) {
        res.pressed = true;
        res.button_num = ButtonNum::SET;
        res.press_type = set_press;
    }
#ifdef ALTRUIST_INSIDE
    if (res.press_type == PressType::LONG) {
        if (res.button_num == ButtonNum::DOWN) {
            if (set_button.get_last_state() == PRESSED_STATE) {
                res.double_long = true;
                res.second_button_num = ButtonNum::SET;
            }
        } else if (res.button_num == ButtonNum::SET) {
            if (down_button.get_last_state() == PRESSED_STATE) {
                res.double_long = true;
                res.second_button_num = ButtonNum::DOWN;
            }
        }
    }
#endif
    if (res.pressed) {
        print_button_pressed(res);
    }

    return res;
}

uint8_t ButtonManager::get_button_state(ButtonNum button_num) {
    if (button_num == ButtonNum::SET) {
        return set_button.get_last_state();
    }
#ifdef ALTRUIST_INSIDE
    if (button_num == ButtonNum::UP) {
        return up_button.get_last_state();
    }
    if (button_num == ButtonNum::DOWN) {
        return down_button.get_last_state();
    }
#endif
    return 2;
}

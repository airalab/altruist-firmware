#ifdef ALTRUIST_INSIDE

#include "display_manager.h"
#include "screens/main_screen.h"
#include "screens/graph.h"
#include "screens/setup.h"
#include "screens/loading.h"
#include "screens/display_common.h"

void DisplayManager::setScreen(ScreenPage pageID) {
    currentScreenID = pageID;
    refresh_now = true;
}

void DisplayManager::process(button_pressed_t &btn_press) {
    // button_pressed_t btn_press = button_manager.process();
    if (btn_press.pressed) {
        btn_press.pressed = false;
        if (currentScreenID == ScreenPage::MAIN) {
            if (btn_press.button_num == ButtonNum::DOWN || btn_press.button_num == ButtonNum::UP) {
                currentScreenID = ScreenPage::GRAPHS;
            }
            refresh_now = true;
        } else if (currentScreenID == ScreenPage::GRAPHS) {
            if (btn_press.button_num == ButtonNum::DOWN) {
                setNextGraphScreen();
            } else if (btn_press.button_num == ButtonNum::UP) {
                setPrevGraphScreen();
            } else if (btn_press.button_num == ButtonNum::SET) {
                currentScreenID = ScreenPage::MAIN;
                current_graph_screen = 1;
            }
            refresh_now = true;
        }
    }
    if (msSince(last_refresh_time) > DISPLAY_REFRESH_INTERVAL || refresh_now) {
        refresh_now = false;
        initAndClearScreen();
        UBYTE *BlackImage;
        createNewImage(BlackImage);
        if (currentScreenID == ScreenPage::MAIN) {
            String jsonString;
		    serializeJson(sensors_data, jsonString);
            debug_outln_info(F("Refresh main screen"));
            drawMainScreen(BlackImage, jsonString, deviceStatus.ip_address);
        } else if (currentScreenID == ScreenPage::GRAPHS) {
            drawGraphScreen();
        } else if(currentScreenID == ScreenPage::SETUP) {
            showSetupPage(BlackImage);
        } else if(currentScreenID == ScreenPage::LOADING) {
            showLoadingPage(BlackImage);
        }
        last_refresh_time = millis();
        showImageLong(BlackImage);
    }
}

#endif // ALTRUIST_INSIDE
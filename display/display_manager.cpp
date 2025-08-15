#ifdef ALTRUIST_INSIDE

#include "display_manager.h"
#include "screens/screens.h"

void DisplayManager::setScreen(ScreenPage pageID) {
    currentScreenID = pageID;
    refresh_now = true;
}

void DisplayManager::process(button_pressed_t &btn_press) {
    if (btn_press.pressed && !btn_press.double_long) {
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
    if (refresh_time_for_qr > 0 && msSince(refresh_time_for_qr) > 3000) {
        refresh_time_for_qr = 0;
        refresh_now = true;
    }
    if (msSince(last_refresh_time) > DISPLAY_REFRESH_INTERVAL || refresh_now) {
        refresh_now = false;
        initAndClearScreen();
        UBYTE *BlackImage;
        createNewImage(BlackImage);
        if (msSince(last_refresh_time) > DISPLAY_REFRESH_INTERVAL) {
            // if (refresh_count_for_qr > 0) {
            //     refresh_count_for_qr = 0;
                refresh_time_for_qr = millis();
                showSensorsMapPage(robonomics_address);
                last_refresh_time = millis();
                showImageLong(BlackImage);
                return;
            // } else {
            //     refresh_count_for_qr++;
            // }
        }
        if (currentScreenID == ScreenPage::MAIN) {
            String jsonString;
		    serializeJson(sensors_data, jsonString);
            debug_outln_info(F("Refresh main screen"));
            drawMainScreen(BlackImage, jsonString, deviceStatus.ip_address);
        } else if (currentScreenID == ScreenPage::GRAPHS) {
            drawGraphScreen();
        } else if (currentScreenID == ScreenPage::SETUP) {
            showSetupPage(BlackImage);
        } else if (currentScreenID == ScreenPage::LOADING) {
            showLoadingPage(BlackImage);
        } else if (currentScreenID == ScreenPage::CONNECTING) {
            showConnectingPage(BlackImage);
        } else if (currentScreenID == ScreenPage::LOGO) {
            showLogoPage();
        } else if (currentScreenID == ScreenPage::SENSOR_MAP) {
            showSensorsMapPage(robonomics_address);
        }
        last_refresh_time = millis();
        showImageLong(BlackImage);
    }
}

#endif // ALTRUIST_INSIDE
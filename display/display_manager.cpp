#ifdef ALTRUIST_INSIDE

#include "display_manager.h"
#include "screens/main_screen.h"
#include "screens/graph.h"
#include "../sd_card/sd_card.h"

void DisplayManager::process(button_pressed_t &btn_press) {
    // button_pressed_t btn_press = button_manager.process();
    if (btn_press.pressed) {
        btn_press.pressed = false;
        if (btn_press.button_num == ButtonNum::DOWN || btn_press.button_num == ButtonNum::UP) {
            if (currentScreenID == ScreenPage::MAIN) {
                currentScreenID = ScreenPage::GRAPHS;
                refresh_now = true;
            } else if (currentScreenID == ScreenPage::GRAPHS) {
                currentScreenID = ScreenPage::MAIN;
                refresh_now = true;
            }
        }
    }
    if (msSince(last_refresh_time) > DISPLAY_REFRESH_INTERVAL || refresh_now) {
        refresh_now = false;
        if (currentScreenID == ScreenPage::MAIN) {
            String jsonString;
		    serializeJson(sensors_data, jsonString);
            debug_outln_info(F("Refresh main screen"));
            drawMainScreen(jsonString, deviceStatus.ip_address);
            last_refresh_time = millis();
        } else if (currentScreenID == ScreenPage::GRAPHS) {
            UBYTE *BlackImage;
            createNewImage(BlackImage);
            GraphLineStyle line_style11;
            line_style11.width = DOT_PIXEL_2X2;
            line_style11.use_main_color = true;
            GraphLineStyle line_style1;
            line_style1.width = DOT_PIXEL_1X1;
            line_style1.use_main_color = true;
            uint16_t height = 100;
            uint16_t width = 160;

            // Graph 1
            LineData result = {nullptr, nullptr, 0};
            readSensorDataFromCSV(result, "altruist_urban", "PCBA_noiseMax", 12);
            GraphPainter graph1(10, 110, height, width);
            graph1.setWhiteMode();
            graph1.addLineValues(result.values, result.timestamps, result.count, "Max Noise", line_style1);
            graph1.drawGraph();
            delete[] result.values;
            delete[] result.timestamps;

            // Graph 2
            LineData result22 = {nullptr, nullptr, 0};
            readSensorDataFromCSV(result22, "altruist_urban", "SDS_P1", 12);
            LineData result21 = {nullptr, nullptr, 0};
            readSensorDataFromCSV(result21, "altruist_urban", "SDS_P2", 12);
            GraphPainter graph2(10, 230, height, width);
            graph2.setWhiteMode();
            graph2.addLineValues(result22.values, result22.timestamps, result22.count, "PM10", line_style1);
            // graph2.addLineValues(result21.values, result21.timestamps, result21.count, "PM2.5", line_style11);
            graph2.drawGraph();
            delete[] result22.values;
            delete[] result22.timestamps;
            delete[] result21.values;
            delete[] result21.timestamps;

            // Graph 3
            result = {nullptr, nullptr, 0};
            readSensorDataFromCSV(result, "altruist_urban", "BME280_temperature", 12);
            GraphPainter graph3(190, 110, height, width);
            graph3.setWhiteMode();
            graph3.addLineValues(result.values, result.timestamps, result.count, "Temp", line_style1);
            graph3.drawGraph();
            delete[] result.values;
            delete[] result.timestamps;

            // Graph 3
            result = {nullptr, nullptr, 0};
            readSensorDataFromCSV(result, "altruist_urban", "BME280_humidity", 12);
            GraphPainter graph4(190, 230, height, width);
            graph4.setWhiteMode();
            graph4.addLineValues(result.values, result.timestamps, result.count, "Hum", line_style1);
            graph4.drawGraph();
            delete[] result.values;
            delete[] result.timestamps;

            refreshScreen(BlackImage);
            last_refresh_time = millis();
        }
    }
}

#endif // ALTRUIST_INSIDE
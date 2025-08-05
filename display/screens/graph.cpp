#ifdef ALTRUIST_INSIDE

#include "../paint_driver/graphPainter.h"
#include "graph.h"
#include "../utils.h"
#include "../../utils.h"
#include "../../sd_card/sd_card.h"
#include "../../config_manager/config_helpers.h"

uint8_t current_graph_screen = 1;

#if defined(USE_SD_CARD)
static uint16_t drawOneGraph(int left_x, int left_y, const char* sensor_name, const char* meas_name, const char* label) {
    GraphLineStyle line_style;
    LineData result = {nullptr, nullptr, 0};
    readSensorDataFromCSV(result, sensor_name, meas_name, 12);
    GraphPainter graph(left_x, left_y, GRAPH_HEIGHT, GRAPH_WIDTH);
    graph.setWhiteMode();
    graph.addLineValues(result.values, result.timestamps, result.count, label, line_style);
    graph.drawGraph();
    return graph.getGraphWidth();
}
#endif

void drawGraphScreen() {
    debug_outln_info(F("Set graph screen "), current_graph_screen);
#if defined(USE_SD_CARD)
    // String urban_ip = cfg::chosen_altruist_urban;
    // int last_dot = urban_ip.lastIndexOf('.');
    // if (last_dot == -1) {
    //     debug_outln_info(F("Invalid IP address in cfg::chosen_altruist_urban"));
    //     return;
    // }
    // String last_octet = urban_ip.substring(last_dot + 1);
    // String urban_key = ATRUIST_URBAN_SENSOR + last_octet;
    String urban_key = ATRUIST_URBAN_SENSOR;
    if (current_graph_screen == 1) {
        drawOneGraph(10, 10 + GRAPH_HEIGHT, urban_key.c_str(), "SDS_P1", "PM10");
        drawOneGraph(DISPLAY_WIDTH / 2 + 10, 10 + GRAPH_HEIGHT, urban_key.c_str(), "SDS_P2", "PM2.5");
        drawOneGraph(10, DISPLAY_HEIGHT - 10, urban_key.c_str(), "PCBA_noiseMax", "Max Noise");
        drawOneGraph(DISPLAY_WIDTH / 2 + 10, DISPLAY_HEIGHT - 10, urban_key.c_str(), "PCBA_noiseAvg", "Avg Noise");
    } else if (current_graph_screen == 2) {
        drawOneGraph(10, 10 + GRAPH_HEIGHT, urban_key.c_str(), "BME280_temperature", "Urban Temp");
        drawOneGraph(DISPLAY_WIDTH / 2 - GRAPH_HEIGHT / 2, DISPLAY_HEIGHT - 10, urban_key.c_str(), "BME280_humidity", "Urban Hum");
        drawOneGraph(DISPLAY_WIDTH / 2 + 10, 10 + GRAPH_HEIGHT, urban_key.c_str(), "BME280_pressure", "Urban Press");
    } else if (current_graph_screen == 3) {
        drawOneGraph(10, 10 + GRAPH_HEIGHT, "BME680", "temperature", "Insight Temp");
        drawOneGraph(DISPLAY_WIDTH / 2 + 10, 10 + GRAPH_HEIGHT, "BME680", "pressure", "Insight Press");
        drawOneGraph(10, DISPLAY_HEIGHT - 10, "BME680", "humidity", "Insight Hum");
        drawOneGraph(DISPLAY_WIDTH / 2 + 10, DISPLAY_HEIGHT - 10, "SCD4x", "co2", "Insight CO2");
    }
#else
    Paint_DrawString_EN_Center("No history available", &Font24, WHITE, BLACK);
#endif
}

void setNextGraphScreen() {
    if (current_graph_screen < 3) {
        current_graph_screen++;
    } else {
        current_graph_screen = 1;
    }
}

void setPrevGraphScreen() {
    if (current_graph_screen > 1) {
        current_graph_screen--;
    } else {
        current_graph_screen = 3;
    }
}

#endif
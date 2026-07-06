
#ifdef ALTRUIST_INSIGHT

#ifndef DISPLAY_GRAPH_H
#define DISPLAY_GRAPH_H

#include <stdlib.h>
#include "GUI_Paint.h"
#include "../driver/EPD.h"
#include "../driver/DEV_Config.h"

#define MAX_LINES 3

struct GraphLineStyle {
    LINE_STYLE style = LINE_STYLE_SOLID;
    DOT_PIXEL width = DOT_PIXEL_1X1;
    bool use_main_color = true;
};

struct GraphLine {
    float *values;
    uint32_t *timestamps;
    int values_count;
    const char *label;
    GraphLineStyle line_style;
};

class GraphPainter {
public:
    GraphPainter(uint16_t left_bottom_x, uint16_t left_bottom_y, uint16_t height, uint16_t width);
    // ~GraphPainter();

    void drawGraph();
    void addLineValues(float* values, uint32_t* timestamps, int values_count, const char* label, GraphLineStyle line_style);

    void addLineValues(float* values, uint32_t* timestamps, int values_count, const char* label);
    void setShowHours(uint8_t hours);
    // Optional right-aligned note shown next to "hourly update" label.
    // `note` is copied into an internal buffer (nullptr/empty clears it).
    void setUpdateNote(const char* note);
    void setBlackMode() {
        main_color = WHITE;
        background_color = BLACK;
        inversed_colors = true;
    }

    void setWhiteMode() {
        main_color = BLACK;
        background_color = WHITE;
        inversed_colors = false;
    }

    bool inversed_colors = false;

    uint16_t getGraphWidth() const {
        return graph_width;
    }

private:
    uint16_t main_color = BLACK;
    uint16_t background_color = WHITE;
    sFONT digitFont = Font12;
    sFONT labelFont = Font12;
    uint8_t show_hours = 12; // Должно делиться на 4
    GraphLine lines[MAX_LINES];
    uint8_t lines_count = 0;
    uint16_t left_bottom_x;
    uint16_t left_bottom_y;
    uint16_t height;
    uint16_t width;
    float max_value = 0;
    float min_value = 99999;
    int ticks = 8; 
    float range;
    float step;
    float graph_min;
    float graph_max;
    uint16_t digit_width;
    uint16_t digit_height;
    uint16_t graph_width;
    uint16_t graph_height;
    uint16_t left_bottom_graph_x;
    uint16_t left_bottom_graph_y;
    char update_note[48] = {0};

    void calculateYLabelWidth();
    void calculateMinMax();
    void drawLabel();
    void drawAxisLabels(time_t *time_now);
    void drawYLabels();
    void drawXLabels(time_t *time_now);
    void drawBorders();
    void drawLine(uint8_t line_number, time_t *time_now);
    float nice_number(float value, int round);
};

#endif // DISPLAY_GRAPH_H

#endif
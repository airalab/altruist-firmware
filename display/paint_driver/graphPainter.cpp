#ifdef ALTRUIST_INSIDE

#include "graphPainter.h"
#include "utils.h"
#include "../utils.h"
#include "../../config_manager/config_helpers.h"

int get_timezone_offset() {
    String tz_string(cfg::timezone);
    int start = tz_string.indexOf('<');
    int end = tz_string.indexOf('>');

    if (start == -1 || end == -1 || end <= start + 1) {
        return 0;  // значение по умолчанию
    }

    String offset_str = tz_string.substring(start + 1, end);
    return offset_str.toInt();  // автоматически распарсит -12, +3 и т.д.
}

GraphPainter::GraphPainter(uint16_t left_bottom_x, uint16_t left_bottom_y,
                           uint16_t height, uint16_t width)
    : left_bottom_x(left_bottom_x), left_bottom_y(left_bottom_y),
      height(height), width(width) {}

void GraphPainter::addLineValues(float* values, uint32_t* timestamps, int values_count, const char* label) {
    addLineValues(values, timestamps, values_count, label, GraphLineStyle{});
}

void GraphPainter::addLineValues(float* values, uint32_t* timestamps, int values_count, const char* label, GraphLineStyle line_style) {
    if (lines_count < MAX_LINES) {
        lines[lines_count++] = { values, timestamps, values_count, label, line_style };
    }
}

void GraphPainter::calculateMinMax() {
    for (int j = 0; j < lines_count; j++) {
        GraphLine& line = lines[j];
        for (int i = 0; i < line.values_count; i++) {
            if (line.values[i] > max_value) max_value = line.values[i];
            if (line.values[i] < min_value) min_value = line.values[i];
        }
    }
    // Serial.printf("min_value: %.2f, max_value: %.2f\n\r", min_value, max_value);
    range = nice_number(max_value - min_value, 0);
    step = nice_number(range / (ticks - 1), 1);
    graph_min = floorf(min_value / step) * step;
    graph_max = graph_min + step * (ticks - 1);

    // Serial.printf("graph_min: %.2f, graph_max: %.2f, step: %.2f, range: %.2f\n\r", graph_min, graph_max, step, range);
}

void GraphPainter::drawGraph() {
    calculateMinMax();
    calculateYLabelWidth();
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        debug_outln_info(F("Failed to get time"));
        return;
    }
    time_t time_now = mktime(&timeinfo);
    // uint32_t time_now = 1747765576;
    drawAxisLabels(&time_now);
    drawBorders();
    for (int i = 0; i < lines_count; i++) {
        if (lines[i].values_count > 0) {
            drawLine(i, &time_now);
        }
    }
    drawLabel();
}

void GraphPainter::drawLabel() {
    char label_text[40];
    for (int j = 0; j < lines_count; j++) {
        char label_value[5];
        stringFromFloat(label_value, lines[j].values[lines[j].values_count - 1]);
        if (j == 0) {
            snprintf(label_text, sizeof(label_text), "%s: %s", lines[j].label, label_value);
        } else {
            snprintf(label_text, sizeof(label_text), "%s, %s: %s", label_text, lines[j].label, label_value);
        }
        // Serial.printf("label_text: %s\n\r", label_text);
    }
    uint16_t x;
    if (strlen(label_text) * labelFont.Width > width) {
        x = left_bottom_x;
    } else {
        x = left_bottom_x + (width - strlen(label_text) * labelFont.Width) / 2;
    }
    uint16_t y = left_bottom_graph_y - graph_height - labelFont.Height - 8;
    // Serial.printf("Label x: %d, y: %d\n\r", x, y);
    Paint_DrawString_EN(x, y, label_text, &labelFont, background_color, main_color);
}

void GraphPainter::drawLine(uint8_t line_number, time_t *time_now) {
    uint32_t start_time = *time_now - show_hours*60*60;
    uint16_t x_prev = 0;
    uint16_t line_color;
    if (lines[line_number].line_style.use_main_color) {
        line_color = main_color;
    } else {
        line_color = background_color;
    }
    for (int i = 0; i < lines[line_number].values_count; i++) {
        uint16_t x = left_bottom_graph_x + (lines[line_number].timestamps[i] - start_time) * graph_width / (*time_now - start_time);
        if (i == 0) {
            Paint_DrawPoint(x, left_bottom_graph_y - (graph_height * (lines[line_number].values[i] - graph_min) / (graph_max - graph_min)), line_color, DOT_PIXEL_1X1, DOT_STYLE_DFT);
            // Paint_DrawPoint(x_start, y[i], BLACK, line_width, DOT_STYLE_DFT);
        } else {
            uint16_t y = left_bottom_graph_y - (graph_height * (lines[line_number].values[i] - graph_min) / (graph_max - graph_min));
            uint16_t y_prev = left_bottom_graph_y - (graph_height * (lines[line_number].values[i - 1] - graph_min) / (graph_max - graph_min));

            Paint_DrawLine(x_prev, y_prev, x, y, line_color, lines[line_number].line_style.width, lines[line_number].line_style.style);
        }
        x_prev = x;
    }
}

void GraphPainter::drawBorders() {
    Paint_DrawLine(left_bottom_graph_x, left_bottom_graph_y - graph_height, left_bottom_graph_x, left_bottom_graph_y, main_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(left_bottom_graph_x, left_bottom_graph_y, left_bottom_graph_x + graph_width, left_bottom_graph_y, main_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

void GraphPainter::drawAxisLabels(time_t *time_now) {
    drawYLabels();
    drawXLabels(time_now);
}

void GraphPainter::drawXLabels(time_t *time_now) {
    uint32_t start_time = *time_now - show_hours*60*60;
    // Serial.printf("start_time: %d\n\r", start_time);
    unsigned long seconds_in_day = start_time % 86400;  // Seconds since midnight
    // Serial.printf("seconds_in_day: %d\n\r", seconds_in_day);
    uint8_t start_hour = seconds_in_day / 3600 + 1;
    uint16_t start_seconds = seconds_in_day % 3600;
    uint32_t start_hour_time = 3600 - start_seconds;
    // Serial.printf("start_hour: %d, start_seconds: %d, start_hour_time: %d\n\r", start_hour, start_seconds, start_hour_time);
    for (int i = 0; i < 4; i++) {
        int timezone_offset = get_timezone_offset();
        int current_hour = start_hour + i * show_hours / 4 + timezone_offset;
        if (current_hour < 0) {
            current_hour = 24 + current_hour;
        } else if (current_hour > 24) {
            current_hour = current_hour - 24;
        }
        char label[5];
        snprintf(label, sizeof(label), "%02dh", current_hour);
        float time_span = (float)(*time_now - start_time); // Ensure float division
        float time_offset = (float)(start_hour_time + i * 3600 * show_hours / 4); // seconds from start

        float x_pos = (float)left_bottom_graph_x + (time_offset * graph_width) / time_span;
        uint16_t x = (uint16_t)x_pos;
        // Serial.printf("time_span: %f, time_offset: %f, x_pos: %f\n\r", time_span, time_offset, x_pos);
        uint16_t y = left_bottom_graph_y + 5;
        Paint_DrawString_EN(x - digitFont.Width * 3 / 2, y, label, &digitFont, background_color, main_color);
        Paint_DrawLine(x, left_bottom_graph_y - 2, x, left_bottom_graph_y + 2, main_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }
}

void GraphPainter::drawYLabels() {
    for (int i = 0; i < ticks; i++) {
        float val = graph_min + i * step;
        uint16_t y = left_bottom_graph_y - (graph_height * (val - graph_min) / range);
        if (y >= (left_bottom_graph_y - graph_height)) {
            Paint_DrawLine(left_bottom_graph_x - 2, y, left_bottom_graph_x + 2, y, main_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            char label[16];
            stringFromFloat(label, val);
            Paint_DrawString_EN(left_bottom_x, y - digitFont.Height / 2, label, &digitFont, background_color, main_color);
        }
    }
}

void GraphPainter::calculateYLabelWidth() {
    char label[16];
    int max_label_pixel_width = 0;

    for (int i = 0; i < ticks; i++) {
        float val = graph_min + i * step;
        snprintf(label, sizeof(label), "%.2f", val);

        // Trim trailing zeros
        char* dot = strchr(label, '.');
        if (dot) {
            char* end = label + strlen(label) - 1;
            while (end > dot && *end == '0') {
                *end-- = '\0';
            }
            if (*end == '.') *end = '\0';
        }

        int pixel_width = strlen(label) * digitFont.Width;
        if (pixel_width > max_label_pixel_width) {
            max_label_pixel_width = pixel_width;
        }
    }
    uint16_t label_height = labelFont.Height + 2;

    digit_width = max_label_pixel_width + 5;
    digit_height = digitFont.Height + 5;
    graph_width = width - digit_width;
    graph_height = height - digit_height - label_height;
    left_bottom_graph_x = left_bottom_x + digit_width;
    left_bottom_graph_y = left_bottom_y - digit_height;
}


float GraphPainter::nice_number(float value, int round) {
    int exponent = floorf(log10f(value));       // Exponent of value
    float fraction = value / powf(10, exponent); // Fractional part 1–10

    float nice_fraction;
    if (round) {
        if (fraction < 1.5)
            nice_fraction = 1;
        else if (fraction < 3)
            nice_fraction = 2;
        else if (fraction < 7)
            nice_fraction = 5;
        else
            nice_fraction = 10;
    } else {
        if (fraction <= 1)
            nice_fraction = 1;
        else if (fraction <= 2)
            nice_fraction = 2;
        else if (fraction <= 5)
            nice_fraction = 5;
        else
            nice_fraction = 10;
    }

    return nice_fraction * powf(10, exponent);
}

#endif
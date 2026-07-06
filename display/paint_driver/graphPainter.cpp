#ifdef ALTRUIST_INSIGHT

#include "graphPainter.h"
#include "utils.h"
#include "../utils.h"
#include "fonts/fonts.h"
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

void GraphPainter::setUpdateNote(const char* note) {
    if (note == nullptr || note[0] == '\0') {
        update_note[0] = '\0';
        return;
    }
    snprintf(update_note, sizeof(update_note), "%s", note);
}

void GraphPainter::addLineValues(float* values, uint32_t* timestamps, int values_count, const char* label) {
    addLineValues(values, timestamps, values_count, label, GraphLineStyle{});
}

void GraphPainter::addLineValues(float* values, uint32_t* timestamps, int values_count, const char* label, GraphLineStyle line_style) {
    if (lines_count < MAX_LINES) {
        lines[lines_count++] = { values, timestamps, values_count, label, line_style };
    }
}

void GraphPainter::setShowHours(uint8_t hours) {
    // Clamp between 1 and 12 hours, and ensure it's divisible by 4 for x-axis labels
    if (hours < 1) hours = 1;
    if (hours > 12) hours = 12;
    // Round to nearest value divisible by 4, or use the exact value if <= 4
    if (hours > 4) {
        hours = ((hours + 2) / 4) * 4;  // Round to nearest multiple of 4
    }
    show_hours = hours;
}

void GraphPainter::calculateMinMax() {
    max_value = 0;
    min_value = 99999;
    
    // Check if all values are positive 
    // This is used to prevent negative y-axis for inherently positive data (like PM values)
    // BUT: if any value is negative (e.g., temperature below 0°C), all_positive will be false,
    // and the graph will correctly display negative values
    bool all_positive = true;
    
    for (int j = 0; j < lines_count; j++) {
        GraphLine& line = lines[j];
        for (int i = 0; i < line.values_count; i++) {
            if (line.values[i] > max_value) max_value = line.values[i];
            if (line.values[i] < min_value) min_value = line.values[i];
            if (line.values[i] < 0) {
                all_positive = false;  // Found negative value (e.g., -3°C), allow negative y-axis
            }
        }
    }
    
    // If no valid data, set default
    if (min_value > max_value || min_value == 99999) {
        min_value = 0;
        max_value = 100;
    }
    
    // Add safety margin so lines don't touch/cross graph borders
    if (max_value > min_value) {
        float rawRange = max_value - min_value;
        float padding;
        if (rawRange > 100.0f) {
            padding = rawRange * 0.05f;  // 5% for large ranges (like PM values 10-500)
        } else if (rawRange > 10.0f) {
            padding = rawRange * 0.08f;  // 8% for medium ranges
        } else {
            padding = rawRange * 0.1f;   // 10% for small ranges
        }
        if (padding < 1.0f) {
            padding = 1.0f;                
        }
        min_value -= padding;
        max_value += padding;
        
        // Don't let min_value go below 0 if all original values were >= 0
        if (all_positive && min_value < 0) {
            min_value = 0;
        }
    } else if (max_value == min_value) {
        // Handle case where all values are the same
        if (max_value >= 0) {
            min_value = 0;
            max_value = max_value + 1.0f;
        } else {
            min_value -= 1.0f;
            max_value += 1.0f;
        }
    }
    
    // For positive data, ensure min_value is at least 0 before calculating graph_min
    if (all_positive && min_value < 0) {
        min_value = 0;
    }
    
    range = nice_number(max_value - min_value, 0);
    if (range < 0.01f) range = 1.0f;  // Prevent division by zero or negative
    step = nice_number(range / (ticks - 1), 1);
    if (step < 0.01f) step = 1.0f;  // Ensure step is valid
    
    graph_min = floorf(min_value / step) * step;
    graph_max = graph_min + step * (ticks - 1);
    
    // Ensure graph_max is at least >= max_value (with extra margin for safety)
    while (graph_max < max_value) {
        graph_max += step;
    }
    if (graph_max - max_value < step * 0.5f) {
        graph_max += step;
    }
    
    // Adjust graph_min to be <= min_value, but NEVER below 0 for positive data
    while (graph_min > min_value) {
        float new_graph_min = graph_min - step;
        if (all_positive && new_graph_min < 0) {
            graph_min = 0;
            break;
        }
        graph_min = new_graph_min;
    }
    
    if (all_positive) {
        if (graph_min < 0) {
            graph_min = 0;
        }
    }
    
    // FINAL VERIFICATION: Ensure graph_max definitely covers max_value
    // This is critical - if max_value is higher than graph_max, lines will be cut off
    if (graph_max < max_value) {
        // Force graph_max to be at least max_value + one step for safety
        graph_max = ceilf((max_value + step) / step) * step;
        if (graph_max <= max_value) {
            graph_max += step;
        }
    }
    
    // CRITICAL: Store the required max_value before any step recalculation
    // We MUST ensure graph_max is always >= max_value to prevent overflow
    float required_max = max_value;
    
    // CRITICAL: Recalculate everything to ensure perfect alignment
    // We need graph_max to be exactly graph_min + (ticks - 1) * step
    // This ensures all tick positions are filled with no gaps
    
    // Calculate the minimum range needed (data range + safety margin)
    float safety_margin_steps = 1.5f;  // Use 1.5 steps as safety margin (reduced from 2.0)
    float required_range = required_max - graph_min;
    float target_range = required_range + (required_range * 0.1f);  // Add 10% padding
    
    // Calculate step that will give us nice numbers and fill all ticks
    float raw_step = target_range / (ticks - 1);
    step = nice_number(raw_step, 1);
    if (step < 0.01f) step = 1.0f;
    
    // Recalculate graph_max to be exactly aligned with ticks
    // This ensures the last tick (at index ticks-1) equals graph_max
    graph_max = graph_min + (ticks - 1) * step;
    
    // Ensure graph_max covers required_max with safety margin
    float required_max_with_margin = required_max + (step * safety_margin_steps);
    if (graph_max < required_max_with_margin) {
        // Need more steps - recalculate step to cover the required range
        float min_range_needed = required_max_with_margin - graph_min;
        float min_step_needed = min_range_needed / (ticks - 1);
        step = nice_number(min_step_needed, 1);
        // If nice_number rounded down, round up to ensure coverage
        if (step < min_step_needed) {
            // Try rounding up by using a slightly larger multiplier
            step = nice_number(min_step_needed * 1.2f, 1);
        }
        if (step < 0.01f) step = 1.0f;
        
        // Recalculate graph_max with the new step
        graph_max = graph_min + (ticks - 1) * step;
        
        // Final safety check - if still not enough, add one more step
        if (graph_max < required_max_with_margin) {
            graph_max += step;
            // Recalculate step to maintain alignment
            step = (graph_max - graph_min) / (ticks - 1);
        }
    }
    
    // Final range recalculation
    range = graph_max - graph_min;
    
    // Verify alignment: graph_max should equal graph_min + (ticks-1) * step
    float expected_max = graph_min + (ticks - 1) * step;
    if (fabsf(graph_max - expected_max) > 0.01f) {
        // Force alignment
        graph_max = expected_max;
        range = graph_max - graph_min;
    }
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
    // Show only the update cadence note (no legend/value text).
    const char* hourly_note = "hourly update";
    char label_buf[96];
    if (update_note[0] != '\0') {
        // Prefer "<value> (hourly update)" so the important part is first.
        snprintf(label_buf, sizeof(label_buf), "%s (%s)", update_note, hourly_note);
    } else {
        snprintf(label_buf, sizeof(label_buf), "%s", hourly_note);
    }

    uint16_t text_width = Paint_GetStringWidth_Display(label_buf, &labelFont, &font_12_cyrillic, &font_12_ascii);
    // Right-align so it sits near the global sidebar (instead of centered).
    const uint16_t right_pad = 2;
    uint16_t x = left_bottom_x;
    if (text_width + right_pad < width) {
        x = left_bottom_x + (width - text_width - right_pad);
    }

    uint16_t y = left_bottom_graph_y - graph_height - labelFont.Height - 4;
    Paint_DrawString_Display(x, y, label_buf, &labelFont, &font_12_cyrillic, &font_12_ascii, background_color, main_color);
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
    
    // Add padding so lines don't touch graph borders
    const uint16_t x_padding_left = 4;   // Smaller gap on left
    const uint16_t x_padding_right = 2;  // Small gap on right
    const uint16_t y_padding = 2;
    uint16_t padded_graph_x = left_bottom_graph_x + x_padding_left;
    uint16_t padded_graph_width = graph_width - x_padding_left - x_padding_right;
    uint16_t padded_graph_height = graph_height - 2 * y_padding;
    uint16_t padded_graph_bottom = left_bottom_graph_y - y_padding;
    uint16_t padded_graph_top = padded_graph_bottom - padded_graph_height;
    
    bool first_visible_point = true;
    
    for (int i = 0; i < lines[line_number].values_count; i++) {
        if (lines[line_number].timestamps[i] < start_time) continue;
        
        // Normal orientation: old data on left, new data ("now") on right
        uint16_t x = padded_graph_x + (lines[line_number].timestamps[i] - start_time) * padded_graph_width / (*time_now - start_time);
        if (first_visible_point) {
            x = padded_graph_x;
            first_visible_point = false;
        }
        if (x < padded_graph_x) x = padded_graph_x;
        if (x > padded_graph_x + padded_graph_width) x = padded_graph_x + padded_graph_width;
        
        // Calculate y coordinate, ensuring it stays within padded bounds
        // This prevents any possibility of overflow
        float clamped_value = lines[line_number].values[i];
        if (clamped_value > graph_max) clamped_value = graph_max;
        if (clamped_value < graph_min) clamped_value = graph_min;
        
        // Calculate y position within the padded graph area
        // CRITICAL: Use actual graph_max - graph_min for range, not the class member 'range'
        // This ensures perfect alignment with the graph boundaries
        float actual_range = graph_max - graph_min;
        if (actual_range < 0.01f) actual_range = 1.0f;  // Prevent division by zero
        
        // Calculate normalized position (0.0 = graph_min at bottom, 1.0 = graph_max at top)
        float normalized = (clamped_value - graph_min) / actual_range;
        if (normalized < 0.0f) normalized = 0.0f;
        if (normalized > 1.0f) normalized = 1.0f;
        
        // Calculate y coordinate (inverted: higher values = higher on screen)
        // Use floating point calculation first, then convert to uint16_t
        float y_float = (float)padded_graph_bottom - (normalized * (float)padded_graph_height);
        uint16_t y = (uint16_t)y_float;
        
        // Ensure y NEVER goes beyond padded bounds
        // This is the last line of defense - clamp BEFORE any drawing
        if (y > padded_graph_bottom) y = padded_graph_bottom;
        if (y < padded_graph_top) y = padded_graph_top;
        
        if (i == 0) {
            Paint_DrawPoint(x, y, line_color, DOT_PIXEL_1X1, DOT_STYLE_DFT);
        } else {
            // Calculate y_prev with same strict logic
            float clamped_value_prev = lines[line_number].values[i - 1];
            if (clamped_value_prev > graph_max) clamped_value_prev = graph_max;
            if (clamped_value_prev < graph_min) clamped_value_prev = graph_min;
            
            // Use actual graph_max - graph_min for range 
            float actual_range_prev = graph_max - graph_min;
            if (actual_range_prev < 0.01f) actual_range_prev = 1.0f;
            
        
            float normalized_prev = (clamped_value_prev - graph_min) / actual_range_prev;
            if (normalized_prev < 0.0f) normalized_prev = 0.0f;
            if (normalized_prev > 1.0f) normalized_prev = 1.0f;
            
            // Use floating point calculation first, then convert to uint16_t
            float y_prev_float = (float)padded_graph_bottom - (normalized_prev * (float)padded_graph_height);
            uint16_t y_prev = (uint16_t)y_prev_float;
            
            // Ensure y_prev NEVER goes beyond padded bounds
            if (y_prev > padded_graph_bottom) y_prev = padded_graph_bottom;
            if (y_prev < padded_graph_top) y_prev = padded_graph_top;

            // FINAL SAFETY CHECK: Ensure both coordinates are within bounds before drawing
            // This prevents any possibility of drawing outside the graph area
            if (x_prev >= padded_graph_x && x_prev <= padded_graph_x + padded_graph_width &&
                x >= padded_graph_x && x <= padded_graph_x + padded_graph_width &&
                y_prev >= padded_graph_top && y_prev <= padded_graph_bottom &&
                y >= padded_graph_top && y <= padded_graph_bottom) {
                Paint_DrawLine(x_prev, y_prev, x, y, line_color, lines[line_number].line_style.width, lines[line_number].line_style.style);
            } else {
                // If coordinates are out of bounds, clamp them one more time and draw
                // This should never happen, but it's a safety net
                if (x_prev < padded_graph_x) x_prev = padded_graph_x;
                if (x_prev > padded_graph_x + padded_graph_width) x_prev = padded_graph_x + padded_graph_width;
                if (x < padded_graph_x) x = padded_graph_x;
                if (x > padded_graph_x + padded_graph_width) x = padded_graph_x + padded_graph_width;
                if (y_prev < padded_graph_top) y_prev = padded_graph_top;
                if (y_prev > padded_graph_bottom) y_prev = padded_graph_bottom;
                if (y < padded_graph_top) y = padded_graph_top;
                if (y > padded_graph_bottom) y = padded_graph_bottom;
                Paint_DrawLine(x_prev, y_prev, x, y, line_color, lines[line_number].line_style.width, lines[line_number].line_style.style);
            }
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
    float time_span = (float)(*time_now - start_time);
    uint16_t max_x = left_bottom_graph_x + graph_width;  
    uint16_t y = left_bottom_graph_y + 5;
    
    // Add padding from right edge to prevent overflow into sidebar
    const uint16_t right_padding = 5;
    uint16_t safe_max_x = max_x - right_padding;
    
    char prev_label[8] = "";  // Track previous label to avoid duplicates
    
    for (int i = 0; i < 4; i++) {
        // Calculate time offset: i=0 is start (oldest), i=3 is "now" (newest)
        float time_offset = (i * time_span / 3.0f);
        
        float x_pos = (float)left_bottom_graph_x + (time_offset * graph_width) / time_span;
        uint16_t x = (uint16_t)x_pos;
        
        if (x > safe_max_x) x = safe_max_x;
        
        char label[8];
        if (i == 3) {
            // Last label shows "now"
            snprintf(label, sizeof(label), "now");
        } else if (i == 0) {
            // First label shows the full time span (on the left)
            snprintf(label, sizeof(label), "%dh", show_hours);
        } else {
            // Show hours ago: for 12h span, show 8h, 4h
            // Use floating point to avoid integer division issues
            float hours_ago_float = show_hours - (i * show_hours / 3.0f);
            int hours_ago = (int)(hours_ago_float + 0.5f);  // Round to nearest
            if (hours_ago < 1) hours_ago = 1;  // At least 1 hour
            snprintf(label, sizeof(label), "%dh", hours_ago);
        }
        
        // Skip this label if it's the same as the previous one
        if (i > 0 && strcmp(label, prev_label) == 0) {
            // Still draw the tick mark, just skip the label
            Paint_DrawLine(x, left_bottom_graph_y - 2, x, left_bottom_graph_y + 2, main_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            continue;
        }
        
        strcpy(prev_label, label);  // Remember this label for next iteration
        
        uint16_t label_width = digitFont.Width * strlen(label);
        uint16_t label_x = x - label_width / 2; 

        if (label_x < left_bottom_graph_x) {
            label_x = left_bottom_graph_x;
        } else if (label_x + label_width > safe_max_x) {
            label_x = safe_max_x - label_width;
        }
        
        // Draw tick mark
        Paint_DrawLine(x, left_bottom_graph_y - 2, x, left_bottom_graph_y + 2, main_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        
        // Draw label
        Paint_DrawString_EN(label_x, y, label, &digitFont, background_color, main_color);
    }
}

void GraphPainter::drawYLabels() {
    // Use actual graph range for accurate positioning
    float actual_range = graph_max - graph_min;
    if (actual_range < 0.01f) actual_range = 1.0f;  // Prevent division by zero
    
    // Draw labels for ALL ticks (0 to ticks-1), ensuring no gaps
    // The last tick (i = ticks-1) should equal graph_max
    for (int i = 0; i < ticks; i++) {
        float val = graph_min + i * step;
        
        // For the last tick, use graph_max to ensure it's exactly at the top
        if (i == ticks - 1) {
            val = graph_max;
        }
        
        // Calculate y position using actual range to ensure accurate placement
        float normalized = (val - graph_min) / actual_range;
        if (normalized < 0.0f) normalized = 0.0f;
        if (normalized > 1.0f) normalized = 1.0f;
        
        uint16_t y = left_bottom_graph_y - (uint16_t)(normalized * graph_height);
        
        uint16_t graph_top = left_bottom_graph_y - graph_height;
        if (y < graph_top) y = graph_top;
        if (y > left_bottom_graph_y) y = left_bottom_graph_y;
        
        // Draw tick mark
        Paint_DrawLine(left_bottom_graph_x - 2, y, left_bottom_graph_x + 2, y, main_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        
        // Draw label - round to nearest integer for whole numbers only
        char label[16];
        int rounded_val = (int)(val + 0.5f);  // Round to nearest integer
        snprintf(label, sizeof(label), "%d", rounded_val);
        Paint_DrawString_EN(left_bottom_x, y - digitFont.Height / 2, label, &digitFont, background_color, main_color);
    }
}

void GraphPainter::calculateYLabelWidth() {
    char label[16];
    int max_label_pixel_width = 0;

    for (int i = 0; i < ticks; i++) {
        float val = graph_min + i * step;
        // Round to nearest integer for whole numbers only
        int rounded_val = (int)(val + 0.5f);
        snprintf(label, sizeof(label), "%d", rounded_val);

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
    int exponent = floorf(log10f(value));       
    float fraction = value / powf(10, exponent);

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
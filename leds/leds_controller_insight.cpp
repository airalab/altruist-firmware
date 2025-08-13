#ifdef ALTRUIST_INSIDE

#include "leds_controller_insight.h"
#include "../utils.h"
#include "../config_manager/config_helpers.h"


void LedControllerInsight::init() {
    if (LED_PIN != -1 && cfg::leds_on) {
        pixels.begin();
        pixels.clear();
        uint8_t brightness;
        if (cfg::leds_brightness * 255 / 100 < 0) {
            brightness = 0;
        } else if (cfg::leds_brightness * 255 / 100 > 255) {
            brightness = 255;
        } else {
            brightness = cfg::leds_brightness * 255 / 100;
        }
        pixels.setBrightness(brightness);
        pixels.show();
        debug_outln_info(F("Setup leds on pin "), LED_PIN);
    } else {
        debug_outln_info(F("Will not setup leds on pin "), LED_PIN);
    }
    last_refresh_time = millis() - REFRESH_INTERVAL; 
}

void LedControllerInsight::process() {
    if (LED_PIN == -1 || !cfg::leds_on) {
        return;
    }
    uint32_t color;
    if (msSince(last_refresh_time) > REFRESH_INTERVAL) {
        last_refresh_time = millis();
        _setAllPixels(pixels.Color(255, 255, 255));
        if (sensors_data.containsKey(ATRUIST_URBAN_SENSOR)) {
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("SDS_P1") && sensors_data[ATRUIST_URBAN_SENSOR].containsKey("SDS_P2")) {
                color = _getPMColor(sensors_data[ATRUIST_URBAN_SENSOR]["SDS_P1"]["value"].as<float>(), sensors_data[ATRUIST_URBAN_SENSOR]["SDS_P2"]["value"].as<float>());
                _setPartColor(1, 3, color);
                debug_outln_info(F("Set PM color "), getColorName(color));
            }
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("PCBA_noiseAvg")) {
                color = _getNoiseColor(sensors_data[ATRUIST_URBAN_SENSOR]["PCBA_noiseAvg"]["value"].as<float>());
                _setPartColor(4, 6, color);
                debug_outln_info(F("Set Noise color "), getColorName(color));
            }
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("BME280_temperature")) {
                color = _getTempColor(sensors_data[ATRUIST_URBAN_SENSOR]["BME280_temperature"]["value"].as<float>());
                _setPartColor(10, 12, color);
                debug_outln_info(F("Set Temp color "), getColorName(color));
            }
        }
        if (sensors_data.containsKey("SCD4x")) {
            color = _getCO2Color(sensors_data["SCD4x"]["co2"]["value"].as<float>());
            _setPartColor(7, 9, color);
            debug_outln_info(F("Set CO2 color "), getColorName(color));
        }
        pixels.show();
    }

}

void LedControllerInsight::_setAllPixels(uint32_t color) {
    for (int pixel = 0; pixel < LED_COUNT; pixel++) {
        pixels.setPixelColor(pixel, color);
    }
}

void LedControllerInsight::_setPartColor(uint8_t start_led, uint8_t end_led, uint32_t color) {
    for (int pixel = start_led - 1; pixel < end_led; pixel++) {
        pixels.setPixelColor(pixel, color);
    }
}

uint32_t LedControllerInsight::_getPMColor(float pm10, float pm25) {
    if (pm10 < 50 && pm25 < 36) {
        return getColor(ColorName::GREEN_LED);
    } else if (pm10 < 100 && pm25 < 70) {
        return getColor(ColorName::BLUE_LED);
    } else if (pm10 < 250 && pm25 < 150) {
        return getColor(ColorName::ORANGE_LED);
    } else if (pm10 < 350 && pm25 < 250) {
        return getColor(ColorName::RED_LED);
    } else {
        return getColor(ColorName::PURPLE_LED);
    }
}

uint32_t LedControllerInsight::_getNoiseColor(float noise) {
    if (noise < 50) {
        return getColor(ColorName::GREEN_LED);
    } else if (noise < 70) {
        return getColor(ColorName::BLUE_LED);
    } else if (noise < 80) {
        return getColor(ColorName::ORANGE_LED);
    } else if (noise < 100) {
        return getColor(ColorName::RED_LED);
    } else {
        return getColor(ColorName::PURPLE_LED);
    }
}

uint32_t LedControllerInsight::_getCO2Color(float co2) {
    if (co2 < 800) {
        return getColor(ColorName::GREEN_LED);
    } else if (co2 < 1000) {
        return getColor(ColorName::BLUE_LED);
    } else if (co2 < 2500) {
        return getColor(ColorName::ORANGE_LED);
    } else if (co2 < 5000) {
        return getColor(ColorName::RED_LED);
    } else {
        return getColor(ColorName::PURPLE_LED);
    }
}

uint32_t LedControllerInsight::_getTempColor(float temperature) {
    if (temperature < -9) {
        return getColor(ColorName::PURPLE_LED);
    } else if (temperature < 10) {
        return getColor(ColorName::BLUE_LED);
    } else if (temperature < 25) {
        return getColor(ColorName::GREEN_LED);
    } else {
        return getColor(ColorName::ORANGE_LED);
    }
}

#endif
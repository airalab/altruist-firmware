#ifdef ALTRUIST_INSIGHT

#ifndef GRAPH_SCREEN_H
#define GRAPH_SCREEN_H

#include <Arduino.h>
#include "../driver/EPD.h"

#define GRAPH_HEIGHT (DISPLAY_HEIGHT - 40)/2
#define GRAPH_WIDTH (DISPLAY_WIDTH - 40)/2

// Order of values when cycling on the graphs screen:
enum class GraphValue : uint8_t {
    INSIGHT_TEMP = 0,
    INSIGHT_HUM,
    INSIGHT_CO2,
    INSIGHT_PRESSURE,
    URBAN_AIR,
    URBAN_NOISE,
    URBAN_TEMP,
    URBAN_HUM,
    URBAN_PRESSURE
};

extern uint8_t current_graph_screen;

void drawGraphScreen();
void setNextGraphScreen();
void setPrevGraphScreen();

// Cycle through graph values on the GRAPHS screen
// Returns true if we've reached the last graph (should switch to next screen)
bool setNextGraphValue();
bool setPrevGraphValue();

// Check if graphs are available (SD card present and data files exist)
// Returns true if graphs can be displayed, false otherwise
bool areGraphsAvailable();

// Cached during drawGraphScreen(); safe to read from button handler (no SD I/O).
bool graphsNavigationCanCycle();
/** True when graph draw should run again after SD was temporarily busy. */
bool graphScreenSdRetryDue();
/** Call before refresh when user cycles temp/humidity/etc. on the graphs screen. */
void graphMarkValueSwitch();
/** Invalidate cached SD probe (card removed or probe stale). */
void graphInvalidateSdProbeCache();
/** Entering graphs from another screen — reset SD retry state. */
void graphClearDeferOnScreenEntry();

#endif
#endif
#ifdef ALTRUIST_INSIDE

#ifndef GRAPH_SCREEN_H
#define GRAPH_SCREEN_H

#include <Arduino.h>
#include "../driver/EPD.h"

#define GRAPH_HEIGHT (DISPLAY_HEIGHT - 40)/2
#define GRAPH_WIDTH (DISPLAY_WIDTH - 40)/2

extern uint8_t current_graph_screen;

void drawGraphScreen();
void setNextGraphScreen();
void setPrevGraphScreen();

#endif
#endif
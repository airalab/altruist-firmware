#ifdef ALTRUIST_INSIGHT

#ifndef _DESK_SCREEN_H
#define _DESK_SCREEN_H

#include <Arduino.h>
#include "main_screen.h"

uint8_t deskEffectivePagesMask();
uint8_t deskEnabledCount();
uint8_t deskPageIdAt(uint8_t slot);
uint8_t deskHomeSlot();
unsigned deskIntervalMs();
void deskNormalizeConfig();

void drawDeskMainBody(const main_screen_values_t &values,
                      uint16_t body_top, uint16_t content_left, uint16_t content_right,
                      uint8_t page_id, uint8_t pos_1based, uint8_t total);

#endif
#endif

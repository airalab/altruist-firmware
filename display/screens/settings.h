#ifdef ALTRUIST_INSIGHT

#ifndef _SETTINGS_SCREEN_H
#define _SETTINGS_SCREEN_H

#include "../driver/EPD.h"
#include "../../utils.h"

void showSettingsPage(UBYTE *BlackImage, device_status_t &deviceStatus, const String &urban_ip = "", const String &robonomics_address = "");

#endif // _SETTINGS_SCREEN_H

#endif
#ifdef ALTRUIST_INSIGHT

#ifndef _OTA_UPDATE_H
#define _OTA_UPDATE_H

#include "../driver/EPD.h"

struct device_status_t;
void showOTAUpdatePage(UBYTE *BlackImage, const device_status_t &deviceStatus);
void showOTAFailedPage(UBYTE *BlackImage);
void showOTASuccessPage(UBYTE *BlackImage);

#endif
#endif

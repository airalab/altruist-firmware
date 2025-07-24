#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include "webserver/webserver.h"

bool connectWifi(SensorWebServer &webserver);
void wifiConfig(SensorWebServer &webserver);

#endif // __WIFI_MANAGER_H__
#ifdef ALTRUIST_INSIGHT

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <stdlib.h>
#include <ArduinoJson.h>
#include <freertos/semphr.h>
#include "paint_driver/GUI_Paint.h"
#include "driver/EPD.h"
#include "driver/DEV_Config.h"
#include "screens/main_screen.h"
#include "screens/analytics.h"
#include "../utils.h"
#include "../buttons/button_manager.h"

#define DISPLAY_REFRESH_INTERVAL 60000L  // 1 minute (was 5 minutes)
#define OTA_DISPLAY_REFRESH_INTERVAL 15000L   // 15 seconds - OTA screen shows progress during download
#define OTA_FULL_REFRESH_EVERY_N 5            // Every 5th OTA refresh is FULL (1-4: partial, 5: full)

enum class ScreenPage {
    MAIN,
    ANALYTICS,
    GRAPHS,
    CONNECTING,
    SETUP,
    LOADING,
    LOGO,
    SENSOR_MAP,
    SETTINGS
};

class DisplayManager {
public:
    DisplayManager(JsonDocument &_data, device_status_t &_deviceStatus, SemaphoreHandle_t _mutex) 
        : sensors_data(_data), deviceStatus(_deviceStatus), mutex(_mutex) {}

    void setup();
    void process(button_pressed_t &btn_press);
    void setScreen(ScreenPage pageID);
    void setRobonomicsAddress(const char *address) { robonomics_address = String(address);};
    /** Request a full e-paper refresh on the next display cycle. */
    void requestEpdFullRefresh();
    /** Block until "WiFi credentials cleared" is painted (SET+DOWN long). */
    void requestWifiClearConfirmScreen();
    /** Forget SPIFFS + RAM Urban SS58 used for MAIN/SENSOR_MAP QR. */
    void clearUrbanCache();
private:
    device_status_t &deviceStatus;
    JsonDocument &sensors_data;
    SemaphoreHandle_t mutex;
    bool refresh_now = false;
    String robonomics_address;
    String cached_urban_address; // cached Urban Robonomics address once discovered
    UBYTE *BlackImage = nullptr;

    ScreenPage currentScreenID = ScreenPage::MAIN;
    unsigned long last_refresh_time = -DISPLAY_REFRESH_INTERVAL;

    // Watchdog-style safety: periodically force a full EPD re-init and
    // full refresh, in case the panel/driver gets into a bad state while
    // the rest of the device keeps running.
    unsigned long last_epd_reinit_time_ms = 0;
    // Watchdog interval: how often we proactively re-init the EPD driver.
    // With 25 minutes, we very rarely add extra FULL refreshes, but still
    // occasionally "kick" a stuck panel back to life.
    static constexpr unsigned long EPD_REINIT_INTERVAL_MS = 25UL * 60UL * 1000UL; // 25 minutes

    // Screen navigation helpers
    ScreenPage getNextScreen(ScreenPage current);
    ScreenPage getPrevScreen(ScreenPage current);

    // Wake-up loading screen state (show loading for 12s, then check WiFi)
    bool wake_loading_active = false;
    uint32_t wake_loading_deadline_ms = 0;
    static constexpr uint32_t WAKE_LOADING_DURATION_MS = 12000; // 12 seconds
    
    // Force full refresh on next update (e.g., after wake from sleep)
    bool force_full_refresh = false;

    bool wifi_clear_confirm_pending = false;

    // Retry updating Sensor Map QR when Urban address not yet available
    bool     sensor_map_waiting_addr   = false;
    uint32_t next_sensor_map_check_ms  = 0;
    uint8_t  sensor_map_waiting_tries  = 0;

    // Cached values for MAIN screen - reused when mutex is busy.
    // Keeping this snapshot avoids expensive JSON serialization under mutex.
    main_screen_values_t cached_main_values;
    analytics_screen_values_t cached_analytics_values;
    unsigned long last_analytics_stats_refresh_ms = 0;
    static constexpr unsigned long ANALYTICS_STATS_REFRESH_INTERVAL_MS = 5UL * 60UL * 1000UL; // 5 minutes
    static constexpr unsigned long ANALYTICS_STATS_DEFER_ON_ENTER_MS = 10UL * 1000UL; // show screen first, then refresh stats

    // OTA screen: last time we displayed it (0 = not yet shown this session)
    unsigned long last_ota_display_ms = 0;
    // OTA refresh counter: 1-4 = partial, 5 = full (then reset)
    uint8_t ota_display_refresh_count = 0;
};

#endif // DISPLAY_MANAGER_H

#endif
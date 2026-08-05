#ifndef intl_h
#define intl_h

#define REGION_GLOBAL "Global"
#define REGION_RU "RU"
// Legacy region ids (migrated to Global on config load)
#define REGION_EU "Europe"
#define REGION_AS "Asia"
#define REGION_AF "Africa"
#define REGION_AU "Australia"
#define REGION_NA "NorthAmerica"
#define REGION_SA "SouthAmerica"

#if defined(INTL_BG)
#include "translations/intl_bg.h"
#elif defined(INTL_CZ)
#include "translations/intl_cz.h"
#elif defined(INTL_DE)
#include "translations/intl_de.h"
#elif defined(INTL_DK)
#include "translations/intl_dk.h"
#elif defined(INTL_EN)
#include "translations/intl_en.h"
#elif defined(INTL_ES)
#include "translations/intl_es.h"
#elif defined(INTL_FR)
#include "translations/intl_fr.h"
#elif defined(INTL_HU)
#include "translations/intl_hu.h"
#elif defined(INTL_IT)
#include "translations/intl_it.h"
#elif defined(INTL_LU)
#include "translations/intl_lu.h"
#elif defined(INTL_NL)
#include "translations/intl_nl.h"
#elif defined(INTL_PL)
#include "translations/intl_pl.h"
#elif defined(INTL_PT)
#include "translations/intl_pt.h"
#elif defined(INTL_RS)
#include "translations/intl_rs.h"
#elif defined(INTL_RU)
#include "translations/intl_ru.h"
#elif defined(INTL_SE)
#include "translations/intl_se.h"
#elif defined(INTL_SK)
#include "translations/intl_sk.h"
#elif defined(INTL_TR)
#include "translations/intl_tr.h"
#elif defined(INTL_UA)
#include "translations/intl_ua.h"
#else
#warning No language defined
#include "translations/intl_en.h"
#endif

#ifndef INTL_DISP_ANALYTICS_GRADE
#define INTL_DISP_ANALYTICS_GRADE "Grade"
#endif
#ifndef INTL_DISP_ANALYTICS_COL_METRIC
#define INTL_DISP_ANALYTICS_COL_METRIC "Metric"
#endif
#ifndef INTL_DISP_ANALYTICS_COL_MAX
#define INTL_DISP_ANALYTICS_COL_MAX "Max"
#endif
#ifndef INTL_DISP_ANALYTICS_COL_MIN
#define INTL_DISP_ANALYTICS_COL_MIN "Min"
#endif
#ifndef INTL_DISP_ANALYTICS_COL_CONSERV
#define INTL_DISP_ANALYTICS_COL_CONSERV "Conserv"
#endif
#ifndef INTL_DISP_ANALYTICS_COL_BIOHACK
#define INTL_DISP_ANALYTICS_COL_BIOHACK "Biohack"
#endif
#ifndef INTL_DISP_ANALYTICS_ROW_CO2
#define INTL_DISP_ANALYTICS_ROW_CO2 "CO2 ppm"
#endif
#ifndef INTL_DISP_ANALYTICS_ROW_TEMP
#define INTL_DISP_ANALYTICS_ROW_TEMP "Temperature C"
#endif
#ifndef INTL_DISP_ANALYTICS_ROW_HUM
#define INTL_DISP_ANALYTICS_ROW_HUM "Humidity %"
#endif
#ifndef INTL_DISP_ANALYTICS_ROW_PM25
#define INTL_DISP_ANALYTICS_ROW_PM25 "PM2.5 ug/m3"
#endif
#ifndef INTL_DISP_ANALYTICS_ROW_NOISE
#define INTL_DISP_ANALYTICS_ROW_NOISE "Noise dB"
#endif
#ifndef INTL_DISP_ANALYTICS_AT
#define INTL_DISP_ANALYTICS_AT "at"
#endif
#ifndef INTL_DISP_ANALYTICS_HOUR_SUFFIX
#define INTL_DISP_ANALYTICS_HOUR_SUFFIX "h"
#endif

#ifndef INTL_DISP_INFO_LABEL
#define INTL_DISP_INFO_LABEL "Info:"
#endif
#ifndef INTL_DISP_LEVEL_HIGH
#define INTL_DISP_LEVEL_HIGH "high"
#endif
#ifndef INTL_DISP_LEVEL_LOW
#define INTL_DISP_LEVEL_LOW "low"
#endif
#ifndef INTL_DISP_IS_TOO
#define INTL_DISP_IS_TOO "is too"
#endif
#ifndef INTL_DISP_CHECK_MAP_FULL_DATA
#define INTL_DISP_CHECK_MAP_FULL_DATA "Check out our sensor map for full data and analytics."
#endif
#ifndef INTL_DISP_DEW_POINT_U_PREFIX
#define INTL_DISP_DEW_POINT_U_PREFIX "Dew Point (U): "
#endif
#ifndef INTL_DISP_DEW_POINT_IS
#define INTL_DISP_DEW_POINT_IS "Dew Point is "
#endif
#ifndef INTL_DISP_TEMP_SHORT
#define INTL_DISP_TEMP_SHORT "Temp"
#endif
#ifndef INTL_DISP_PRESS_SHORT
#define INTL_DISP_PRESS_SHORT "Press."
#endif
#ifndef INTL_STANDALONE_SHOP_PROMPT
#define INTL_STANDALONE_SHOP_PROMPT "Add more measurements to your home"
#endif
#ifndef INTL_STANDALONE_INSIGHT_FOOTER_PROMPT
#define INTL_STANDALONE_INSIGHT_FOOTER_PROMPT \
    "Add noise, air dust and outdoor atmospheric measurements to your Insight."
#endif
#ifndef INTL_DISP_NOISE_AVGMAX_SUFFIX
#define INTL_DISP_NOISE_AVGMAX_SUFFIX "(avg/max)"
#endif
#ifndef INTL_DISP_EXPLORE_ADVANTAGES
#define INTL_DISP_EXPLORE_ADVANTAGES "Explore all advantages"
#endif
#ifndef INTL_DISP_EXPLORE_ENVIRONMENT
#define INTL_DISP_EXPLORE_ENVIRONMENT "Explore your environment"
#endif
#ifndef INTL_DISP_EXPLORE_YOUR
#define INTL_DISP_EXPLORE_YOUR "Explore your"
#endif
#ifndef INTL_DISP_ENVIRONMENT_CAPS
#define INTL_DISP_ENVIRONMENT_CAPS "ENVIRONMENT"
#endif
#ifndef INTL_DISP_SENSORS_MAP
#define INTL_DISP_SENSORS_MAP "SENSORS MAP"
#endif
#ifndef INTL_DISP_POWERED
#define INTL_DISP_POWERED "Powered"
#endif
#ifndef INTL_DISP_BY_ROBONOMICS
#define INTL_DISP_BY_ROBONOMICS "by Robonomics"
#endif
#ifndef INTL_DISP_MAP_ENV_BETTER
#define INTL_DISP_MAP_ENV_BETTER "Know your environment better."
#endif
#ifndef INTL_DISP_MAP_REVIEW_INSIGHTS
#define INTL_DISP_MAP_REVIEW_INSIGHTS "Review insights over time."
#endif
#ifndef INTL_DISP_MAP_COMPARE_CONDITIONS
#define INTL_DISP_MAP_COMPARE_CONDITIONS "Compare with others nearby."
#endif
#ifndef INTL_GUEST_CONNECTED
#define INTL_GUEST_CONNECTED "Connected"
#endif
#ifndef INTL_GUEST_CONNECTING
#define INTL_GUEST_CONNECTING "Connecting to Wi‑Fi…"
#endif
#ifndef INTL_GUEST_CONNECTING_HINT
#define INTL_GUEST_CONNECTING_HINT "This usually takes a few seconds. Keep this page open."
#endif
#ifndef INTL_GUEST_CONNECT_FAILED
#define INTL_GUEST_CONNECT_FAILED "Connection failed"
#endif
#ifndef INTL_GUEST_CONNECT_FAILED_HINT
#define INTL_GUEST_CONNECT_FAILED_HINT "Check the network name and password, then try again."
#endif
#ifndef INTL_GUEST_CONNECT_FAILED_INSIGHT
#define INTL_GUEST_CONNECT_FAILED_INSIGHT "Check the network name and password, then try again from the setup page."
#endif
#ifndef INTL_GUEST_WIFI_STEP_TITLE
#define INTL_GUEST_WIFI_STEP_TITLE "Wi-Fi connected"
#endif
#ifndef INTL_GUEST_SETUP_STEP_1_LABEL
#define INTL_GUEST_SETUP_STEP_1_LABEL "Step 1 of 2"
#endif
#ifndef INTL_GUEST_SETUP_STEP_2_LABEL
#define INTL_GUEST_SETUP_STEP_2_LABEL "Step 2 of 2"
#endif
#ifndef INTL_GUEST_SETUP_STEP_1_TITLE
#define INTL_GUEST_SETUP_STEP_1_TITLE "Connect to Wi-Fi"
#endif
#ifndef INTL_GUEST_INSIGHT_FINISH_HINT
#define INTL_GUEST_INSIGHT_FINISH_HINT "Press Continue to finish setup and restart the device."
#endif
#ifndef INTL_GUEST_INSIGHT_AUTO_FINISH_HINT
#define INTL_GUEST_INSIGHT_AUTO_FINISH_HINT "If you don't press Continue, setup will finish alone (standalone) in"
#endif
#ifndef INTL_GUEST_INSIGHT_AUTO_FINISH_SUFFIX
#define INTL_GUEST_INSIGHT_AUTO_FINISH_SUFFIX "seconds (standalone mode)."
#endif
#ifndef INTL_GUEST_IP_ADDRESS
#define INTL_GUEST_IP_ADDRESS "IP Address:"
#endif
#ifndef INTL_GUEST_OPEN_IP_HINT
#define INTL_GUEST_OPEN_IP_HINT "Copy the IP address and open it in your browser."
#endif
#ifndef INTL_GUEST_DEVICE_INFO_HINT
#define INTL_GUEST_DEVICE_INFO_HINT "On iPhone: tap Save → Share → Save to Files. On Android it can go to Downloads. Or copy as text."
#endif
#ifndef INTL_GUEST_DEVICE_INFO_DOWNLOAD
#define INTL_GUEST_DEVICE_INFO_DOWNLOAD "Save device info"
#endif
#ifndef INTL_GUEST_DEVICE_INFO_COPY
#define INTL_GUEST_DEVICE_INFO_COPY "Copy as text"
#endif
#ifndef INTL_GUEST_DEVICE_INFO_SAVED
#define INTL_GUEST_DEVICE_INFO_SAVED "Saved"
#endif
#ifndef INTL_GUEST_DEVICE_INFO_SHARED
#define INTL_GUEST_DEVICE_INFO_SHARED "Choose Save to Files in the share sheet"
#endif
#ifndef INTL_GUEST_DEVICE_INFO_COPIED
#define INTL_GUEST_DEVICE_INFO_COPIED "Copied — paste into Notes"
#endif
#ifndef INTL_GUEST_DEVICE_INFO_SAVE_FAIL
#define INTL_GUEST_DEVICE_INFO_SAVE_FAIL "Could not save — copy the IP above"
#endif
#ifndef INTL_GUEST_SENSOR_ADDRESS
#define INTL_GUEST_SENSOR_ADDRESS "Robonomics address:"
#endif
#ifndef INTL_GUEST_RESTART_PAUSE_HINT
#define INTL_GUEST_RESTART_PAUSE_HINT "The device will restart soon — copy or save your IP before it does."
#endif
#ifndef INTL_GUEST_FINISH_SETUP
#define INTL_GUEST_FINISH_SETUP "Finish setup"
#endif
#ifndef INTL_GUEST_FINISHING_SETUP
#define INTL_GUEST_FINISHING_SETUP "Finishing setup — restarting…"
#endif
#ifndef INTL_GUEST_KEEP_OPEN_HINT
#define INTL_GUEST_KEEP_OPEN_HINT "Do not close this page until you press Continue."
#endif
#ifndef INTL_SETUP_INSIGHT_MODE_HINT
#define INTL_SETUP_INSIGHT_MODE_HINT "To pair with an Altruist Urban, check the box below, then press Continue. If you leave it unchecked, setup continues in standalone mode."
#endif
#ifndef INTL_SETUP_PAIR_WITH_URBAN
#define INTL_SETUP_PAIR_WITH_URBAN "Set up link to an Altruist Urban now"
#endif
#ifndef INTL_SETUP_CONTINUE
#define INTL_SETUP_CONTINUE "Continue"
#endif
#ifndef INTL_DISP_MAP_PROMO_TITLE
#define INTL_DISP_MAP_PROMO_TITLE "Better analytics on your smartphone"
#endif
#ifndef INTL_DISP_MAP_PROMO_LINE1
#define INTL_DISP_MAP_PROMO_LINE1 "Just go to our web map: AQI, data history,"
#endif
#ifndef INTL_DISP_MAP_PROMO_LINE2
#define INTL_DISP_MAP_PROMO_LINE2 "color-coded charts, easy sharing, and"
#endif
#ifndef INTL_DISP_MAP_PROMO_LINE3
#define INTL_DISP_MAP_PROMO_LINE3 "more features coming soon"
#endif
#ifndef INTL_DISP_MAP_DOMAIN
#define INTL_DISP_MAP_DOMAIN "SENSORS.SOCIAL"
#endif

// --- Graphs screen specific strings ---
#ifndef INTL_DISP_GRAPHS_HEADER_PREFIX
#define INTL_DISP_GRAPHS_HEADER_PREFIX "Current"
#endif
#ifndef INTL_DISP_GRAPHS_HINT_LINE1
#define INTL_DISP_GRAPHS_HINT_LINE1 "long press ->"
#endif
#ifndef INTL_DISP_GRAPHS_HINT_LINE2
#define INTL_DISP_GRAPHS_HINT_LINE2 "next/prev"
#endif
#ifndef INTL_DISP_GRAPHS_HINT_LINE3
#define INTL_DISP_GRAPHS_HINT_LINE3 ""
#endif

#ifndef INTL_GROUP_MENU
#define INTL_GROUP_MENU "Device group (RWS)"
#endif
#ifndef INTL_GROUP_INTRO
#define INTL_GROUP_INTRO "Choose how this device participates in Robonomics Web Services."
#endif
#ifndef INTL_GROUP_MODE_TITLE
#define INTL_GROUP_MODE_TITLE "Operating mode"
#endif
#ifndef INTL_GROUP_MODE_STANDALONE
#define INTL_GROUP_MODE_STANDALONE "Standalone"
#endif
#ifndef INTL_GROUP_MODE_MASTER
#define INTL_GROUP_MODE_MASTER "Create group"
#endif
#ifndef INTL_GROUP_MODE_FOLLOWER
#define INTL_GROUP_MODE_FOLLOWER "Join group"
#endif
#ifndef INTL_GROUP_MODE_MANUAL
#define INTL_GROUP_MODE_MANUAL "Manual owner"
#endif
#ifndef INTL_GROUP_SELF_ADDRESS
#define INTL_GROUP_SELF_ADDRESS "This device Robonomics address"
#endif
#ifndef INTL_GROUP_MASTER_PANEL
#define INTL_GROUP_MASTER_PANEL "Group master"
#endif
#ifndef INTL_GROUP_FOLLOWER_PANEL
#define INTL_GROUP_FOLLOWER_PANEL "Join group"
#endif
#ifndef INTL_GROUP_MANUAL_PANEL
#define INTL_GROUP_MANUAL_PANEL "Manual owner"
#endif
#ifndef INTL_GROUP_ID_LABEL
#define INTL_GROUP_ID_LABEL "Group ID"
#endif
#ifndef INTL_GROUP_MASTER_ADDRESS
#define INTL_GROUP_MASTER_ADDRESS "Master Robonomics address"
#endif
#ifndef INTL_GROUP_KNOWN_DEVICES
#define INTL_GROUP_KNOWN_DEVICES "Group devices (SS58)"
#endif
#ifndef INTL_GROUP_KNOWN_DEVICES_HINT
#define INTL_GROUP_KNOWN_DEVICES_HINT "Add follower addresses, then Save."
#endif
#ifndef INTL_GROUP_FOLLOWER_HINT
#define INTL_GROUP_FOLLOWER_HINT "Copy your address to the master list, enter master address here."
#endif
#ifndef INTL_GROUP_MANUAL_HINT
#define INTL_GROUP_MANUAL_HINT "Datalog uses this owner."
#endif
#ifndef INTL_GROUP_STATUS_GROUP_CREATING
#define INTL_GROUP_STATUS_GROUP_CREATING "Group created — syncing on-chain"
#endif
#ifndef INTL_GROUP_STATUS_LIST_UPDATED
#define INTL_GROUP_STATUS_LIST_UPDATED "Device list updated — syncing on-chain"
#endif
#ifndef INTL_GROUP_STATUS_LIST_SYNCED
#define INTL_GROUP_STATUS_LIST_SYNCED "Device list synced on-chain"
#endif
#ifndef INTL_GROUP_STATUS_CREATED
#define INTL_GROUP_STATUS_CREATED "Group created, devices synced"
#endif
#ifndef INTL_GROUP_CURRENT_DEVICES
#define INTL_GROUP_CURRENT_DEVICES "Current devices list"
#endif
#ifndef INTL_GROUP_STATUS_LABEL
#define INTL_GROUP_STATUS_LABEL "Status"
#endif
#ifndef INTL_GROUP_STATUS_PENDING
#define INTL_GROUP_STATUS_PENDING "Pending sync"
#endif
#ifndef INTL_GROUP_STATUS_DEVICES_SYNCED
#define INTL_GROUP_STATUS_DEVICES_SYNCED "Devices synced on-chain"
#endif
#ifndef INTL_GROUP_STATUS_JOINED
#define INTL_GROUP_STATUS_JOINED "Joined group"
#endif
#ifndef INTL_GROUP_STATUS_MANUAL
#define INTL_GROUP_STATUS_MANUAL "Manual owner configured"
#endif
#ifndef INTL_GROUP_SAVE_OK
#define INTL_GROUP_SAVE_OK "Group settings saved."
#endif
#ifndef INTL_GROUP_BACKUP_TITLE
#define INTL_GROUP_BACKUP_TITLE "Device backup"
#endif
#ifndef INTL_GROUP_BACKUP_HINT
#define INTL_GROUP_BACKUP_HINT "This device is its own owner. After Save we download a backup once — import it on sensors.map Login to decrypt encrypted metrics, or restore it on another device. Keep the file private (local HTTP is not encrypted)."
#endif
#ifndef INTL_GROUP_BACKUP_SAVED_NOTICE
#define INTL_GROUP_BACKUP_SAVED_NOTICE "Device backup download started. Store it safely, then import it on sensors.map or restore it from Maintenance."
#endif
#ifndef INTL_GROUP_BACKUP_DOWNLOAD_AGAIN
#define INTL_GROUP_BACKUP_DOWNLOAD_AGAIN "Download backup again…"
#endif
#ifndef INTL_GROUP_BACKUP_REDOWNLOAD_CONFIRM
#define INTL_GROUP_BACKUP_REDOWNLOAD_CONFIRM "Download the device backup again? Anyone with this file can decrypt your encrypted metrics and restore your settings."
#endif
#ifndef INTL_DEVICE_BACKUP_TITLE
#define INTL_DEVICE_BACKUP_TITLE "Backup & restore"
#endif
#ifndef INTL_DEVICE_BACKUP_HINT
#define INTL_DEVICE_BACKUP_HINT "Download a full backup (settings + owner key). You can restore it here or import the same file on sensors.map Login."
#endif
#ifndef INTL_DEVICE_BACKUP_DOWNLOAD
#define INTL_DEVICE_BACKUP_DOWNLOAD "Download backup"
#endif
#ifndef INTL_DEVICE_BACKUP_RESTORE
#define INTL_DEVICE_BACKUP_RESTORE "Restore from backup"
#endif
#ifndef INTL_DEVICE_BACKUP_FILE_LABEL
#define INTL_DEVICE_BACKUP_FILE_LABEL "Backup file"
#endif
#ifndef INTL_DEVICE_BACKUP_FILE_CHOOSE
#define INTL_DEVICE_BACKUP_FILE_CHOOSE "Choose file…"
#endif
#ifndef INTL_DEVICE_BACKUP_FILE_EMPTY
#define INTL_DEVICE_BACKUP_FILE_EMPTY "No file selected"
#endif
#ifndef INTL_DEVICE_BACKUP_FILE_REQUIRED
#define INTL_DEVICE_BACKUP_FILE_REQUIRED "Please choose a backup JSON file first."
#endif
#ifndef INTL_DEVICE_BACKUP_RESTORE_HINT
#define INTL_DEVICE_BACKUP_RESTORE_HINT "Restoring replaces all settings on this device and restarts it."
#endif
#ifndef INTL_DEVICE_BACKUP_RESTORE_CONFIRM
#define INTL_DEVICE_BACKUP_RESTORE_CONFIRM "Restore settings from this backup? Current configuration will be replaced."
#endif
#ifndef INTL_DEVICE_BACKUP_RESTORE_OK
#define INTL_DEVICE_BACKUP_RESTORE_OK "Backup restored. Restarting…"
#endif
#ifndef INTL_DEVICE_BACKUP_RESTORE_FAILED
#define INTL_DEVICE_BACKUP_RESTORE_FAILED "Could not restore backup. Check the file format."
#endif
#ifndef INTL_GUEST_RESTORE_HINT
#define INTL_GUEST_RESTORE_HINT "Have a backup from before the reset? Restore it here to bring back Wi‑Fi, owner key, and settings — then the device restarts."
#endif
#ifndef INTL_GROUP_OWNER_ACCESS_TITLE
#define INTL_GROUP_OWNER_ACCESS_TITLE INTL_GROUP_BACKUP_TITLE
#endif
#ifndef INTL_GROUP_OWNER_ACCESS_HINT
#define INTL_GROUP_OWNER_ACCESS_HINT INTL_GROUP_BACKUP_HINT
#endif
#ifndef INTL_GROUP_OWNER_ACCESS_SAVED_NOTICE
#define INTL_GROUP_OWNER_ACCESS_SAVED_NOTICE INTL_GROUP_BACKUP_SAVED_NOTICE
#endif
#ifndef INTL_GROUP_OWNER_ACCESS_DOWNLOAD_AGAIN
#define INTL_GROUP_OWNER_ACCESS_DOWNLOAD_AGAIN INTL_GROUP_BACKUP_DOWNLOAD_AGAIN
#endif
#ifndef INTL_GROUP_OWNER_ACCESS_REDOWNLOAD_CONFIRM
#define INTL_GROUP_OWNER_ACCESS_REDOWNLOAD_CONFIRM INTL_GROUP_BACKUP_REDOWNLOAD_CONFIRM
#endif
#ifndef INTL_GROUP_OWNER_ACCESS_DOWNLOAD
#define INTL_GROUP_OWNER_ACCESS_DOWNLOAD INTL_DEVICE_BACKUP_DOWNLOAD
#endif
#ifndef INTL_GROUP_SAVE_FAILED
#define INTL_GROUP_SAVE_FAILED "Could not save group settings."
#endif
#ifndef INTL_GROUP_SAVE_CONFIG_FAILED
#define INTL_GROUP_SAVE_CONFIG_FAILED "Could not write configuration to storage."
#endif
#ifndef INTL_GROUP_ERROR_INVALID_MASTER
#define INTL_GROUP_ERROR_INVALID_MASTER "Enter a valid master Robonomics address."
#endif
#ifndef INTL_GROUP_ERROR_INVALID_MANUAL_OWNER
#define INTL_GROUP_ERROR_INVALID_MANUAL_OWNER "Enter a valid owner Robonomics address."
#endif
#ifndef INTL_SCREEN_MENU
#define INTL_SCREEN_MENU "Screen mode"
#endif
#ifndef INTL_SCREEN_INTRO
#define INTL_SCREEN_INTRO "Choose how the e-paper display is refreshed."
#endif
#ifndef INTL_SCREEN_MODE_SAFE
#define INTL_SCREEN_MODE_SAFE "Safe"
#endif
#ifndef INTL_SCREEN_MODE_SAFE_HINT
#define INTL_SCREEN_MODE_SAFE_HINT "Full-screen clean refreshes only. Recommended."
#endif
#ifndef INTL_SCREEN_MODE_EXPERIMENTAL
#define INTL_SCREEN_MODE_EXPERIMENTAL "Experimental partial refresh"
#endif
#ifndef INTL_SCREEN_MODE_EXPERIMENTAL_HINT
#define INTL_SCREEN_MODE_EXPERIMENTAL_HINT "Faster partial updates; may cause ghosting on some panels."
#endif
#ifndef INTL_SCREEN_SAVE_OK
#define INTL_SCREEN_SAVE_OK "Screen mode saved."
#endif
#ifndef INTL_SCREEN_SAVE_FAILED
#define INTL_SCREEN_SAVE_FAILED "Could not save screen mode."
#endif
#ifndef INTL_SCREEN_SAVE_INVALID_MODE
#define INTL_SCREEN_SAVE_INVALID_MODE "Invalid screen mode."
#endif
#ifndef INTL_SCREEN_SAVE_CONFIG_FAILED
#define INTL_SCREEN_SAVE_CONFIG_FAILED "Could not write configuration."
#endif

#ifndef INTL_NAV_MONITOR
#define INTL_NAV_MONITOR "Data & status"
#endif
#ifndef INTL_NAV_SETTINGS
#define INTL_NAV_SETTINGS "Settings"
#endif
#ifndef INTL_NAV_MAINTENANCE
#define INTL_NAV_MAINTENANCE "Maintenance"
#endif
#ifndef INTL_CONFIG_TAB_INTEGRATIONS
#define INTL_CONFIG_TAB_INTEGRATIONS "Integrations"
#endif
#ifndef INTL_CONFIG_PANEL1_INTRO
#define INTL_CONFIG_PANEL1_INTRO "Wi-Fi, Robonomics, location, and what data is published to the public map."
#endif
#ifndef INTL_CONFIG_PANEL2_INTRO
#define INTL_CONFIG_PANEL2_INTRO "Security, firmware updates, and options for advanced users."
#endif
#ifndef INTL_CONFIG_PANEL3_INTRO
#define INTL_CONFIG_PANEL3_INTRO "Optional exports to your own services. Expand a section only if you need it."
#endif
#ifndef INTL_BADGE_BETA
#define INTL_BADGE_BETA "Beta"
#endif
#ifndef INTL_BADGE_EXPERIMENTAL
#define INTL_BADGE_EXPERIMENTAL "Experimental"
#endif

#ifndef INTL_PANEL_TITLE_LEDS
#define INTL_PANEL_TITLE_LEDS "LEDs"
#endif

#ifndef INTL_ANALYTICS_MORNING_AUTOSWITCH
#define INTL_ANALYTICS_MORNING_AUTOSWITCH "Open sleep analytics on the display each morning (06:00–end)"
#endif
#ifndef INTL_ANALYTICS_NIGHT_START_TIME
#define INTL_ANALYTICS_NIGHT_START_TIME "Sleep analytics night start (local, HH:MM)"
#endif
#ifndef INTL_ANALYTICS_NIGHT_END_TIME
#define INTL_ANALYTICS_NIGHT_END_TIME "Sleep analytics night end (local, HH:MM)"
#endif
#ifndef INTL_ANALYTICS_NIGHT_END_HINT
#define INTL_ANALYTICS_NIGHT_END_HINT "End is exclusive for hourly buckets (e.g. 07:00 uses hours through 06:xx)."
#endif
#ifndef INTL_ANALYTICS_MORNING_END_TIME
#define INTL_ANALYTICS_MORNING_END_TIME "Morning display until (local, HH:MM)"
#endif
#ifndef INTL_ANALYTICS_MORNING_END_HINT
#define INTL_ANALYTICS_MORNING_END_HINT "After this time the display returns to the main screen. End time is exclusive (10:00 means until 09:59)."
#endif
#ifndef INTL_ANALYTICS_SLEEP_ADD_URBAN
#define INTL_ANALYTICS_SLEEP_ADD_URBAN "Add Urban data to sleep analytics (PM2.5 & noise)"
#endif
#ifndef INTL_ANALYTICS_SLEEP_ADD_URBAN_STANDALONE_HINT
#define INTL_ANALYTICS_SLEEP_ADD_URBAN_STANDALONE_HINT "Disable Insight standalone in the Firmware section to pair with Urban and use outdoor PM2.5/noise in sleep analytics."
#endif
#ifndef INTL_PANEL_TITLE_SLEEP_ANALYTICS
#define INTL_PANEL_TITLE_SLEEP_ANALYTICS "Sleep analytics"
#endif
#ifndef INTL_ANALYTICS_MORNING_AUTOSWITCH
#define INTL_ANALYTICS_MORNING_AUTOSWITCH "Open sleep analytics on the display each morning (06:00–end)"
#endif
#ifndef INTL_ANALYTICS_MORNING_END_TIME
#define INTL_ANALYTICS_MORNING_END_TIME "Morning display until (local, HH:MM)"
#endif
#ifndef INTL_ANALYTICS_MORNING_END_HINT
#define INTL_ANALYTICS_MORNING_END_HINT "After this time the display returns to the main screen. End time is exclusive (10:00 means until 09:59)."
#endif
#ifndef INTL_DATA_SECTION_SDS
#define INTL_DATA_SECTION_SDS "SDS"
#endif
#ifndef INTL_DATA_SECTION_SCD
#define INTL_DATA_SECTION_SCD "SCD4x"
#endif
#ifndef INTL_DATA_SECTION_BME
#define INTL_DATA_SECTION_BME "BME"
#endif
#ifndef INTL_DATA_SECTION_URBAN
#define INTL_DATA_SECTION_URBAN "Urban data"
#endif
#ifndef INTL_DATA_SECTION_OVERVIEW
#define INTL_DATA_SECTION_OVERVIEW "Overview"
#endif
#ifndef INTL_DATA_SECTION_DEVICE
#define INTL_DATA_SECTION_DEVICE "Device"
#endif
#ifndef INTL_DATA_SECTION_RUNTIME
#define INTL_DATA_SECTION_RUNTIME "Runtime"
#endif
#ifndef INTL_DATA_SECTION_NETWORK
#define INTL_DATA_SECTION_NETWORK "Network"
#endif
#ifndef INTL_DATA_SECTION_EXPORT
#define INTL_DATA_SECTION_EXPORT "Data export"
#endif
#ifndef INTL_DATA_SECTION_TECHNICAL
#define INTL_DATA_SECTION_TECHNICAL "Technical details"
#endif
#ifndef INTL_VALUE_YES
#define INTL_VALUE_YES "Yes"
#endif
#ifndef INTL_VALUE_NO
#define INTL_VALUE_NO "No"
#endif
#ifndef INTL_READINGS_SECTION_NETWORK_INTRO
#define INTL_READINGS_SECTION_NETWORK_INTRO "Wi-Fi signal strength at the sensor."
#endif
#ifndef INTL_STATUS_SECTION_OVERVIEW_INTRO
#define INTL_STATUS_SECTION_OVERVIEW_INTRO "Is the device running normally right now?"
#endif
#ifndef INTL_STATUS_SECTION_DEVICE_INTRO
#define INTL_STATUS_SECTION_DEVICE_INTRO "Firmware version, memory, and storage."
#endif
#ifndef INTL_STATUS_SECTION_TECH_INTRO
#define INTL_STATUS_SECTION_TECH_INTRO "Build details — useful when contacting support."
#endif
#ifndef INTL_STATUS_SECTION_EXPORT_INTRO
#define INTL_STATUS_SECTION_EXPORT_INTRO "Whether your data is reaching each service."
#endif
#ifndef INTL_API_SENDS_SHORT
#define INTL_API_SENDS_SHORT "Sends"
#endif
#ifndef INTL_API_LAST_SHORT
#define INTL_API_LAST_SHORT "Last"
#endif
#ifndef INTL_DATA_BUSY
#define INTL_DATA_BUSY "Sensor data is updating — refresh in a moment."
#endif
#ifndef INTL_NAV_HOME
#define INTL_NAV_HOME "Home"
#endif
#ifndef INTL_NAV_READINGS
#define INTL_NAV_READINGS "Readings"
#endif
#ifndef INTL_NAV_STATUS
#define INTL_NAV_STATUS "Status"
#endif
#ifndef INTL_NAV_MAIN
#define INTL_NAV_MAIN "Main navigation"
#endif
#ifndef INTL_BREADCRUMB_ARIA
#define INTL_BREADCRUMB_ARIA "Breadcrumb"
#endif
#ifndef INTL_DASH_TITLE
#define INTL_DASH_TITLE "Dashboard"
#endif
#ifndef INTL_DASH_ALL_READINGS
#define INTL_DASH_ALL_READINGS "All readings"
#endif
#ifndef INTL_DASH_DEVICE_HEALTH
#define INTL_DASH_DEVICE_HEALTH "Device health"
#endif
#ifndef INTL_DASH_WIFI_OK
#define INTL_DASH_WIFI_OK "WiFi connected"
#endif
#ifndef INTL_DASH_WIFI_OFF
#define INTL_DASH_WIFI_OFF "WiFi offline"
#endif
#ifndef INTL_DASH_DATALOG_OK
#define INTL_DASH_DATALOG_OK "Datalog OK"
#endif
#ifndef INTL_DASH_DATALOG_ERR
#define INTL_DASH_DATALOG_ERR "Datalog issue"
#endif
#ifndef INTL_DASH_MAP_OK
#define INTL_DASH_MAP_OK "Map OK"
#endif
#ifndef INTL_DASH_MAP_ERR
#define INTL_DASH_MAP_ERR "Map issue"
#endif
#ifndef INTL_DASH_NAV
#define INTL_DASH_NAV "Quick links"
#endif
#ifndef INTL_DASH_SECTION_MONITOR_INTRO
#define INTL_DASH_SECTION_MONITOR_INTRO "See what your sensor measures and whether it is working."
#endif
#ifndef INTL_DASH_SECTION_SETTINGS_INTRO
#define INTL_DASH_SECTION_SETTINGS_INTRO "Wi-Fi, updates, and how your data is shared."
#endif
#ifndef INTL_DASH_SECTION_MAP_INTRO
#define INTL_DASH_SECTION_MAP_INTRO "Find this device on the public air-quality map."
#endif
#ifndef INTL_DASH_READINGS_DESC
#define INTL_DASH_READINGS_DESC "Live temperature, humidity, and air quality"
#endif
#ifndef INTL_DASH_STATUS_DESC
#define INTL_DASH_STATUS_DESC "Connection, firmware, and device details"
#endif
#ifndef INTL_DASH_CONFIG_DESC
#define INTL_DASH_CONFIG_DESC "Wi-Fi, location, and publishing options"
#endif
#ifndef INTL_DASH_GROUP_DESC
#define INTL_DASH_GROUP_DESC "Combine readings with nearby sensors"
#endif
#ifndef INTL_DASH_OTA_DESC
#define INTL_DASH_OTA_DESC "Check and install firmware updates"
#endif
#ifndef INTL_DASH_SCREEN_DESC
#define INTL_DASH_SCREEN_DESC "Choose what the display shows"
#endif
#ifndef INTL_DASH_MAP_DESC
#define INTL_DASH_MAP_DESC "Open sensors.social in a new tab"
#endif
#ifndef INTL_DASH_SECTION_MAINTENANCE_INTRO
#define INTL_DASH_SECTION_MAINTENANCE_INTRO "Advanced actions — only if you know what you are doing."
#endif
#ifndef INTL_DASH_DEBUG_DESC
#define INTL_DASH_DEBUG_DESC "More detailed logs for troubleshooting"
#endif
#ifndef INTL_DASH_RESTART_DESC
#define INTL_DASH_RESTART_DESC "Reboot the sensor (about a minute)"
#endif
#ifndef INTL_DASH_DELETE_CONFIG_DESC
#define INTL_DASH_DELETE_CONFIG_DESC "Erase saved settings from the device"
#endif
#ifndef INTL_DASH_HEALTH_TITLE
#define INTL_DASH_HEALTH_TITLE "At a glance"
#endif
#ifndef INTL_HUB_LOCAL_TITLE
#define INTL_HUB_LOCAL_TITLE "Local"
#endif
#ifndef INTL_HUB_LOCAL_DESC
#define INTL_HUB_LOCAL_DESC "Readings, settings, and device maintenance"
#endif
#ifndef INTL_HUB_SOCIAL_DESC
#define INTL_HUB_SOCIAL_DESC "Public map and Robonomics network"
#endif
#ifndef INTL_HUB_CUSTOM_DESC
#define INTL_HUB_CUSTOM_DESC "Home Assistant, API, InfluxDB, CSV"
#endif
#ifndef INTL_DASH_GROUP_LOCAL_INTRO
#define INTL_DASH_GROUP_LOCAL_INTRO "Readings, settings, and maintenance on your home network."
#endif
#ifndef INTL_DASH_GROUP_SOCIAL_TITLE
#define INTL_DASH_GROUP_SOCIAL_TITLE "sensors.social"
#endif
#ifndef INTL_DASH_GROUP_SOCIAL_INTRO
#define INTL_DASH_GROUP_SOCIAL_INTRO "Public map, Robonomics network, and what you publish."
#endif
#ifndef INTL_DASH_GROUP_CUSTOM_TITLE
#define INTL_DASH_GROUP_CUSTOM_TITLE "Custom"
#endif
#ifndef INTL_DASH_GROUP_CUSTOM_INTRO
#define INTL_DASH_GROUP_CUSTOM_INTRO "Home Assistant, your own API, InfluxDB, and CSV export."
#endif
#ifndef INTL_DASH_CAT_DATA
#define INTL_DASH_CAT_DATA "Data"
#endif
#ifndef INTL_DASH_CAT_SETTINGS
#define INTL_DASH_CAT_SETTINGS "Settings"
#endif
#ifndef INTL_DASH_CAT_MAINTENANCE
#define INTL_DASH_CAT_MAINTENANCE "Maintenance"
#endif
#ifndef INTL_DASH_CAT_MAP
#define INTL_DASH_CAT_MAP "Public map"
#endif
#ifndef INTL_DASH_CAT_NETWORK
#define INTL_DASH_CAT_NETWORK "Robonomics"
#endif
#ifndef INTL_DASH_CAT_INTEGRATIONS
#define INTL_DASH_CAT_INTEGRATIONS "Integrations"
#endif
#ifndef INTL_DASH_CONFIG_MAP_DESC
#define INTL_DASH_CONFIG_MAP_DESC "Choose which measurements appear on the public map."
#endif
#ifndef INTL_DASH_CONFIG_ROBONOMICS_DESC
#define INTL_DASH_CONFIG_ROBONOMICS_DESC "Owner, node, and on-chain data settings."
#endif
#ifndef INTL_DASH_CONFIG_INTEGRATIONS_DESC
#define INTL_DASH_CONFIG_INTEGRATIONS_DESC "Open the data export tab in configuration."
#endif
#ifndef INTL_DASH_CUSTOM_API_DESC
#define INTL_DASH_CUSTOM_API_DESC "HTTP push to your server (e.g. Home Assistant REST)."
#endif
#ifndef INTL_DASH_INFLUX_DESC
#define INTL_DASH_INFLUX_DESC "Send measurements to InfluxDB."
#endif
#ifndef INTL_DASH_CSV_DESC
#define INTL_DASH_CSV_DESC "Write a CSV file on the device or your endpoint."
#endif
#ifndef INTL_PAGE_READINGS_INTRO
#define INTL_PAGE_READINGS_INTRO "Latest measurements from your sensors."
#endif
#ifndef INTL_PAGE_STATUS_INTRO
#define INTL_PAGE_STATUS_INTRO "Quick health check first — open a section below for details."
#endif
#ifndef INTL_PAGE_OTA_INTRO
#define INTL_PAGE_OTA_INTRO "Check for updates or switch the firmware language."
#endif
#ifndef INTL_PAGE_DEBUG_INTRO
#define INTL_PAGE_DEBUG_INTRO "Live log output and how much detail is recorded."
#endif
#ifndef INTL_PAGE_RESTART_INTRO
#define INTL_PAGE_RESTART_INTRO "The sensor will reboot and reconnect to Wi-Fi."
#endif
#ifndef INTL_PAGE_DELETE_CONFIG_INTRO
#define INTL_PAGE_DELETE_CONFIG_INTRO "Choose what to erase. This cannot be undone."
#endif
#ifndef INTL_DELETE_CONFIG_ALL
#define INTL_DELETE_CONFIG_ALL "All settings"
#endif
#ifndef INTL_CONFIGURATION_DELETE_WARNING
#define INTL_CONFIGURATION_DELETE_WARNING "This cannot be undone. The device will restart after deleting the selected settings."
#endif
#ifndef INTL_CONFIGURATION_DELETE_CONFIRM
#define INTL_CONFIGURATION_DELETE_CONFIRM "Yes, delete permanently"
#endif
#ifndef INTL_DELETE_CONFIG_WIFI
#define INTL_DELETE_CONFIG_WIFI "Wi-Fi only"
#endif
#ifndef INTL_DELETE_CONFIG_ALL_DESC
#define INTL_DELETE_CONFIG_ALL_DESC "Everything stored on the device"
#endif
#ifndef INTL_DELETE_CONFIG_WIFI_DESC
#define INTL_DELETE_CONFIG_WIFI_DESC "Only forget the saved Wi-Fi network"
#endif
#ifndef INTL_WIFI_CREDENTIALS_DELETED
#define INTL_WIFI_CREDENTIALS_DELETED "Wi-Fi credentials were deleted. You can close this page."
#endif
#ifndef INTL_PANEL_TITLE_DATA_ENCRYPT
#define INTL_PANEL_TITLE_DATA_ENCRYPT "Encrypt map values"
#endif
#ifndef INTL_DATA_ENCRYPT_DISCLAIMER
#define INTL_DATA_ENCRYPT_DISCLAIMER "Optional. Selected metrics are sent encrypted (AES-256). Import this device key in sensors.map to view them. Erasing flash creates a new key."
#endif
#ifndef INTL_DATA_ENCRYPT_BACKUP_HINT
#define INTL_DATA_ENCRYPT_BACKUP_HINT "No Robonomics seed phrase for sensors.map login? Import a device backup instead (only works with self-owner) :"
#endif
#ifndef INTL_DATA_ENCRYPT_BACKUP_LINK
#define INTL_DATA_ENCRYPT_BACKUP_LINK "Open Backup & restore"
#endif
#ifndef INTL_DATA_ENCRYPT_KEY_LABEL
#define INTL_DATA_ENCRYPT_KEY_LABEL "Device encryption key"
#endif
#ifndef INTL_DATA_ENCRYPT_KEY_SHOW
#define INTL_DATA_ENCRYPT_KEY_SHOW "Show key"
#endif
#ifndef INTL_DATA_ENCRYPT_KEY_HIDE
#define INTL_DATA_ENCRYPT_KEY_HIDE "Hide key"
#endif
#ifndef INTL_DATA_ENCRYPT_KEY_COPY
#define INTL_DATA_ENCRYPT_KEY_COPY "Copy key"
#endif
#ifndef INTL_DATA_ENCRYPT_KEY_COPIED
#define INTL_DATA_ENCRYPT_KEY_COPIED "Key copied"
#endif
#ifndef INTL_DATA_ENCRYPT_KEY_HINT
#define INTL_DATA_ENCRYPT_KEY_HINT "Scan with your phone camera (same Wi‑Fi) to download the key JSON file. Or tap Show key and copy the text."
#endif
#ifndef INTL_DATA_ENCRYPT_QR_FAIL
#define INTL_DATA_ENCRYPT_QR_FAIL "Could not render key QR."
#endif
#ifndef INTL_REGION_RU
#define INTL_REGION_RU "Russia"
#endif
#ifndef INTL_REGION_HINT
#define INTL_REGION_HINT "Used for sensors.social connectivity. Auto from coordinates / OTA unless you change it here."
#endif

#endif

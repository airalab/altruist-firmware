#ifndef intl_h
#define intl_h

#define REGION_GLOBAL "Global"
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
#define INTL_GUEST_INSIGHT_AUTO_FINISH_HINT "If you close this page, setup will finish automatically in"
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
#ifndef INTL_GUEST_RESTART_PAUSE_HINT
#define INTL_GUEST_RESTART_PAUSE_HINT "The device will restart in a few seconds — copy the IP now."
#endif
#ifndef INTL_GUEST_KEEP_OPEN_HINT
#define INTL_GUEST_KEEP_OPEN_HINT "Do not close this page until you press Continue."
#endif
#ifndef INTL_SETUP_INSIGHT_MODE_HINT
#define INTL_SETUP_INSIGHT_MODE_HINT "If you have an Altruist Urban (outdoor unit), check the box and press Continue to search the network and pick it. Leave the box unchecked to use this Insight on its own — the device will restart right away."
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

#endif

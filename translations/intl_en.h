/*
 *	airRohr firmware
 *	Copyright (C) 2016-2018  Code for Stuttgart a.o.
 *
 *  English translations
 *
 *	Texts should be as short as possible
 */

#define INTL_LANG "EN"
#define INTL_PM_SENSOR "Particulate matter sensor"
const char INTL_CONFIGURATION[] PROGMEM = "Configuration";
#define INTL_COMMON_SETTINGS "Connection"
#define INTL_APIS_SETTINGS "Data export"
#define INTL_NAV_MONITOR "Data & status"
#define INTL_NAV_SETTINGS "Settings"
#define INTL_NAV_MAINTENANCE "Maintenance"
#define INTL_NAV_HOME "Home"
#define INTL_NAV_READINGS "Readings"
#define INTL_NAV_STATUS "Status"
#define INTL_NAV_MAIN "Main navigation"
#define INTL_BREADCRUMB_ARIA "Breadcrumb"
#define INTL_DASH_TITLE "Dashboard"
#define INTL_DASH_ALL_READINGS "All readings"
#define INTL_DASH_DEVICE_HEALTH "Device health"
#define INTL_DASH_WIFI_OK "WiFi connected"
#define INTL_DASH_WIFI_OFF "WiFi offline"
#define INTL_DASH_DATALOG_OK "Datalog OK"
#define INTL_DASH_DATALOG_ERR "Datalog issue"
#define INTL_DASH_MAP_OK "Map OK"
#define INTL_DASH_MAP_ERR "Map issue"
#define INTL_DASH_NAV "Quick links"
#define INTL_DASH_SECTION_MONITOR_INTRO "See what your sensor measures and whether it is working."
#define INTL_DASH_SECTION_SETTINGS_INTRO "Wi-Fi, updates, and how your data is shared."
#define INTL_DASH_SECTION_MAP_INTRO "Find this device on the public air-quality map."
#define INTL_DASH_READINGS_DESC "Live temperature, humidity, and air quality"
#define INTL_DASH_STATUS_DESC "Connection, firmware, and device details"
#define INTL_DASH_CONFIG_DESC "Wi-Fi, location, and publishing options"
#define INTL_DASH_GROUP_DESC "Combine readings with nearby sensors"
#define INTL_DASH_OTA_DESC "Check and install firmware updates"
#define INTL_DASH_SCREEN_DESC "Choose what the display shows"
#define INTL_DASH_MAP_DESC "Open sensors.social in a new tab"
#define INTL_DASH_SECTION_MAINTENANCE_INTRO "Advanced actions — only if you know what you are doing."
#define INTL_DASH_DEBUG_DESC "More detailed logs for troubleshooting"
#define INTL_DASH_RESTART_DESC "Reboot the sensor (about a minute)"
#define INTL_DASH_DELETE_CONFIG_DESC "Erase saved settings from the device"
#define INTL_DASH_HEALTH_TITLE "At a glance"
#define INTL_HUB_LOCAL_TITLE "Local"
#define INTL_HUB_LOCAL_DESC "Readings and device settings"
#define INTL_HUB_SOCIAL_DESC "Public map and Robonomics network"
#define INTL_HUB_CUSTOM_DESC "Home Assistant, API, InfluxDB, CSV"
#define INTL_NAV_ADVANCED "System"
#define INTL_HUB_ADVANCED_DESC "Debug log, restart, and factory reset"
#define INTL_HUB_DIV_NETWORK "Network"
#define INTL_HUB_DIV_ACCESS "Access"
#define INTL_HUB_DIV_DEVICE "Device"
#define INTL_HUB_DIV_PUBLISH "Publishing"
#define INTL_HUB_DIV_LOCATION "Location"
#define INTL_HUB_DIV_DIAGNOSTICS "Diagnostics"
#define INTL_HUB_DIV_DANGER "Danger zone"
#define INTL_DASH_GROUP_LOCAL_INTRO "Readings, settings, and maintenance on your home network."
#define INTL_DASH_GROUP_SOCIAL_TITLE "Map"
#define INTL_DASH_GROUP_SOCIAL_INTRO "Public map, Robonomics network, and what you publish."
#define INTL_DASH_GROUP_CUSTOM_TITLE "Custom"
#define INTL_DASH_GROUP_CUSTOM_INTRO "Home Assistant, your own API, InfluxDB, and CSV export."
#define INTL_DASH_CAT_DATA "Data"
#define INTL_DASH_CAT_SETTINGS "Settings"
#define INTL_DASH_CAT_MAINTENANCE "Maintenance"
#define INTL_DASH_CAT_MAP "Map"
#define INTL_DASH_CAT_NETWORK "Robonomics"
#define INTL_DASH_CAT_INTEGRATIONS "Integrations"
#define INTL_DASH_CONFIG_MAP_DESC "Choose which measurements appear on the public map."
#define INTL_DASH_CONFIG_ROBONOMICS_DESC "Owner, node, and on-chain data settings."
#define INTL_DASH_CONFIG_INTEGRATIONS_DESC "Open the data export tab in configuration."
#define INTL_DASH_CUSTOM_API_DESC "HTTP push to your server (e.g. Home Assistant REST)."
#define INTL_DASH_INFLUX_DESC "Send measurements to InfluxDB."
#define INTL_DASH_CSV_DESC "Write a CSV file on the device or your endpoint."
#define INTL_PAGE_READINGS_INTRO "Latest measurements from your sensors."
#define INTL_PAGE_STATUS_INTRO "Quick health check first — open a section below for details."
#define INTL_PAGE_OTA_INTRO "Check for updates or switch the firmware language."
#define INTL_PAGE_DEBUG_INTRO "Live log output and how much detail is recorded."
#define INTL_PAGE_RESTART_INTRO "The sensor will reboot and reconnect to Wi-Fi."
#define INTL_PAGE_DELETE_CONFIG_INTRO "Choose what to erase. This cannot be undone."
#define INTL_DELETE_CONFIG_ALL "All settings"
#define INTL_DELETE_CONFIG_WIFI "Wi-Fi only"
#define INTL_DELETE_CONFIG_ALL_DESC "Everything stored on the device"
#define INTL_DELETE_CONFIG_WIFI_DESC "Only forget the saved Wi-Fi network"
#define INTL_WIFI_CREDENTIALS_DELETED "Wi-Fi credentials were deleted. You can close this page."
#define INTL_CONFIG_TAB_INTEGRATIONS "Data export"
#define INTL_CONFIG_PANEL1_INTRO "Wi-Fi, Robonomics, location, and what data is published to the public map."
#define INTL_CONFIG_PANEL2_INTRO "Security, firmware updates, and options for advanced users."
#define INTL_CONFIG_PANEL3_INTRO "Optional exports to your own services. Expand a section only if you need it."
#define INTL_BADGE_BETA "Beta"
#define INTL_BADGE_EXPERIMENTAL "Experimental"
#define INTL_WIFI_NETWORKS "Loading wifi networks ..."
#define INTL_LANGUAGE "Language"
#define INTL_NO_WLAN_PWD "Check it if WiFi network has no password"
const char INTL_NO_NETWORKS[] PROGMEM =  "No WiFi Network Found";
const char INTL_NETWORKS_FOUND[] PROGMEM = "Found Networks: ";
const char INTL_AB_HIER_NUR_ANDERN[] PROGMEM = "Advanced settings (only if you know what you are doing)";
const char INTL_SAVE[] PROGMEM = "Save";
const char INTL_SENSORS[] PROGMEM = "Sensors";
const char INTL_MORE_SENSORS[] PROGMEM = "More Sensors";
const char INTL_SDS011[] PROGMEM = "SDS011 ({pm})";
const char INTL_GC[] PROGMEM = "Geiger counter";
const char INTL_DBMETER[] PROGMEM = "Noise Level Sensor, (sending interval must be > 30 s)";
const char INTL_I2SNOISE[] PROGMEM = "I2S Noise Level Sensor, (sending interval must be > 30 s)";
const char INTL_PMS[] PROGMEM = "Plantower PMS(1,3,5,6,7)003 ({pm})";
const char INTL_HPM[] PROGMEM = "Honeywell PM ({pm})";
const char INTL_NPM[] PROGMEM = "Tera Sensor Next PM ({pm})";
const char INTL_SPS30[] PROGMEM = "Sensirion SPS30 ({pm})";
const char INTL_PPD42NS[] PROGMEM = "PPD42NS ({pm})";
const char INTL_DHT22[] PROGMEM = "DHT22 ({t}, {h})";
const char INTL_HTU21D[] PROGMEM = "HTU21D ({t}, {h})";
const char INTL_BMP180[] PROGMEM = "BMP180 ({t}, {p})";
const char INTL_BMX280[] PROGMEM = "BME280 ({t}, {h}, {p}), BMP280 ({t}, {p})";
const char INTL_SHT3X[] PROGMEM = "SHT3X ({t}, {h})";
const char INTL_DS18B20[] PROGMEM = "DS18B20 ({t})";
const char INTL_CCS811_27[] PROGMEM = "CCS811 (I2C: 0x5A)";
const char INTL_CCS811_3F[] PROGMEM = "CCS811 (I2C: 0x5B)";
const char INTL_DNMS[] PROGMEM = "DNMS ({l_a})";
const char INTL_DNMS_CORRECTION[] PROGMEM = "correction in dB(A)";
const char INTL_TEMP_CORRECTION[] PROGMEM = "Temperature correction in °C";
const char INTL_CUSTOM_ALTRUIST[] PROGMEM = "Custom Altruist Urban Address";
const char INTL_USE_CUSTOM_URBAN[] PROGMEM = "Use Custom Altruist Urban Address";
const char INTL_INSIGHT_STANDALONE[] PROGMEM = "Insight standalone";

// Urban selection (guest setup & config page)
#define INTL_SCANNING_URBANS "Scanning for Altruist Urban devices..."
#define INTL_SELECT_URBAN_TITLE "Select Altruist Urban Device"
#define INTL_SELECT_URBAN_DESC "Choose which Urban device this Insight should read outdoor sensor data from."
#define INTL_NO_URBANS_FOUND "No Altruist Urban devices found on this network. Make sure your Urban device is powered on and connected to the same WiFi network. You can enter a custom IP address below, then save. You can add or change Urban later in Settings."
#define INTL_USE_CUSTOM_IP "Use custom IP address:"
#define INTL_SETUP_INSIGHT_MODE_HINT "To pair with an Altruist Urban, check the box below, then press Continue. If you leave it unchecked, setup continues in standalone mode."
#define INTL_SETUP_PAIR_WITH_URBAN "Set up link to an Altruist Urban now"
#define INTL_SETUP_CONTINUE "Continue"
#define INTL_SKIP_URBAN_SELECTION "Skip &mdash; configure later from Settings page"
#define INTL_SETUP_COMPLETE "Setup Complete"
#define INTL_SETTINGS_SAVED "Settings saved"
#define INTL_DEVICE_RESTARTING "Restarting device..."
#define INTL_GUEST_CONNECTED "Connected"
#define INTL_GUEST_CONNECTING "Connecting to Wi‑Fi…"
#define INTL_GUEST_CONNECTING_HINT "This usually takes a few seconds. Keep this page open."
#define INTL_GUEST_CONNECT_FAILED "Connection failed"
#define INTL_GUEST_CONNECT_FAILED_HINT "Check the network name and password, then try again."
#define INTL_GUEST_CONNECT_FAILED_INSIGHT "Check the network name and password, then try again from the setup page."
#define INTL_GUEST_WIFI_STEP_TITLE "Wi-Fi connected"
#define INTL_GUEST_SETUP_STEP_1_LABEL "Step 1 of 2"
#define INTL_GUEST_SETUP_STEP_2_LABEL "Step 2 of 2"
#define INTL_GUEST_SETUP_STEP_1_TITLE "Connect to Wi-Fi"
#define INTL_GUEST_INSIGHT_FINISH_HINT "Press Continue to finish setup and restart the device."
#define INTL_GUEST_INSIGHT_AUTO_FINISH_HINT "If you don't press Continue, setup will finish alone (standalone) in"
#define INTL_GUEST_INSIGHT_AUTO_FINISH_SUFFIX "seconds (standalone mode)."
#define INTL_GUEST_IP_ADDRESS "IP Address:"
#define INTL_GUEST_OPEN_IP_HINT "Copy the IP address and open it in your browser."
#define INTL_GUEST_DEVICE_INFO_HINT "On iPhone: tap Save → Share → Save to Files. On Android it can go to Downloads. Or copy as text."
#define INTL_GUEST_DEVICE_INFO_DOWNLOAD "Save device info"
#define INTL_GUEST_DEVICE_INFO_COPY "Copy as text"
#define INTL_GUEST_DEVICE_INFO_SAVED "Saved"
#define INTL_GUEST_DEVICE_INFO_SHARED "Choose Save to Files in the share sheet"
#define INTL_GUEST_DEVICE_INFO_COPIED "Copied — paste into Notes"
#define INTL_GUEST_DEVICE_INFO_SAVE_FAIL "Could not save — copy the IP above"
#define INTL_GUEST_SENSOR_ADDRESS "Robonomics address:"
#define INTL_GUEST_RESTART_PAUSE_HINT "The device will restart soon — copy or save your IP before it does."
#define INTL_GUEST_FINISH_SETUP "Finish setup"
#define INTL_GUEST_FINISHING_SETUP "Finishing setup — restarting…"
#define INTL_GUEST_KEEP_OPEN_HINT "Do not close this page until you press Continue."
#define INTL_DISP_MAP_PROMO_TITLE "Better analytics on your smartphone"
#define INTL_DISP_MAP_PROMO_LINE1 "Just go to our web map: AQI, data history,"
#define INTL_DISP_MAP_PROMO_LINE2 "color-coded charts, easy sharing, and"
#define INTL_DISP_MAP_PROMO_LINE3 "more features coming soon"
#define INTL_DISP_MAP_DOMAIN "SENSORS.SOCIAL"
#define INTL_SCAN_BTN "Scan"
#define INTL_SCAN_SCANNING "Scanning..."
#define INTL_SCAN_NO_URBANS "No Urban found via mDNS. For C3 Urban use Custom Urban IP (browser IP)."
#define INTL_SCAN_FOUND_PREFIX "Found "
#define INTL_SCAN_FOUND_SUFFIX " Urban device(s)."
#define INTL_SCAN_FAILED "Scan failed: "

const char INTL_NEO6M[] PROGMEM = "GPS (NEO 6M)";
const char INTL_RWS_OWNER[] PROGMEM = "RWS Owner Address";
const char INTL_GROUP_MENU[] PROGMEM = "Device group (RWS)";
const char INTL_GROUP_INTRO[] PROGMEM = "Choose how this device participates in Robonomics Web Services (owner and on-chain device list).";
const char INTL_GROUP_MODE_TITLE[] PROGMEM = "Operating mode";
const char INTL_GROUP_MODE_STANDALONE[] PROGMEM = "Standalone — this device is its own master (setDevices with itself only)";
const char INTL_GROUP_MODE_MASTER[] PROGMEM = "Create group — this device is the group master";
const char INTL_GROUP_MODE_FOLLOWER[] PROGMEM = "Join group — follow a master device (enter master address below)";
const char INTL_GROUP_MODE_MANUAL[] PROGMEM = "Manual owner — legacy: set owner only, no automatic setDevices";
const char INTL_GROUP_SELF_ADDRESS[] PROGMEM = "This device Robonomics address (copy to master when joining)";
const char INTL_GROUP_MASTER_PANEL[] PROGMEM = "Group master";
const char INTL_GROUP_FOLLOWER_PANEL[] PROGMEM = "Join group";
const char INTL_GROUP_MANUAL_PANEL[] PROGMEM = "Manual owner";
const char INTL_GROUP_ID_LABEL[] PROGMEM = "Group ID";
const char INTL_GROUP_MASTER_ADDRESS[] PROGMEM = "Master Robonomics address";
const char INTL_GROUP_MASTER_INCLUDED[] PROGMEM = "Master device (added to setDevices automatically)";
const char INTL_GROUP_KNOWN_DEVICES[] PROGMEM = "Additional devices — followers (SS58, one per line)";
const char INTL_GROUP_KNOWN_DEVICES_HINT[] PROGMEM = "Your device address above is always included as master. Add follower SS58 addresses here, then Save.";
const char INTL_GROUP_FOLLOWER_HINT[] PROGMEM = "Copy your address above to the master device list, enter the master address here, then Save.";
const char INTL_GROUP_MANUAL_HINT[] PROGMEM = "Datalog uses this owner. setDevices is not called automatically.";
const char INTL_GROUP_STATUS_GROUP_CREATING[] PROGMEM = "Group created — syncing on-chain";
const char INTL_GROUP_STATUS_LIST_UPDATED[] PROGMEM = "Device list updated — syncing on-chain";
const char INTL_GROUP_STATUS_LIST_SYNCED[] PROGMEM = "Device list synced on-chain";
const char INTL_GROUP_STATUS_LABEL[] PROGMEM = "Status";
const char INTL_GROUP_STATUS_CREATED[] PROGMEM = "Group created, devices synced";
const char INTL_GROUP_CURRENT_DEVICES[] PROGMEM = "Current devices list";
const char INTL_GROUP_STATUS_PENDING[] PROGMEM = "Pending sync";
const char INTL_GROUP_STATUS_DEVICES_SYNCED[] PROGMEM = "Devices synced on-chain";
const char INTL_GROUP_STATUS_JOINED[] PROGMEM = "Joined group";
const char INTL_GROUP_STATUS_MANUAL[] PROGMEM = "Manual owner configured";
const char INTL_GROUP_SAVE_OK[] PROGMEM = "Group settings saved.";
const char INTL_GROUP_BACKUP_TITLE[] PROGMEM = "Device backup";
const char INTL_GROUP_BACKUP_HINT[] PROGMEM =
	"This device is its own owner. After Save we download a backup once — import it on sensors.map Login to decrypt encrypted metrics, or restore it on another device. Keep the file private (local HTTP is not encrypted).";
const char INTL_GROUP_BACKUP_SAVED_NOTICE[] PROGMEM =
	"Device backup download started. Store it safely, then import it on sensors.map or restore it from Maintenance.";
const char INTL_GROUP_BACKUP_DOWNLOAD_AGAIN[] PROGMEM = "Download backup again…";
const char INTL_GROUP_BACKUP_REDOWNLOAD_CONFIRM[] PROGMEM =
	"Download the device backup again? Anyone with this file can decrypt your encrypted metrics and restore your settings.";
const char INTL_DEVICE_BACKUP_TITLE[] PROGMEM = "Backup & restore";
const char INTL_DEVICE_BACKUP_HINT[] PROGMEM =
	"Download a full backup (settings + owner key). You can restore it here or import the same file on sensors.map Login.";
const char INTL_DEVICE_BACKUP_DOWNLOAD[] PROGMEM = "Download backup";
const char INTL_DEVICE_BACKUP_RESTORE[] PROGMEM = "Restore from backup";
const char INTL_DEVICE_BACKUP_FILE_LABEL[] PROGMEM = "Backup file";
const char INTL_DEVICE_BACKUP_FILE_CHOOSE[] PROGMEM = "Choose file…";
const char INTL_DEVICE_BACKUP_FILE_EMPTY[] PROGMEM = "No file selected";
const char INTL_DEVICE_BACKUP_FILE_REQUIRED[] PROGMEM = "Please choose a backup JSON file first.";
const char INTL_DEVICE_BACKUP_RESTORE_HINT[] PROGMEM =
	"Restoring replaces all settings on this device and restarts it.";
const char INTL_DEVICE_BACKUP_RESTORE_CONFIRM[] PROGMEM =
	"Restore settings from this backup? Current configuration will be replaced.";
const char INTL_DEVICE_BACKUP_RESTORE_OK[] PROGMEM = "Backup restored. Restarting…";
const char INTL_DEVICE_BACKUP_RESTORE_FAILED[] PROGMEM = "Could not restore backup. Check the file format.";
const char INTL_GUEST_RESTORE_HINT[] PROGMEM =
	"Have a backup from before the reset? Restore it here to bring back Wi‑Fi, owner key, and settings — then the device restarts.";
const char INTL_GROUP_OWNER_ACCESS_TITLE[] PROGMEM = "Map access (self-owner)";
const char INTL_GROUP_OWNER_ACCESS_HINT[] PROGMEM =
	"This device is its own owner. After Save we download a secret JSON once — import it on sensors.map Login to decrypt encrypted metrics. Keep the file private (local HTTP is not encrypted).";
const char INTL_GROUP_OWNER_ACCESS_SAVED_NOTICE[] PROGMEM =
	"Owner access JSON download started. Store it safely, then import it on sensors.map.";
const char INTL_GROUP_OWNER_ACCESS_DOWNLOAD_AGAIN[] PROGMEM = "Download again…";
const char INTL_GROUP_OWNER_ACCESS_REDOWNLOAD_CONFIRM[] PROGMEM =
	"Download the secret owner access JSON again? Anyone with this file can decrypt your encrypted metrics.";
const char INTL_GROUP_SAVE_FAILED[] PROGMEM = "Could not save group settings.";
const char INTL_GROUP_SAVE_CONFIG_FAILED[] PROGMEM = "Could not write configuration to storage.";
const char INTL_GROUP_ERROR_INVALID_MASTER[] PROGMEM = "Enter a valid master Robonomics address.";
const char INTL_GROUP_ERROR_INVALID_MANUAL_OWNER[] PROGMEM = "Enter a valid owner Robonomics address.";
const char INTL_SCREEN_MENU[] PROGMEM = "Screen mode";
const char INTL_SCREEN_INTRO[] PROGMEM = "Choose how the e-paper display is refreshed. Different display batches behave differently with partial updates.";
const char INTL_SCREEN_MODE_SAFE[] PROGMEM = "Safe";
const char INTL_SCREEN_MODE_SAFE_HINT[] PROGMEM = "Uses full-screen clean refreshes only. Recommended for all devices. Prevents display artifacts.";
const char INTL_SCREEN_MODE_EXPERIMENTAL[] PROGMEM = "Experimental partial refresh";
const char INTL_SCREEN_MODE_EXPERIMENTAL_HINT[] PROGMEM = "Uses faster partial screen updates. May reduce flicker, but can cause ghosting or broken image artifacts on some panels.";
const char INTL_SCREEN_SAVE_OK[] PROGMEM = "Screen mode saved.";
const char INTL_SCREEN_SAVE_FAILED[] PROGMEM = "Could not save screen mode.";
const char INTL_SCREEN_SAVE_INVALID_MODE[] PROGMEM = "Invalid screen mode selected.";
const char INTL_SCREEN_SAVE_CONFIG_FAILED[] PROGMEM = "Could not write configuration to storage.";
const char INTL_ROBONOMICS_PUBLIC_NODE[] PROGMEM = "Robonomics Public Node";
const char INTL_ROBONOMICS_CONNECTIVITY_HOST[] PROGMEM = "Robonomics Map Host (connectivity)";
const char INTL_ROBONOMICS_CONNECTIVITY_HOSTS[] PROGMEM = "Robonomics Map Hosts pool (one per line)";
const char INTL_ROBONOMICS_CONNECTIVITY_MODE_AUTO[] PROGMEM = "Default pool (auto)";
const char INTL_ROBONOMICS_CONNECTIVITY_MODE_PRESET[] PROGMEM = "Pinned (preset)";
const char INTL_ROBONOMICS_CONNECTIVITY_MODE_CUSTOM[] PROGMEM = "Custom host";
const char INTL_ROBONOMICS_CONNECTIVITY_MODE_POOL[] PROGMEM = "Custom pool";
const char INTL_ROBONOMICS_CONNECTIVITY_PRESET_LABEL[] PROGMEM = "Pinned host (preset)";
const char INTL_ROBONOMICS_CONNECTIVITY_CUSTOM_LABEL[] PROGMEM = "Custom host";
const char INTL_MAP_SEND_CSV[] PROGMEM = "Send JSON/CSV to Map (current connectivity)";
const char INTL_MAP_SEND_PROTO[] PROGMEM = "Send protobuf (new protocol)";
const char INTL_MAP_PROTO_HOST[] PROGMEM = "Protobuf URL (empty = same Map host:65/)";
const char INTL_MAP_DUAL_HINT[] PROGMEM = "Both can be on at once. JSON keeps the live map. For the test connectivity paste http://128.140.46.75/v1/telemetry below. Datalog is not duplicated.";
const char INTL_LORA_UART_ENABLED[] PROGMEM = "Send compact JSON to LoRa UART";
const char INTL_LORA_UART_INTERVAL[] PROGMEM = "LoRa sending interval (sec)";
const char INTL_LORA_UART_HINT[] PROGMEM = "Urban C6: TX GPIO22 (CN4 pin 6), RX GPIO20 (pin 8). Default 180 s (3 min); 120–300 s is typical for Meshtastic. Serial Module: enabled, TEXTMSG, 115200 (baud 11), timeout 1 s. Cross TX/RX and share GND. Compact JSON is Ed25519-signed (field s); verify by stripping s.";
const char INTL_ROBONOMICS_PUBLIC_NODE_CUSTOM[] PROGMEM = "Custom Robonomics Public Node";
const char INTL_GUEST_CONNECTED_SENSORS[] PROGMEM = "Connected Sensors";
const char INTL_COORD_LAT[] PROGMEM = "Latitude";
const char INTL_COORD_LON[] PROGMEM = "Longtitude";
const char INTL_COORDS[] PROGMEM = "GPS: Latitude, Longtitude";
const char INTL_BASICAUTH[] PROGMEM = "Authentication";
#define INTL_REPORT_ISSUE "Report an issue"

#define INTL_PANEL_TITLE_WIFI "WiFi Credentials"
#define INTL_PANEL_TITLE_ROBONOMICS "Robonomics"
#define INTL_PANEL_TITLE_GPS "GPS & Sensors"
#define INTL_PANEL_TITLE_AUTH "Authentication"
#define INTL_PANEL_TITLE_DEBUG "Debug Level"
#define INTL_PANEL_TITLE_LEDS "LEDs"
#define INTL_PANEL_TITLE_SLEEP_ANALYTICS "Sleep analytics"
#define INTL_ANALYTICS_GROUP_NIGHT "Night window"
#define INTL_ANALYTICS_GROUP_MORNING "Morning display"
#define INTL_ANALYTICS_GROUP_DATA "Data"
#define INTL_ANALYTICS_MORNING_AUTOSWITCH "Open sleep analytics on the display each morning (06:00–end)"
#define INTL_ANALYTICS_NIGHT_START_TIME "Sleep analytics night start (local, HH:MM)"
#define INTL_ANALYTICS_NIGHT_END_TIME "Sleep analytics night end (local, HH:MM)"
#define INTL_ANALYTICS_NIGHT_END_HINT "End is exclusive for hourly buckets (e.g. 07:00 uses hours through 06:xx)."
#define INTL_ANALYTICS_MORNING_END_TIME "Morning display until (local, HH:MM)"
#define INTL_ANALYTICS_MORNING_END_HINT "After this time the display returns to the main screen. End time is exclusive (10:00 means until 09:59)."
#define INTL_ANALYTICS_SLEEP_ADD_URBAN "Add Urban data to sleep analytics (PM2.5 & noise)"
#define INTL_ANALYTICS_SLEEP_ADD_URBAN_STANDALONE_HINT "Disable Insight standalone in the Firmware section to pair with Urban and use outdoor PM2.5/noise in sleep analytics."
#define INTL_PANEL_TITLE_FIRMWARE "System"
#define INTL_PANEL_TITLE_WIFI_CONFIG "WiFi in Configuration Mode"
#define INTL_PANEL_TITLE_CSV "CSV"
#define INTL_PANEL_TITLE_CUSTOMAPI "Custom API"
#define INTL_PANEL_TITLE_INFLUX "Influx DB"
#define INTL_PANEL_TITLE_DATA_SHARING "Publish to Map"
#define INTL_DATA_SHARING_DISCLAIMER "By default, all sensor data is shared to the public sensors map. You can choose which data types to share below. Unshared data will still be displayed on your device screen and available locally."
#define INTL_DATA_SHARING_ADDITIONAL "Additional sensors (optional)"
#define INTL_PANEL_TITLE_DATA_ENCRYPT "Encrypt map values"
#define INTL_DATA_ENCRYPT_DISCLAIMER "Optional. Selected metrics are sent encrypted for the device owner (CPS / AES-256-GCM). Log in as owner on sensors.map to view them."
#define INTL_DATA_ENCRYPT_BACKUP_HINT "No Robonomics seed phrase for sensors.map login? Import a device backup instead (only works with self-owner):"
#define INTL_DATA_ENCRYPT_BACKUP_LINK "Open Backup & restore"
#define INTL_DATA_ENCRYPT_KEY_LABEL "Device encryption key"
#define INTL_DATA_ENCRYPT_KEY_SHOW "Show key"
#define INTL_DATA_ENCRYPT_KEY_HIDE "Hide key"
#define INTL_DATA_ENCRYPT_KEY_COPY "Copy key"
#define INTL_DATA_ENCRYPT_KEY_COPIED "Key copied"
#define INTL_DATA_ENCRYPT_KEY_HINT "Scan with your phone camera (same Wi‑Fi) to download the key JSON file. Or tap Show key and copy the text."
#define INTL_DATA_ENCRYPT_QR_FAIL "Could not render key QR."
const char INTL_SHARE_TEMPERATURE[] PROGMEM = "Temperature";
const char INTL_SHARE_HUMIDITY[] PROGMEM = "Humidity";
const char INTL_SHARE_PRESSURE[] PROGMEM = "Pressure";
const char INTL_SHARE_CO2[] PROGMEM = "CO2";
const char INTL_SHARE_PM[] PROGMEM = "Particulate Matter (PM2.5/PM10)";
const char INTL_SHARE_NOISE[] PROGMEM = "Noise Level";
const char INTL_SHARE_CO[] PROGMEM = "Carbon monoxide (CO)";
const char INTL_SHARE_RADIATION[] PROGMEM = "Radiation";
const char INTL_SHARE_O3[] PROGMEM = "Ozone (O3)";
const char INTL_SHARE_NO2[] PROGMEM = "Nitrogen dioxide (NO2)";
const char INTL_SHARE_FAST_AQI[] PROGMEM = "FAST AQI";
const char INTL_SHARE_EPA_AQI[] PROGMEM = "EPA AQI";
const char INTL_ENCRYPT_TEMPERATURE[] PROGMEM = "Encrypt climate (temperature & humidity)";
const char INTL_ENCRYPT_HUMIDITY[] PROGMEM = "Encrypt humidity";
const char INTL_ENCRYPT_PRESSURE[] PROGMEM = "Encrypt pressure";
const char INTL_ENCRYPT_CO2[] PROGMEM = "Encrypt CO2";
const char INTL_ENCRYPT_PM[] PROGMEM = "Encrypt PM";
const char INTL_ENCRYPT_NOISE[] PROGMEM = "Encrypt noise";
const char INTL_ENCRYPT_CO[] PROGMEM = "Encrypt CO";
const char INTL_ENCRYPT_RADIATION[] PROGMEM = "Encrypt radiation";
const char INTL_ENCRYPT_O3[] PROGMEM = "Encrypt O3";
const char INTL_ENCRYPT_NO2[] PROGMEM = "Encrypt NO2";
const char INTL_ENCRYPT_FAST_AQI[] PROGMEM = "Encrypt FAST AQI";
const char INTL_ENCRYPT_EPA_AQI[] PROGMEM = "Encrypt EPA AQI";

const char INTL_FS_WIFI_DESCRIPTION[] PROGMEM = "WiFi Sensor in configuration mode";
const char INTL_FS_WIFI_NAME[] PROGMEM = "Network name";
const char INTL_MORE_SETTINGS[] PROGMEM = "Device";
const char INTL_AUTO_UPDATE[] PROGMEM = "Auto update firmware";
const char INTL_USE_BETA[] PROGMEM = "Load beta firmware";
const char INTL_DISPLAY[] PROGMEM = "OLED SSD1306";
const char INTL_SH1106[] PROGMEM = "OLED SH1106";
const char INTL_FLIP_DISPLAY[] PROGMEM = "OLED display flip";
const char INTL_LCD1602_27[] PROGMEM = "LCD 1602 (I2C: 0x27)";
const char INTL_LCD1602_3F[] PROGMEM = "LCD 1602 (I2C: 0x3F)";
const char INTL_LCD2004_27[] PROGMEM = "LCD 2004 (I2C: 0x27)";
const char INTL_LCD2004_3F[] PROGMEM = "LCD 2004 (I2C: 0x3F)";
const char INTL_DISPLAY_WIFI_INFO[] PROGMEM = "Display Wifi info";
const char INTL_DISPLAY_DEVICE_INFO[] PROGMEM = "Display device info";
const char INTL_DEBUG_LEVEL[] PROGMEM = "Debug&nbsp;level";
const char INTL_MEASUREMENT_INTERVAL[] PROGMEM = "Sending data interval (sec)";
const char INTL_LEDS_BRIGHTNESS[] PROGMEM = "LED brightness (%)";
const char INTL_LEDS_ON[] PROGMEM = "Turn LEDs on";
const char INTL_LEDS_OFF_HOUR[] PROGMEM = "Turn off at (local hour)";
const char INTL_LEDS_ON_HOUR[] PROGMEM = "Turn back on at (local hour)";
const char INTL_LEDS_SCHEDULE_HINT[] PROGMEM = "Whole hours, local device time (0 = midnight, 23 = 11 PM). LEDs dim for about 2 hours before off. Set both to the same hour to keep LEDs on overnight.";
const char INTL_SDS_MEAS_INTERVAL[] PROGMEM = "SDS measure interval (sec)";
const char INTL_DATALOG_SENDING_INTERVAL[] PROGMEM = "Datalog sending interval (sec)";
const char INTL_DURATION_ROUTER_MODE[] PROGMEM = "Duration router mode";
const char INTL_MORE_APIS[] PROGMEM = "More APIs";
const char INTL_SEND_TO_OWN_API[] PROGMEM = "Send data to custom API";
const char INTL_SERVER[] PROGMEM = "Server";
const char INTL_PATH[] PROGMEM = "Path";
const char INTL_PORT[] PROGMEM = "Port";
const char INTL_USER[] PROGMEM = "User";
const char INTL_PASSWORD[] PROGMEM = "Password";
const char INTL_LOCAL_HOSTNAME[] PROGMEM = "Local Hostname (Change it if you have more then one Altruist in the same network)";
const char INTL_MEASUREMENT[] PROGMEM = "Measurement";
const char INTL_SEND_TO[] PROGMEM = "Send to {v}";
const char INTL_READ_FROM[] PROGMEM = "Read from {v}";
const char INTL_SENSOR_IS_REBOOTING[] PROGMEM = "Sensor is rebooting.";
const char INTL_RESTART_DEVICE[] PROGMEM = "Restart device";
const char INTL_DELETE_CONFIG[] PROGMEM = "delete saved configuration";
const char INTL_RESTART_SENSOR[] PROGMEM = "Restart sensor";
#define INTL_HOME "Home"
#define INTL_BACK_TO_HOME "Back to home page"
const char INTL_CURRENT_DATA[] PROGMEM = "Current data";
const char INTL_DATA_BUSY[] PROGMEM = "Sensor data is updating — refresh in a moment.";
// Graphs screen
#define INTL_DISP_GRAPHS_HEADER_PREFIX "Current"
#define INTL_DISP_GRAPHS_HINT_LINE1 "long press ->"
#define INTL_DISP_GRAPHS_HINT_LINE2 "next/prev"
#define INTL_DISP_GRAPHS_HINT_LINE3 "screen"
const char INTL_DEVICE_STATUS[] PROGMEM = "Device status";
#define INTL_ACTIVE_SENSORS_MAP "Active sensors map (external link)"
#define INTL_CONFIGURATION_DELETE "Delete configuration"
#define INTL_CONFIGURATION_REALLY_DELETE "Are you sure you want to delete the configuration?"
#define INTL_CONFIGURATION_DELETE_WARNING "This cannot be undone. The device will restart after deleting the selected settings."
#define INTL_CONFIGURATION_DELETE_CONFIRM "Yes, delete permanently"
#define INTL_DELETE "Delete"
#define INTL_CANCEL "Cancel"
#define INTL_REALLY_RESTART_SENSOR "Are you sure you want to restart the sensor?"
#define INTL_RESTART "Restart"
const char INTL_SAVE_AND_RESTART[] PROGMEM = "Save configuration and restart";
#define INTL_FIRMWARE "Firmware:"
#define INTL_IP_ADDRESS "IP Address"
const char INTL_SD_CONNECTED[] PROGMEM = "SD Card connected";
const char INTL_FREE_RAM[] PROGMEM = "Free Memory (RAM)";
const char INTL_LAST_OTA[] PROGMEM = "Last OTA check";
#define INTL_OTA_UPDATE "Firmware Update"
const char INTL_OTA_CHECK_UPDATE[] PROGMEM = "Check for update";
const char INTL_OTA_CURRENT_VERSION[] PROGMEM = "Current version";
const char INTL_OTA_LATEST_VERSION[] PROGMEM = "Latest available";
const char INTL_OTA_UP_TO_DATE[] PROGMEM = "Firmware is up to date";
const char INTL_OTA_UPDATE_AVAILABLE[] PROGMEM = "A newer firmware is available";
const char INTL_OTA_INSTALL[] PROGMEM = "Update";
const char INTL_OTA_INSTALL_REQUESTED[] PROGMEM = "Update started. Keep Wi‑Fi connected until the device restarts.";
const char INTL_OTA_UPDATING[] PROGMEM = "Updating firmware";
const char INTL_OTA_CHECK_FAILED[] PROGMEM = "Could not check for updates. Try again later.";
const char INTL_OTA_CHECK_REQUESTED[] PROGMEM = "Update check requested. The device will download and install the new firmware if available.";
const char INTL_OTA_NO_WIFI[] PROGMEM = "WiFi is not connected. Cannot check for updates.";
const char INTL_OTA_SWITCH_LANG[] PROGMEM = "Switch language";
const char INTL_OTA_CURRENT_LANG[] PROGMEM = "Current language";
const char INTL_OTA_SWITCH_LANG_NOTE[] PROGMEM = "Device will download and install firmware in the selected language";
const char INTL_OTA_LANG_SAME[] PROGMEM = "Already using this language.";
const char INTL_OTA_LANG_REQUESTED[] PROGMEM = "Language switch requested. Please wait. The device will download and install firmware in the selected language.";
const char INTL_UPTIME[] PROGMEM = "Uptime";
const char INTL_RESET_REASON[] PROGMEM = "Reset Reason";
const char INTL_OTA_RETURN[] PROGMEM = "OTA Return";
const char INTL_COUNT_SUCCESS_SENDS[] PROGMEM = "count success sends";
const char INTL_LAST_SEND_TIME[] PROGMEM = "last send time";
#define INTL_CHIP_TYPE "Chip type"
#define INTL_ROBONOMICS_ADDR "Robonomics Address"
const char INTL_DEBUG_SETTING_TO[] PROGMEM = "Set debug level to";
#define INTL_NONE "off"
#define INTL_ERROR "only errors"
#define INTL_WARNING "warnings"
#define INTL_MIN_INFO "min. info"
#define INTL_MED_INFO "mid. info"
#define INTL_MAX_INFO "max. info"
#define INTL_CONFIG_DELETED "Configuration was deleted"
#define INTL_CONFIG_CAN_NOT_BE_DELETED "Configuration can not be deleted"
#define INTL_CONFIG_NOT_FOUND "Configuration not found"
const char INTL_TIME_TO_FIRST_MEASUREMENT[] PROGMEM = "Still {v} seconds until first measurement.";
const char INTL_TIME_SINCE_LAST_MEASUREMENT[] PROGMEM = " seconds since last measurement.";
const char INTL_PARTICLES_PER_LITER[] PROGMEM = "particles/liter";
const char INTL_PARTICULATE_MATTER[] PROGMEM = "particulate matter";
const char INTL_TEMPERATURE[] PROGMEM = "temperature";
const char INTL_NOISE[] PROGMEM = "noise";
const char INTL_NOISE_MAX[] PROGMEM = "max noise";
const char INTL_NOISE_MEAN[] PROGMEM = "mean noise";
const char INTL_HUMIDITY[] PROGMEM = "humidity";
const char INTL_PRESSURE[] PROGMEM = "air pressure";
const char INTL_RADIATION[] PROGMEM = "Radiation";
const char INTL_CO2[] PROGMEM = "CO2";
const char INTL_LEQ_A[] PROGMEM = "LAeq";
const char INTL_LA_MIN[] PROGMEM = "LA min";
const char INTL_LA_MAX[] PROGMEM = "LA max";
const char INTL_LATITUDE[] PROGMEM = "Latitude";
const char INTL_LONGITUDE[] PROGMEM = "Longitude";
const char INTL_ALTITUDE[] PROGMEM = "Altitude";
const char INTL_TIME_LOCAL[] PROGMEM = "Time";
const char INTL_SIGNAL_STRENGTH[] PROGMEM = "signal strength";
const char INTL_SIGNAL_QUALITY[] PROGMEM = "signal quality";
#define INTL_NUMBER_OF_MEASUREMENTS "Number of measurements"
#define INTL_TIME_SENDING_MS "Time spent uploading"
#define INTL_SENSOR "Sensor"
#define INTL_PARAMETER "Parameter"
#define INTL_VALUE "Value"

#define INTL_DATA_SECTION_SDS "SDS"
#define INTL_DATA_SECTION_SCD "SCD4x"
#define INTL_DATA_SECTION_BME "BME"
#define INTL_DATA_SECTION_URBAN "Urban data"
#define INTL_DATA_SECTION_OVERVIEW "Overview"
#define INTL_DATA_SECTION_DEVICE "Device"
#define INTL_DATA_SECTION_RUNTIME "Runtime"
#define INTL_DATA_SECTION_NETWORK "Network"
#define INTL_DATA_SECTION_EXPORT "Data export"
#define INTL_DATA_SECTION_TECHNICAL "Technical details"
#define INTL_COPY_ALL "Copy all"
#define INTL_COPIED "Copied"
#define INTL_TOPBAR_ONLINE "Online"
#define INTL_TOPBAR_OFFLINE "Offline"
#define INTL_TOPBAR_SEND_ISSUE "Send issue"
#define INTL_TOPBAR_SEND_OK "OK"
#define INTL_TOPBAR_SEND_ERR "Not send"
#define INTL_TOPBAR_LAST_SEND "Last send"
#define INTL_TOPBAR_NO_SENDS "No sends yet"
#define INTL_TOPBAR_MAP "Map"
#define INTL_TOPBAR_DATALOG "Datalog"
#define INTL_TOPBAR_JUST_NOW "just now"
#define INTL_TOPBAR_WIFI "Wi‑Fi"
#define INTL_TOPBAR_HOST "Hostname"
#define INTL_TOPBAR_DEVICE "Device"
#define INTL_TOPBAR_SEND "Send"
#define INTL_TOPBAR_STANDALONE "Standalone"
#define INTL_TOPBAR_PAIRED "Urban paired"
#define INTL_TOPBAR_UPDATE "Update"
#define INTL_TOPBAR_FW_CURRENT "Up to date"
#define INTL_TOPBAR_ROBONOMICS "Robonomics address"
#define INTL_TOPBAR_Q_ACCESS "Open in browser"
#define INTL_TOPBAR_TAP_COPY "Tap to copy"
#define INTL_TOPBAR_MORE "Device info"
#define INTL_VALUE_YES "Yes"
#define INTL_VALUE_NO "No"
#define INTL_READINGS_SECTION_NETWORK_INTRO "Wi-Fi signal strength at the sensor."
#define INTL_STATUS_SECTION_OVERVIEW_INTRO "Is the device running normally right now?"
#define INTL_STATUS_SECTION_DEVICE_INTRO "Firmware version, memory, and storage."
#define INTL_STATUS_SECTION_TECH_INTRO "Build details — useful when contacting support."
#define INTL_STATUS_SECTION_EXPORT_INTRO "Whether your data is reaching each service."
#define INTL_API_SENDS_SHORT "Sends"
#define INTL_API_LAST_SHORT "Last"

#define INTL_REGION "Region"
#define INTL_REGION_GLOBAL "Global"
#define INTL_REGION_RU "Russia"
#define INTL_REGION_HINT "Used for sensors.social connectivity. Auto from coordinates / OTA unless you change it here."
#define INTL_REGION_EU "Europe"
#define INTL_REGION_AS "Asia"
#define INTL_REGION_AF "Africa"
#define INTL_REGION_AU "Australia"
#define INTL_REGION_NA "North America"
#define INTL_REGION_SA "South America"

/* Insight display (e-paper) strings */
#define INTL_DISP_PRODUCT_INSIGHT "Altruist Insight"
#define INTL_DISP_WIFI_CLEARED "WiFi credentials cleared"
#define INTL_DISP_RESTARTING "Restarting device..."
#define INTL_DISP_WIFI_SETUP "Wi-Fi Setup"
#define INTL_DISP_CONNECT_TO "Connect to"
#define INTL_DISP_PASSWORD_PREFIX "Password: "
#define INTL_DISP_TITLE_INSIGHT "ALTRUIST INSIGHT"
#define INTL_DISP_CONNECTING_WIFI "Connecting to Wi-Fi"
#define INTL_DISP_PLEASE_WAIT "Please wait..."
#define INTL_DISP_SD_NOT_FOUND "SD card not found"
#define INTL_DISP_INSERT_SD "Please insert SD card"
#define INTL_DISP_FAT32_FORMATTED "(FAT32 formatted)"
#define INTL_DISP_NO_DATA_FILES "No data files found"
#define INTL_DISP_DEVICE_WILL_CREATE "Device will create"
#define INTL_DISP_FILES_AUTOMATICALLY "files automatically"
#define INTL_DISP_AFTER_COLLECTING "after collecting data"
#define INTL_DISP_SD_NOT_AVAILABLE "SD card not available"
#define INTL_DISP_GRAPHS_REQUIRE_SD "Graphs require SD card"
#define INTL_DISP_ENABLE_SD "Please enable SD card"
#define INTL_DISP_INSIGHT_HEADER "Insight"
#define INTL_DISP_INSIGHT_ONLY "Insight only"
#define INTL_DISP_URBAN_HEADER "Urban"
#define INTL_DISP_URBAN_ONLY "Urban only"
#define INTL_DISP_GOING_TO_SLEEP "Going to sleep..."
#define INTL_DISP_OTA_UPDATING "Updating firmware"
#define INTL_DISP_OTA_DO_NOT_DISCONNECT "Do not disconnect power"
#define INTL_DISP_OTA_FAILED "Update failed"
#define INTL_DISP_OTA_WILL_RETRY "Will retry later"
#define INTL_DISP_OTA_SUCCESS "Firmware updated"
#define INTL_DISP_OTA_RESTARTING "Restarting..."
#define INTL_DISP_WAITING_URBAN_ID "Waiting for Urban ID..."
#define INTL_DISP_URBAN_IP "Urban IP:"
#define INTL_DISP_INSIGHT_IP "Insight IP:"
#define INTL_DISP_SD_CARD "SD Card:"
#define INTL_DISP_WIFI_STATUS "WiFi Status:"
#define INTL_DISP_WIFI_NAME "WiFi Name:"
#define INTL_DISP_UNIQUE_ADDR "Unique Addr:"
#define INTL_DISP_DEVICE_INFO "Device info"
#define INTL_DISP_SCAN_FOR_MORE "Scan for more"
#define INTL_DISP_NO_DATA "--"
#define INTL_DISP_TEMPERATURE "Temperature"
#define INTL_DISP_HUMIDITY "Humidity"
#define INTL_DISP_PRESSURE "Pressure"
#define INTL_DISP_AIR "Air"
#define INTL_DISP_AIR_QUALITY "Air quality"
#define INTL_DISP_NOISE "Noise"
#define INTL_DISP_MAIN_URBAN "URBAN"
#define INTL_DISP_MAIN_INSIGHT "INSIGHT"
#define INTL_DISP_SENSORS_MAP "SENSORS MAP"
#define INTL_DISP_EXPLORE_ADVANTAGES "Explore all advantages"
#define INTL_DISP_EXPLORE_ENVIRONMENT "Explore your environment"
#define INTL_DISP_EXPLORE_YOUR "Explore your"
#define INTL_DISP_ENVIRONMENT_CAPS "ENVIRONMENT"
#define INTL_DISP_MAP_ENV_BETTER "Know your environment better."
#define INTL_DISP_MAP_REVIEW_INSIGHTS "Review insights over time."
#define INTL_DISP_MAP_COMPARE_CONDITIONS "Compare with others nearby."
#define INTL_DISP_SCAN_TO_OPEN "Scan to open online"
#define INTL_DISP_POWERED_BY "Powered by Robonomics"
#define INTL_DISP_POWERED "Powered"
#define INTL_DISP_BY_ROBONOMICS "by Robonomics"
#define INTL_DISP_NOT_CONNECTED "Not connected"
#define INTL_DISP_CONNECTED "Connected"
#define INTL_DISP_DISCONNECTED "Disconnected"
#define INTL_DISP_NOT_SET "Not set"
#define INTL_DISP_NOISE_MAX "Noise Max."
#define INTL_DISP_NOISE_AVG "Noise Avg."
#define INTL_DISP_NO_DATA_AVAILABLE "No data available"
#define INTL_DISP_NOT_ENOUGH_DATA_YET "Not enough data yet"
#define INTL_DISP_COLLECTING_DATA "Collecting data..."
#define INTL_DISP_LOADING "Loading..."
#define INTL_DISP_ANALYTICS_C_LEGEND "C=Conservative"
#define INTL_DISP_ANALYTICS_B_LEGEND "B=Biohacking"
#define INTL_DISP_ANALYTICS_GRADE "Grade"
#define INTL_DISP_ANALYTICS_COL_METRIC "Metric"
#define INTL_DISP_ANALYTICS_COL_MAX "Max"
#define INTL_DISP_ANALYTICS_COL_MIN "Min"
#define INTL_DISP_ANALYTICS_COL_CONSERV "Conserv"
#define INTL_DISP_ANALYTICS_COL_BIOHACK "Biohack"
#define INTL_DISP_ANALYTICS_ROW_CO2 "CO2 ppm"
#define INTL_DISP_ANALYTICS_ROW_TEMP "Temperature C"
#define INTL_DISP_ANALYTICS_ROW_HUM "Humidity %"
#define INTL_DISP_ANALYTICS_ROW_PM25 "PM2.5 ug/m3"
#define INTL_DISP_ANALYTICS_ROW_NOISE "Noise dB"
#define INTL_DISP_ANALYTICS_AT "at"
#define INTL_DISP_ANALYTICS_HOUR_SUFFIX "h"
#define INTL_DISP_INFO_LABEL "Info:"
#define INTL_DISP_LEVEL_HIGH "high"
#define INTL_DISP_LEVEL_LOW "low"
#define INTL_DISP_IS_TOO "is too"
#define INTL_DISP_CHECK_MAP_FULL_DATA "Check out our sensor map for full data and analytics."
/** Legacy short shop line (unused in two-row footer; kept for intl parity). */
#define INTL_STANDALONE_SHOP_PROMPT "Add more measurements to your home"
/** Insight standalone: second footer row beside shop QR (Font8, wraps). */
#define INTL_STANDALONE_INSIGHT_FOOTER_PROMPT \
    "Add noise, air dust and outdoor atmospheric measurements to your Insight."
#define INTL_DISP_DEW_POINT_U_PREFIX "Dew Point (U): "
#define INTL_DISP_DEW_POINT_IS "Dew Point is "
#define INTL_DISP_TEMP_SHORT "Temp"
#define INTL_DISP_PRESS_SHORT "Press."
#define INTL_DISP_NOISE_AVGMAX_SUFFIX "(avg | max)"

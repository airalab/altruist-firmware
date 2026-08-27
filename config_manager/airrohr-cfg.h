
#ifndef __CONFIG_H__
#define __CONFIG_H__

// This file is generated, please do not edit.
// Change airrohr-cfg.h.py instead.
          
#include "config_defaults.h"

enum ConfigEntryType : unsigned short {
	Config_Type_Bool,
	Config_Type_UInt,
	Config_Type_Time,
	Config_Type_String,
	Config_Type_Password
};

struct ConfigShapeEntry {
	enum ConfigEntryType cfg_type;
	unsigned short cfg_len;
	const char* _cfg_key;
	union {
		void* as_void;
		bool* as_bool;
		unsigned int* as_uint;
		char* as_str;
	} cfg_val;
	const __FlashStringHelper* cfg_key() const { return FPSTR(_cfg_key); }
};

enum ConfigShapeId {
	Config_current_lang,
	Config_wlanssid,
	Config_wlanpwd,
	Config_wlannopwd,
	Config_www_username,
	Config_www_password,
	Config_fs_ssid,
	Config_fs_pwd,
	Config_www_basicauth_enabled,
	Config_rws_owner,
	Config_robonomics_public_node,
	Config_robonomics_connectivity_host,
	Config_robonomics_connectivity_hosts,
	Config_robonomics_proto_connectivity_host,
	Config_map_send_csv,
	Config_map_send_proto,
	Config_rws_auto_register,
	Config_rws_group_mode,
	Config_rws_group_id,
	Config_rws_devices_extra,
	Config_rws_devices_registered_hash,
	Config_private_key,
	Config_coords_gps,
	Config_send2robonomics,
	Config_send2csv,
	Config_lora_uart_enabled,
	Config_auto_update,
	Config_use_beta,
	Config_has_display,
	Config_has_sh1106,
	Config_has_flipped_display,
	Config_debug,
	Config_sending_intervall_ms,
	Config_datalog_sending_intervall_ms,
	Config_sds_meas_interval_ms,
	Config_time_for_wifi_config,
	Config_send2custom,
	Config_host_custom,
	Config_url_custom,
	Config_port_custom,
	Config_user_custom,
	Config_pwd_custom,
	Config_ssl_custom,
	Config_send2influx,
	Config_host_influx,
	Config_url_influx,
	Config_port_influx,
	Config_user_influx,
	Config_pwd_influx,
	Config_measurement_name_influx,
	Config_ssl_influx,
	Config_donated_by,
	Config_current_reg,
	Config_region_manual,
	Config_temp_correction,
	Config_local_hostname,
	Config_chosen_altruist_urban,
	Config_timezone,
	Config_leds_brightness,
	Config_leds_on,
	Config_leds_off_hour,
	Config_leds_on_hour,
	Config_analytics_night_start_hour,
	Config_analytics_night_end_hour,
	Config_custom_altruist_urban,
	Config_use_custom_urban,
	Config_standalone,
	Config_epd_refresh_mode,
	Config_analytics_sleep_add_urban,
	Config_analytics_morning_autoswitch,
	Config_analytics_morning_end_hour,
	Config_share_temperature,
	Config_share_humidity,
	Config_share_pressure,
	Config_share_co2,
	Config_share_pm,
	Config_share_noise,
	Config_share_co,
	Config_share_radiation,
	Config_share_o3,
	Config_share_no2,
	Config_share_fast_aqi,
	Config_share_epa_aqi,
	Config_encrypt_temperature,
	Config_encrypt_humidity,
	Config_encrypt_pressure,
	Config_encrypt_co2,
	Config_encrypt_pm,
	Config_encrypt_noise,
	Config_encrypt_co,
	Config_encrypt_radiation,
	Config_encrypt_o3,
	Config_encrypt_no2,
	Config_encrypt_fast_aqi,
	Config_encrypt_epa_aqi,
};
static constexpr char CFG_KEY_CURRENT_LANG[] PROGMEM = "current_lang";
static constexpr char CFG_KEY_WLANSSID[] PROGMEM = "wlanssid";
static constexpr char CFG_KEY_WLANPWD[] PROGMEM = "wlanpwd";
static constexpr char CFG_KEY_WLANNOPWD[] PROGMEM = "wlannopwd";
static constexpr char CFG_KEY_WWW_USERNAME[] PROGMEM = "www_username";
static constexpr char CFG_KEY_WWW_PASSWORD[] PROGMEM = "www_password";
static constexpr char CFG_KEY_FS_SSID[] PROGMEM = "fs_ssid";
static constexpr char CFG_KEY_FS_PWD[] PROGMEM = "fs_pwd";
static constexpr char CFG_KEY_WWW_BASICAUTH_ENABLED[] PROGMEM = "www_basicauth_enabled";
static constexpr char CFG_KEY_RWS_OWNER[] PROGMEM = "rws_owner";
static constexpr char CFG_KEY_ROBONOMICS_PUBLIC_NODE[] PROGMEM = "robonomics_public_node";
static constexpr char CFG_KEY_ROBONOMICS_CONNECTIVITY_HOST[] PROGMEM = "robonomics_connectivity_host";
static constexpr char CFG_KEY_ROBONOMICS_CONNECTIVITY_HOSTS[] PROGMEM = "robonomics_connectivity_hosts";
static constexpr char CFG_KEY_ROBONOMICS_PROTO_CONNECTIVITY_HOST[] PROGMEM = "robonomics_proto_connectivity_host";
static constexpr char CFG_KEY_MAP_SEND_CSV[] PROGMEM = "map_send_csv";
static constexpr char CFG_KEY_MAP_SEND_PROTO[] PROGMEM = "map_send_proto";
static constexpr char CFG_KEY_RWS_AUTO_REGISTER[] PROGMEM = "rws_auto_register";
static constexpr char CFG_KEY_RWS_GROUP_MODE[] PROGMEM = "rws_group_mode";
static constexpr char CFG_KEY_RWS_GROUP_ID[] PROGMEM = "rws_group_id";
static constexpr char CFG_KEY_RWS_DEVICES_EXTRA[] PROGMEM = "rws_devices_extra";
static constexpr char CFG_KEY_RWS_DEVICES_REGISTERED_HASH[] PROGMEM = "rws_devices_registered_hash";
static constexpr char CFG_KEY_PRIVATE_KEY[] PROGMEM = "private_key";
static constexpr char CFG_KEY_COORDS_GPS[] PROGMEM = "coords_gps";
static constexpr char CFG_KEY_SEND2ROBONOMICS[] PROGMEM = "send2robonomics";
static constexpr char CFG_KEY_SEND2CSV[] PROGMEM = "send2csv";
static constexpr char CFG_KEY_LORA_UART_ENABLED[] PROGMEM = "lora_uart_enabled";
static constexpr char CFG_KEY_AUTO_UPDATE[] PROGMEM = "auto_update";
static constexpr char CFG_KEY_USE_BETA[] PROGMEM = "use_beta";
static constexpr char CFG_KEY_HAS_DISPLAY[] PROGMEM = "has_display";
static constexpr char CFG_KEY_HAS_SH1106[] PROGMEM = "has_sh1106";
static constexpr char CFG_KEY_HAS_FLIPPED_DISPLAY[] PROGMEM = "has_flipped_display";
static constexpr char CFG_KEY_DEBUG[] PROGMEM = "debug";
static constexpr char CFG_KEY_SENDING_INTERVALL_MS[] PROGMEM = "sending_intervall_ms";
static constexpr char CFG_KEY_DATALOG_SENDING_INTERVALL_MS[] PROGMEM = "datalog_sending_intervall_ms";
static constexpr char CFG_KEY_SDS_MEAS_INTERVAL_MS[] PROGMEM = "sds_meas_interval_ms";
static constexpr char CFG_KEY_TIME_FOR_WIFI_CONFIG[] PROGMEM = "time_for_wifi_config";
static constexpr char CFG_KEY_SEND2CUSTOM[] PROGMEM = "send2custom";
static constexpr char CFG_KEY_HOST_CUSTOM[] PROGMEM = "host_custom";
static constexpr char CFG_KEY_URL_CUSTOM[] PROGMEM = "url_custom";
static constexpr char CFG_KEY_PORT_CUSTOM[] PROGMEM = "port_custom";
static constexpr char CFG_KEY_USER_CUSTOM[] PROGMEM = "user_custom";
static constexpr char CFG_KEY_PWD_CUSTOM[] PROGMEM = "pwd_custom";
static constexpr char CFG_KEY_SSL_CUSTOM[] PROGMEM = "ssl_custom";
static constexpr char CFG_KEY_SEND2INFLUX[] PROGMEM = "send2influx";
static constexpr char CFG_KEY_HOST_INFLUX[] PROGMEM = "host_influx";
static constexpr char CFG_KEY_URL_INFLUX[] PROGMEM = "url_influx";
static constexpr char CFG_KEY_PORT_INFLUX[] PROGMEM = "port_influx";
static constexpr char CFG_KEY_USER_INFLUX[] PROGMEM = "user_influx";
static constexpr char CFG_KEY_PWD_INFLUX[] PROGMEM = "pwd_influx";
static constexpr char CFG_KEY_MEASUREMENT_NAME_INFLUX[] PROGMEM = "measurement_name_influx";
static constexpr char CFG_KEY_SSL_INFLUX[] PROGMEM = "ssl_influx";
static constexpr char CFG_KEY_DONATED_BY[] PROGMEM = "donated_by";
static constexpr char CFG_KEY_CURRENT_REG[] PROGMEM = "current_reg";
static constexpr char CFG_KEY_REGION_MANUAL[] PROGMEM = "region_manual";
static constexpr char CFG_KEY_TEMP_CORRECTION[] PROGMEM = "temp_correction";
static constexpr char CFG_KEY_LOCAL_HOSTNAME[] PROGMEM = "local_hostname";
static constexpr char CFG_KEY_CHOSEN_ALTRUIST_URBAN[] PROGMEM = "chosen_altruist_urban";
static constexpr char CFG_KEY_TIMEZONE[] PROGMEM = "timezone";
static constexpr char CFG_KEY_LEDS_BRIGHTNESS[] PROGMEM = "leds_brightness";
static constexpr char CFG_KEY_LEDS_ON[] PROGMEM = "leds_on";
static constexpr char CFG_KEY_LEDS_OFF_HOUR[] PROGMEM = "leds_off_hour";
static constexpr char CFG_KEY_LEDS_ON_HOUR[] PROGMEM = "leds_on_hour";
static constexpr char CFG_KEY_ANALYTICS_NIGHT_START_HOUR[] PROGMEM = "analytics_night_start_hour";
static constexpr char CFG_KEY_ANALYTICS_NIGHT_END_HOUR[] PROGMEM = "analytics_night_end_hour";
static constexpr char CFG_KEY_CUSTOM_ALTRUIST_URBAN[] PROGMEM = "custom_altruist_urban";
static constexpr char CFG_KEY_USE_CUSTOM_URBAN[] PROGMEM = "use_custom_urban";
static constexpr char CFG_KEY_STANDALONE[] PROGMEM = "standalone";
static constexpr char CFG_KEY_EPD_REFRESH_MODE[] PROGMEM = "epd_refresh_mode";
static constexpr char CFG_KEY_ANALYTICS_SLEEP_ADD_URBAN[] PROGMEM = "analytics_sleep_add_urban";
static constexpr char CFG_KEY_ANALYTICS_MORNING_AUTOSWITCH[] PROGMEM = "analytics_morning_autoswitch";
static constexpr char CFG_KEY_ANALYTICS_MORNING_END_HOUR[] PROGMEM = "analytics_morning_end_hour";
static constexpr char CFG_KEY_SHARE_TEMPERATURE[] PROGMEM = "share_temperature";
static constexpr char CFG_KEY_SHARE_HUMIDITY[] PROGMEM = "share_humidity";
static constexpr char CFG_KEY_SHARE_PRESSURE[] PROGMEM = "share_pressure";
static constexpr char CFG_KEY_SHARE_CO2[] PROGMEM = "share_co2";
static constexpr char CFG_KEY_SHARE_PM[] PROGMEM = "share_pm";
static constexpr char CFG_KEY_SHARE_NOISE[] PROGMEM = "share_noise";
static constexpr char CFG_KEY_SHARE_CO[] PROGMEM = "share_co";
static constexpr char CFG_KEY_SHARE_RADIATION[] PROGMEM = "share_radiation";
static constexpr char CFG_KEY_SHARE_O3[] PROGMEM = "share_o3";
static constexpr char CFG_KEY_SHARE_NO2[] PROGMEM = "share_no2";
static constexpr char CFG_KEY_SHARE_FAST_AQI[] PROGMEM = "share_fast_aqi";
static constexpr char CFG_KEY_SHARE_EPA_AQI[] PROGMEM = "share_epa_aqi";
static constexpr char CFG_KEY_ENCRYPT_TEMPERATURE[] PROGMEM = "encrypt_temperature";
static constexpr char CFG_KEY_ENCRYPT_HUMIDITY[] PROGMEM = "encrypt_humidity";
static constexpr char CFG_KEY_ENCRYPT_PRESSURE[] PROGMEM = "encrypt_pressure";
static constexpr char CFG_KEY_ENCRYPT_CO2[] PROGMEM = "encrypt_co2";
static constexpr char CFG_KEY_ENCRYPT_PM[] PROGMEM = "encrypt_pm";
static constexpr char CFG_KEY_ENCRYPT_NOISE[] PROGMEM = "encrypt_noise";
static constexpr char CFG_KEY_ENCRYPT_CO[] PROGMEM = "encrypt_co";
static constexpr char CFG_KEY_ENCRYPT_RADIATION[] PROGMEM = "encrypt_radiation";
static constexpr char CFG_KEY_ENCRYPT_O3[] PROGMEM = "encrypt_o3";
static constexpr char CFG_KEY_ENCRYPT_NO2[] PROGMEM = "encrypt_no2";
static constexpr char CFG_KEY_ENCRYPT_FAST_AQI[] PROGMEM = "encrypt_fast_aqi";
static constexpr char CFG_KEY_ENCRYPT_EPA_AQI[] PROGMEM = "encrypt_epa_aqi";
static constexpr ConfigShapeEntry configShape[] PROGMEM = {
	{ Config_Type_String, sizeof(cfg::current_lang)-1, CFG_KEY_CURRENT_LANG, cfg::current_lang },
	{ Config_Type_String, sizeof(cfg::wlanssid)-1, CFG_KEY_WLANSSID, cfg::wlanssid },
	{ Config_Type_Password, sizeof(cfg::wlanpwd)-1, CFG_KEY_WLANPWD, cfg::wlanpwd },
	{ Config_Type_Bool, 0, CFG_KEY_WLANNOPWD, &cfg::wlannopwd },
	{ Config_Type_String, sizeof(cfg::www_username)-1, CFG_KEY_WWW_USERNAME, cfg::www_username },
	{ Config_Type_Password, sizeof(cfg::www_password)-1, CFG_KEY_WWW_PASSWORD, cfg::www_password },
	{ Config_Type_String, sizeof(cfg::fs_ssid)-1, CFG_KEY_FS_SSID, cfg::fs_ssid },
	{ Config_Type_Password, sizeof(cfg::fs_pwd)-1, CFG_KEY_FS_PWD, cfg::fs_pwd },
	{ Config_Type_Bool, 0, CFG_KEY_WWW_BASICAUTH_ENABLED, &cfg::www_basicauth_enabled },
	{ Config_Type_String, sizeof(cfg::rws_owner)-1, CFG_KEY_RWS_OWNER, cfg::rws_owner },
	{ Config_Type_String, sizeof(cfg::robonomics_public_node)-1, CFG_KEY_ROBONOMICS_PUBLIC_NODE, cfg::robonomics_public_node },
	{ Config_Type_String, sizeof(cfg::robonomics_connectivity_host)-1, CFG_KEY_ROBONOMICS_CONNECTIVITY_HOST, cfg::robonomics_connectivity_host },
	{ Config_Type_String, sizeof(cfg::robonomics_connectivity_hosts)-1, CFG_KEY_ROBONOMICS_CONNECTIVITY_HOSTS, cfg::robonomics_connectivity_hosts },
	{ Config_Type_String, sizeof(cfg::robonomics_proto_connectivity_host)-1, CFG_KEY_ROBONOMICS_PROTO_CONNECTIVITY_HOST, cfg::robonomics_proto_connectivity_host },
	{ Config_Type_Bool, 0, CFG_KEY_MAP_SEND_CSV, &cfg::map_send_csv },
	{ Config_Type_Bool, 0, CFG_KEY_MAP_SEND_PROTO, &cfg::map_send_proto },
	{ Config_Type_Bool, 0, CFG_KEY_RWS_AUTO_REGISTER, &cfg::rws_auto_register },
	{ Config_Type_UInt, 0, CFG_KEY_RWS_GROUP_MODE, &cfg::rws_group_mode },
	{ Config_Type_String, sizeof(cfg::rws_group_id)-1, CFG_KEY_RWS_GROUP_ID, cfg::rws_group_id },
	{ Config_Type_String, sizeof(cfg::rws_devices_extra)-1, CFG_KEY_RWS_DEVICES_EXTRA, cfg::rws_devices_extra },
	{ Config_Type_String, sizeof(cfg::rws_devices_registered_hash)-1, CFG_KEY_RWS_DEVICES_REGISTERED_HASH, cfg::rws_devices_registered_hash },
	{ Config_Type_String, sizeof(cfg::private_key)-1, CFG_KEY_PRIVATE_KEY, cfg::private_key },
	{ Config_Type_String, sizeof(cfg::coords_gps)-1, CFG_KEY_COORDS_GPS, cfg::coords_gps },
	{ Config_Type_Bool, 0, CFG_KEY_SEND2ROBONOMICS, &cfg::send2robonomics },
	{ Config_Type_Bool, 0, CFG_KEY_SEND2CSV, &cfg::send2csv },
	{ Config_Type_Bool, 0, CFG_KEY_LORA_UART_ENABLED, &cfg::lora_uart_enabled },
	{ Config_Type_Bool, 0, CFG_KEY_AUTO_UPDATE, &cfg::auto_update },
	{ Config_Type_Bool, 0, CFG_KEY_USE_BETA, &cfg::use_beta },
	{ Config_Type_Bool, 0, CFG_KEY_HAS_DISPLAY, &cfg::has_display },
	{ Config_Type_Bool, 0, CFG_KEY_HAS_SH1106, &cfg::has_sh1106 },
	{ Config_Type_Bool, 0, CFG_KEY_HAS_FLIPPED_DISPLAY, &cfg::has_flipped_display },
	{ Config_Type_UInt, 0, CFG_KEY_DEBUG, &cfg::debug },
	{ Config_Type_Time, 0, CFG_KEY_SENDING_INTERVALL_MS, &cfg::sending_intervall_ms },
	{ Config_Type_Time, 0, CFG_KEY_DATALOG_SENDING_INTERVALL_MS, &cfg::datalog_sending_intervall_ms },
	{ Config_Type_Time, 0, CFG_KEY_SDS_MEAS_INTERVAL_MS, &cfg::sds_meas_interval_ms },
	{ Config_Type_Time, 0, CFG_KEY_TIME_FOR_WIFI_CONFIG, &cfg::time_for_wifi_config },
	{ Config_Type_Bool, 0, CFG_KEY_SEND2CUSTOM, &cfg::send2custom },
	{ Config_Type_String, sizeof(cfg::host_custom)-1, CFG_KEY_HOST_CUSTOM, cfg::host_custom },
	{ Config_Type_String, sizeof(cfg::url_custom)-1, CFG_KEY_URL_CUSTOM, cfg::url_custom },
	{ Config_Type_UInt, 0, CFG_KEY_PORT_CUSTOM, &cfg::port_custom },
	{ Config_Type_String, sizeof(cfg::user_custom)-1, CFG_KEY_USER_CUSTOM, cfg::user_custom },
	{ Config_Type_Password, sizeof(cfg::pwd_custom)-1, CFG_KEY_PWD_CUSTOM, cfg::pwd_custom },
	{ Config_Type_Bool, 0, CFG_KEY_SSL_CUSTOM, &cfg::ssl_custom },
	{ Config_Type_Bool, 0, CFG_KEY_SEND2INFLUX, &cfg::send2influx },
	{ Config_Type_String, sizeof(cfg::host_influx)-1, CFG_KEY_HOST_INFLUX, cfg::host_influx },
	{ Config_Type_String, sizeof(cfg::url_influx)-1, CFG_KEY_URL_INFLUX, cfg::url_influx },
	{ Config_Type_UInt, 0, CFG_KEY_PORT_INFLUX, &cfg::port_influx },
	{ Config_Type_String, sizeof(cfg::user_influx)-1, CFG_KEY_USER_INFLUX, cfg::user_influx },
	{ Config_Type_Password, sizeof(cfg::pwd_influx)-1, CFG_KEY_PWD_INFLUX, cfg::pwd_influx },
	{ Config_Type_String, sizeof(cfg::measurement_name_influx)-1, CFG_KEY_MEASUREMENT_NAME_INFLUX, cfg::measurement_name_influx },
	{ Config_Type_Bool, 0, CFG_KEY_SSL_INFLUX, &cfg::ssl_influx },
	{ Config_Type_String, sizeof(cfg::donated_by)-1, CFG_KEY_DONATED_BY, cfg::donated_by },
	{ Config_Type_String, sizeof(cfg::current_reg)-1, CFG_KEY_CURRENT_REG, cfg::current_reg },
	{ Config_Type_Bool, 0, CFG_KEY_REGION_MANUAL, &cfg::region_manual },
	{ Config_Type_String, sizeof(cfg::temp_correction)-1, CFG_KEY_TEMP_CORRECTION, cfg::temp_correction },
	{ Config_Type_String, sizeof(cfg::local_hostname)-1, CFG_KEY_LOCAL_HOSTNAME, cfg::local_hostname },
	{ Config_Type_String, sizeof(cfg::chosen_altruist_urban)-1, CFG_KEY_CHOSEN_ALTRUIST_URBAN, cfg::chosen_altruist_urban },
	{ Config_Type_String, sizeof(cfg::timezone)-1, CFG_KEY_TIMEZONE, cfg::timezone },
	{ Config_Type_UInt, 0, CFG_KEY_LEDS_BRIGHTNESS, &cfg::leds_brightness },
	{ Config_Type_Bool, 0, CFG_KEY_LEDS_ON, &cfg::leds_on },
	{ Config_Type_UInt, 0, CFG_KEY_LEDS_OFF_HOUR, &cfg::leds_off_hour },
	{ Config_Type_UInt, 0, CFG_KEY_LEDS_ON_HOUR, &cfg::leds_on_hour },
	{ Config_Type_UInt, 0, CFG_KEY_ANALYTICS_NIGHT_START_HOUR, &cfg::analytics_night_start_hour },
	{ Config_Type_UInt, 0, CFG_KEY_ANALYTICS_NIGHT_END_HOUR, &cfg::analytics_night_end_hour },
	{ Config_Type_String, sizeof(cfg::custom_altruist_urban)-1, CFG_KEY_CUSTOM_ALTRUIST_URBAN, cfg::custom_altruist_urban },
	{ Config_Type_Bool, 0, CFG_KEY_USE_CUSTOM_URBAN, &cfg::use_custom_urban },
	{ Config_Type_Bool, 0, CFG_KEY_STANDALONE, &cfg::standalone },
	{ Config_Type_UInt, 0, CFG_KEY_EPD_REFRESH_MODE, &cfg::epd_refresh_mode },
	{ Config_Type_Bool, 0, CFG_KEY_ANALYTICS_SLEEP_ADD_URBAN, &cfg::analytics_sleep_add_urban },
	{ Config_Type_Bool, 0, CFG_KEY_ANALYTICS_MORNING_AUTOSWITCH, &cfg::analytics_morning_autoswitch },
	{ Config_Type_UInt, 0, CFG_KEY_ANALYTICS_MORNING_END_HOUR, &cfg::analytics_morning_end_hour },
	{ Config_Type_Bool, 0, CFG_KEY_SHARE_TEMPERATURE, &cfg::share_temperature },
	{ Config_Type_Bool, 0, CFG_KEY_SHARE_HUMIDITY, &cfg::share_humidity },
	{ Config_Type_Bool, 0, CFG_KEY_SHARE_PRESSURE, &cfg::share_pressure },
	{ Config_Type_Bool, 0, CFG_KEY_SHARE_CO2, &cfg::share_co2 },
	{ Config_Type_Bool, 0, CFG_KEY_SHARE_PM, &cfg::share_pm },
	{ Config_Type_Bool, 0, CFG_KEY_SHARE_NOISE, &cfg::share_noise },
	{ Config_Type_Bool, 0, CFG_KEY_SHARE_CO, &cfg::share_co },
	{ Config_Type_Bool, 0, CFG_KEY_SHARE_RADIATION, &cfg::share_radiation },
	{ Config_Type_Bool, 0, CFG_KEY_SHARE_O3, &cfg::share_o3 },
	{ Config_Type_Bool, 0, CFG_KEY_SHARE_NO2, &cfg::share_no2 },
	{ Config_Type_Bool, 0, CFG_KEY_SHARE_FAST_AQI, &cfg::share_fast_aqi },
	{ Config_Type_Bool, 0, CFG_KEY_SHARE_EPA_AQI, &cfg::share_epa_aqi },
	{ Config_Type_Bool, 0, CFG_KEY_ENCRYPT_TEMPERATURE, &cfg::encrypt_temperature },
	{ Config_Type_Bool, 0, CFG_KEY_ENCRYPT_HUMIDITY, &cfg::encrypt_humidity },
	{ Config_Type_Bool, 0, CFG_KEY_ENCRYPT_PRESSURE, &cfg::encrypt_pressure },
	{ Config_Type_Bool, 0, CFG_KEY_ENCRYPT_CO2, &cfg::encrypt_co2 },
	{ Config_Type_Bool, 0, CFG_KEY_ENCRYPT_PM, &cfg::encrypt_pm },
	{ Config_Type_Bool, 0, CFG_KEY_ENCRYPT_NOISE, &cfg::encrypt_noise },
	{ Config_Type_Bool, 0, CFG_KEY_ENCRYPT_CO, &cfg::encrypt_co },
	{ Config_Type_Bool, 0, CFG_KEY_ENCRYPT_RADIATION, &cfg::encrypt_radiation },
	{ Config_Type_Bool, 0, CFG_KEY_ENCRYPT_O3, &cfg::encrypt_o3 },
	{ Config_Type_Bool, 0, CFG_KEY_ENCRYPT_NO2, &cfg::encrypt_no2 },
	{ Config_Type_Bool, 0, CFG_KEY_ENCRYPT_FAST_AQI, &cfg::encrypt_fast_aqi },
	{ Config_Type_Bool, 0, CFG_KEY_ENCRYPT_EPA_AQI, &cfg::encrypt_epa_aqi },
};
          
#endif // __CONFIG_H__

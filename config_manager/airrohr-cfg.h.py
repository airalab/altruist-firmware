#!/usr/bin/env python3

configshape_in = """
String		current_lang
String		wlanssid
Password		wlanpwd
Bool        wlannopwd
String		www_username
Password		www_password
String		fs_ssid
Password		fs_pwd
Bool		www_basicauth_enabled
String		rws_owner
String      robonomics_public_node
String		robonomics_connectivity_host
String		robonomics_connectivity_hosts
Bool		rws_auto_register
UInt		rws_group_mode
String		rws_group_id
String		rws_devices_extra
String		rws_devices_registered_hash
String		private_key
String		coords_gps
Bool		send2robonomics
Bool		send2csv
Bool		auto_update
Bool		use_beta
Bool		has_display
Bool		has_sh1106
Bool		has_flipped_display
UInt		debug
Time		sending_intervall_ms
Time		datalog_sending_intervall_ms
Time		sds_meas_interval_ms
Time		time_for_wifi_config
Bool		send2custom
String		host_custom
String		url_custom
UInt		port_custom
String		user_custom
Password		pwd_custom
Bool		ssl_custom
Bool		send2influx
String		host_influx
String		url_influx
UInt		port_influx
String		user_influx
Password		pwd_influx
String		measurement_name_influx
Bool		ssl_influx
String      donated_by
String      current_reg
Bool		region_manual
String      temp_correction
String      local_hostname
String      chosen_altruist_urban
String      timezone
UInt		leds_brightness
Bool        leds_on
UInt        leds_off_hour
UInt        leds_on_hour
UInt        analytics_night_start_hour
UInt        analytics_night_end_hour
String      custom_altruist_urban
Bool		use_custom_urban
Bool		standalone
UInt		epd_refresh_mode
Bool		analytics_sleep_add_urban
Bool		analytics_morning_autoswitch
UInt		analytics_morning_end_hour
Bool		share_temperature
Bool		share_humidity
Bool		share_pressure
Bool		share_co2
Bool		share_pm
Bool		share_noise
Bool		share_co
Bool		share_radiation
Bool		share_o3
Bool		share_no2
Bool		share_fast_aqi
Bool		share_epa_aqi
Bool		encrypt_temperature
Bool		encrypt_humidity
Bool		encrypt_pressure
Bool		encrypt_co2
Bool		encrypt_pm
Bool		encrypt_noise
Bool		encrypt_co
Bool		encrypt_radiation
Bool		encrypt_o3
Bool		encrypt_no2
Bool		encrypt_fast_aqi
Bool		encrypt_epa_aqi
"""

with open("airrohr-cfg.h", "w") as h:
    print("""
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

enum ConfigShapeId {""", file=h)

    for cfgentry in configshape_in.strip().split('\n'):
        print("\tConfig_", cfgentry.split()[1], ",", sep='', file=h)
    print("};", file=h)

    for cfgentry in configshape_in.strip().split('\n'):
        _, cfgkey = cfgentry.split()
        print("static constexpr char CFG_KEY_", cfgkey.upper(),
              "[] PROGMEM = \"", cfgkey, "\";", sep='', file=h)

    print("static constexpr ConfigShapeEntry configShape[] PROGMEM = {",
          file=h)
    for cfgentry in configshape_in.strip().split('\n'):
        cfgtype, cfgkey = cfgentry.split()
        print("\t{ Config_Type_", cfgtype,
              ", sizeof(cfg::" + cfgkey + ")-1" if cfgtype in ('String', 'Password') else ", 0",
              ", CFG_KEY_", cfgkey.upper(),
              ", ", "" if cfgtype in ('String', 'Password') else "&",
              "cfg::", cfgkey, " },", sep='', file=h)
    print("""};
          
#endif // __CONFIG_H__""", file=h)

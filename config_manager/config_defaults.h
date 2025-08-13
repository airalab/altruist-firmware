#ifndef __CONFIG_DEFAULTS_H__
#define __CONFIG_DEFAULTS_H__

#include "../defines.h"

namespace cfg {
	extern unsigned debug;

	extern unsigned time_for_wifi_config;
	extern unsigned sending_intervall_ms;
	extern unsigned sds_meas_interval_ms;
	extern unsigned datalog_sending_intervall_ms;

	extern char current_lang[3];
	extern char current_reg[20];
	// credentials for basic auth of internal web server
	extern bool www_basicauth_enabled;
	extern char www_username[LEN_WWW_USERNAME];
	extern char www_password[LEN_CFG_PASSWORD];

	// wifi credentials
	extern char wlanssid[LEN_WLANSSID];
	extern char wlanpwd[LEN_CFG_PASSWORD];
	extern bool wlannopwd;

	// credentials of the sensor in access point mode
	extern char fs_ssid[LEN_FS_SSID];
	extern char fs_pwd[LEN_CFG_PASSWORD];

	extern char rws_owner[LEN_RWS_OWNER];
	extern char robonomics_public_node[LEN_ROBONOMICS_PUBLIC_NODE];
	extern char private_key[LEN_PRIVATE_KEY];
	extern char coords_gps[LEN_GPS_COORDS];

	// send to "APIs"
	extern bool send2robonomics;
	extern bool send2custom;
	extern bool send2influx;
	extern bool send2csv;

	extern bool auto_update;
	extern bool use_beta;

	// (in)active displays
	extern bool has_display;
	extern bool has_sh1106;
	extern bool has_flipped_display;

	extern char host_influx[LEN_HOST_INFLUX];
	extern char url_influx[LEN_URL_INFLUX];
	extern unsigned port_influx;
	extern char user_influx[LEN_USER_INFLUX];
	extern char pwd_influx[LEN_CFG_PASSWORD];
	extern char measurement_name_influx[LEN_MEASUREMENT_NAME_INFLUX];
	extern bool ssl_influx;

	extern char host_custom[LEN_HOST_CUSTOM];
	extern char url_custom[LEN_URL_CUSTOM];
	extern bool ssl_custom;
	extern unsigned port_custom;
	extern char user_custom[LEN_USER_CUSTOM];
	extern char pwd_custom[LEN_CFG_PASSWORD];
	extern char donated_by[LEN_DONATED_BY];

	extern char temp_correction[LEN_TEMP_CORRECTION];
	extern char local_hostname[LEN_LOCAL_HOSTNAME];
	extern char chosen_altruist_urban[LEN_CHOSEN_ALTRUIS_ADDRESS];
	extern char timezone[LEN_TIMEZONE];

	extern unsigned leds_brightness;
	extern bool leds_on;

	extern void initNonTrivials(const char* id);
}

#endif // __CONFIG_DEFAULTS_H__

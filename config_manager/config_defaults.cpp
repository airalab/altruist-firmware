#include <Arduino.h>
#include <cstdio>
#include <cstring>
#include "config_defaults.h"

namespace cfg {
	unsigned debug = ALTRUIST_DEFAULT_LOG_LEVEL;

	unsigned time_for_wifi_config = 600000;
	unsigned sending_intervall_ms = 30000;
	unsigned sds_meas_interval_ms = 300000;
	unsigned datalog_sending_intervall_ms = 600000;

	char current_lang[3];
	char current_reg[20];
	bool region_manual = false;

	// credentials for basic auth of internal web server
	bool www_basicauth_enabled = WWW_BASICAUTH_ENABLED;
	char www_username[LEN_WWW_USERNAME];
	char www_password[LEN_CFG_PASSWORD];

	// wifi credentials
	char wlanssid[LEN_WLANSSID];
	char wlanpwd[LEN_CFG_PASSWORD];
	bool wlannopwd = WLANNOPWD;

	// credentials of the sensor in access point mode
	char fs_ssid[LEN_FS_SSID] = FS_SSID;
	char fs_pwd[LEN_CFG_PASSWORD] = FS_PWD;

	char rws_owner[LEN_RWS_OWNER] = "Not Set";
	char robonomics_public_node[LEN_ROBONOMICS_PUBLIC_NODE] = ROBONOMICS_PUBLIC_NODE;
	char robonomics_connectivity_host[LEN_ROBONOMICS_CONNECTIVITY_HOST] = "";
	char robonomics_connectivity_hosts[LEN_ROBONOMICS_CONNECTIVITY_HOSTS] = "";
	bool rws_auto_register = true;
	unsigned rws_group_mode = RWS_GROUP_STANDALONE;
	char rws_group_id[LEN_RWS_GROUP_ID] = "";
	char rws_devices_extra[LEN_RWS_DEVICES_EXTRA] = "";
	char rws_devices_registered_hash[LEN_RWS_DEVICES_REGISTERED_HASH] = "";
	char private_key[LEN_PRIVATE_KEY] = "Not Set";
	char coords_gps[LEN_GPS_COORDS] = GPS_COORDS;


	// send to "APIs"
	bool send2robonomics = SEND2ROBONOMICS;
	bool send2custom = SEND2CUSTOM;
	bool send2influx = SEND2INFLUX;
	bool send2csv = SEND2CSV;

	bool auto_update = AUTO_UPDATE;
	bool use_beta = USE_BETA;  // Retained so existing config files remain readable.

	// (in)active displays
	bool has_display = HAS_DISPLAY;											// OLED with SSD1306 and I2C
	bool has_sh1106 = HAS_SH1106;
	bool has_flipped_display = HAS_FLIPPED_DISPLAY;

	char host_influx[LEN_HOST_INFLUX];
	char url_influx[LEN_URL_INFLUX];
	unsigned port_influx = PORT_INFLUX;
	char user_influx[LEN_USER_INFLUX] = USER_INFLUX;
	char pwd_influx[LEN_CFG_PASSWORD] = PWD_INFLUX;
	char measurement_name_influx[LEN_MEASUREMENT_NAME_INFLUX];
	bool ssl_influx = SSL_INFLUX;

	char host_custom[LEN_HOST_CUSTOM];
	char url_custom[LEN_URL_CUSTOM];
	bool ssl_custom = SSL_CUSTOM;
	unsigned port_custom = PORT_CUSTOM;
	char user_custom[LEN_USER_CUSTOM] = USER_CUSTOM;
	char pwd_custom[LEN_CFG_PASSWORD] = PWD_CUSTOM;
	char donated_by[LEN_DONATED_BY];

	char temp_correction[LEN_TEMP_CORRECTION] = TEMP_CORRECTION;
	char local_hostname[LEN_LOCAL_HOSTNAME] = "";
	char chosen_altruist_urban[LEN_CHOSEN_ALTRUIS_ADDRESS] = "";
	char timezone[LEN_TIMEZONE] = "<+00>0";
	char custom_altruist_urban[LEN_CHOSEN_ALTRUIS_ADDRESS] = "";
	bool use_custom_urban = false;
	bool standalone = false;
	unsigned epd_refresh_mode = EPD_REFRESH_SAFE;
	bool analytics_sleep_add_urban = false;

	unsigned leds_brightness = 100; // Default 100% = 30% actual brightness (scaled down)
	bool leds_on = true;
	unsigned leds_off_hour = 0; // Default off at 00:00 (local time)
	unsigned leds_on_hour = 6;  // Default on at 06:00 (local time)
	unsigned analytics_night_start_hour = 22 * 60; // minutes from midnight (key name legacy)
	unsigned analytics_night_end_hour = 7 * 60;    // exclusive end, minutes (07:00)
	bool analytics_morning_autoswitch = true;
	unsigned analytics_morning_end_hour = 12 * 60; // exclusive end (12:00 local)

	// data sharing preferences (all shared by default)
	bool share_temperature = true;
	bool share_humidity = true;
	bool share_pressure = true;
	bool share_co2 = true;
	bool share_pm = true;
	bool share_noise = true;
	// Additional (optional) sensors: default OFF (most devices don't have them)
	bool share_co = false;
	bool share_radiation = false;
	bool share_o3 = false;
	bool share_no2 = false;
	bool share_fast_aqi = false;
	bool share_epa_aqi = false;

	bool encrypt_temperature = false;
	bool encrypt_humidity = false;
	bool encrypt_pressure = false;
	bool encrypt_co2 = false;
	bool encrypt_pm = false;
	bool encrypt_noise = false;
	bool encrypt_co = false;
	bool encrypt_radiation = false;
	bool encrypt_o3 = false;
	bool encrypt_no2 = false;
	bool encrypt_fast_aqi = false;
	bool encrypt_epa_aqi = false;

	void formatDefaultLocalHostname(char* out, size_t out_len, const char* chip_id) {
		if (!out || out_len == 0) {
			return;
		}
		char short_id[5] = {'0', '0', '0', '0', '\0'};
		if (chip_id && chip_id[0] != '\0') {
			const size_t n = strlen(chip_id);
			const char* src = (n > 4) ? (chip_id + (n - 4)) : chip_id;
			for (size_t i = 0; i < 4 && src[i] != '\0'; ++i) {
				char c = src[i];
				if (c >= 'A' && c <= 'Z') {
					c = static_cast<char>(c - 'A' + 'a');
				}
				short_id[i] = c;
			}
		}
		snprintf(out, out_len, "altruist-%s-%s", DEVICE_MODEL, short_id);
	}

	void initNonTrivials(const char* id) {
		strcpy(cfg::current_lang, CURRENT_LANG);
		strcpy(cfg::current_reg, CURRENT_REG);
		strcpy_P(www_username, WWW_USERNAME);
		strcpy_P(www_password, WWW_PASSWORD);
		strcpy_P(wlanssid, WLANSSID);
		strcpy_P(wlanpwd, WLANPWD);
		strcpy_P(host_custom, HOST_CUSTOM);
		strcpy_P(url_custom, URL_CUSTOM);
		strcpy_P(host_influx, HOST_INFLUX);
		strcpy_P(url_influx, URL_INFLUX);
		strcpy_P(measurement_name_influx, MEASUREMENT_NAME_INFLUX);
		strcpy_P(donated_by, DONATED_BY);

		if (!*fs_ssid) {
			snprintf(fs_ssid, sizeof(fs_ssid), "Altruist-%s-%s", DEVICE_MODEL, id);
		}
		if (!*local_hostname ||
		    strcmp(local_hostname, "altruist") == 0 ||
		    strcmp(local_hostname, LOCAL_HOSTNAME) == 0) {
			formatDefaultLocalHostname(local_hostname, sizeof(local_hostname), id);
		}
	}
}

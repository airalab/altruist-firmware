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
	/** True after user changes Region in UI; blocks OTA auto-region. */
	extern bool region_manual;
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
	extern char robonomics_connectivity_host[LEN_ROBONOMICS_CONNECTIVITY_HOST];
	extern char robonomics_connectivity_hosts[LEN_ROBONOMICS_CONNECTIVITY_HOSTS];
	extern char robonomics_proto_connectivity_host[LEN_ROBONOMICS_CONNECTIVITY_HOST];
	extern bool map_send_csv;
	extern bool map_send_proto;
	extern bool rws_auto_register;
	extern unsigned rws_group_mode;
	extern char rws_group_id[LEN_RWS_GROUP_ID];
	extern char rws_devices_extra[LEN_RWS_DEVICES_EXTRA];
	extern char rws_devices_registered_hash[LEN_RWS_DEVICES_REGISTERED_HASH];
	extern char private_key[LEN_PRIVATE_KEY];
	extern char coords_gps[LEN_GPS_COORDS];

	// send to "APIs"
	extern bool send2robonomics;
	extern bool send2custom;
	extern bool send2influx;
	extern bool send2csv;
	/** Urban C6: compact JSON Lines UART on CN4, TX GPIO22 / RX GPIO20, 115200 baud. */
	extern bool lora_uart_enabled;

	extern bool auto_update;
	extern bool use_beta;  // Legacy persisted key; ignored by channel-aware OTA.

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

	extern char custom_altruist_urban[LEN_CHOSEN_ALTRUIS_ADDRESS];
	extern bool use_custom_urban;
	/** When true (Insight), do not register the HTTP Urban client sensor. */
	extern bool standalone;
	/** Insight e-paper: 0 = safe full refresh only, 1 = experimental partial refresh. */
	extern unsigned epd_refresh_mode;
	/** Paired mode: add Urban PM2.5 & noise to sleep analytics hourly history (default off = Insight PM2.5 only). */
	extern bool analytics_sleep_add_urban;

	extern unsigned leds_brightness;
	extern bool leds_on;
	extern unsigned leds_off_hour;
	extern unsigned leds_on_hour;
	/** Sleep analytics window: minutes from midnight [0..1439] (local). NVS keys stay *_hour for compatibility; values <=23 are treated as legacy whole hours. */
	extern unsigned analytics_night_start_hour;
	extern unsigned analytics_night_end_hour;
	extern bool analytics_morning_autoswitch;
	extern unsigned analytics_morning_end_hour;

	// data sharing preferences
	extern bool share_temperature;
	extern bool share_humidity;
	extern bool share_pressure;
	extern bool share_co2;
	extern bool share_pm;
	extern bool share_noise;
	extern bool share_co;
	extern bool share_radiation;
	extern bool share_o3;
	extern bool share_no2;
	extern bool share_fast_aqi;
	extern bool share_epa_aqi;

	// optional per-metric encryption for public feed (default off)
	extern bool encrypt_temperature;
	extern bool encrypt_humidity;
	extern bool encrypt_pressure;
	extern bool encrypt_co2;
	extern bool encrypt_pm;
	extern bool encrypt_noise;
	extern bool encrypt_co;
	extern bool encrypt_radiation;
	extern bool encrypt_o3;
	extern bool encrypt_no2;
	extern bool encrypt_fast_aqi;
	extern bool encrypt_epa_aqi;

	/** Default LAN/mDNS name: altruist-<model>-<last4 chip id>, e.g. altruist-insight-a1b2 */
	void formatDefaultLocalHostname(char* out, size_t out_len, const char* chip_id);
	extern void initNonTrivials(const char* id);
}

#endif // __CONFIG_DEFAULTS_H__

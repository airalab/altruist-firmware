#ifndef __DEFINES_H__
#define __DEFINES_H__

#include <stdint.h>

// increment on change
#if defined(ALTRUIST_INSIDE)
#define SOFTWARE_VERSION_BASE "R-INS_2026-05.2"
#define PM_SENSOR_NAME "Altruist Insight"
#endif
#if defined(ALTRUIST_URBAN)
#define SOFTWARE_VERSION_BASE "R-URB_2026-05.2"
#define PM_SENSOR_NAME "Altruist Urban"
#endif
#if defined(ALTRUIST_FIRMWARE_DEV)
#define SOFTWARE_VERSION_STR SOFTWARE_VERSION_BASE "_dev"
#else
#define SOFTWARE_VERSION_STR SOFTWARE_VERSION_BASE
#endif

#if defined(ESP8266)
#define SENSOR_BASENAME "esp8266-"
#define OTA_BASENAME "/airrohr"
#endif
#if defined(ESP32)
#define SENSOR_BASENAME "esp32-"
#define OTA_BASENAME "/airrohr/esp32"
#endif

#define ATRUIST_URBAN_SENSOR "altruist_urban"

#define DEVICE_MODEL_MDNS_PROPERTY "device_model"
#define DEVICE_MODEL_INSIGHT "insight"
#define DEVICE_MODEL_URBAN "urban"
#if defined(ALTRUIST_INSIDE)
#define DEVICE_MODEL DEVICE_MODEL_INSIGHT
#endif
#if defined(ALTRUIST_URBAN)
#define DEVICE_MODEL DEVICE_MODEL_URBAN
#endif

/** ESP32-C6 Urban: NeoPixel ring + reset button. Not defined on ESP32-C3 Urban (no LED/button HW). */
#if defined(ALTRUIST_URBAN) && defined(ALTRUIST_URBAN_HW_UI)
#define ALTRUIST_URBAN_HAS_LED_BTN 1
#endif

/** ESP32-C3 Urban: smaller flash image (lite web UI, core sensors only). Set in platformio env. */
#if defined(ALTRUIST_URBAN_C3_LITE)
#define ALTRUIST_URBAN_MINIMAL_SENSORS 1
#endif
/** C3 Urban: no mDNS library — open device by IP. */
#if defined(ALTRUIST_URBAN_C3_NO_MDNS)
#define ALTRUIST_USE_MDNS 0
#else
#define ALTRUIST_USE_MDNS 1
#endif


#define SSID_BASENAME "Altruist-"
#define HOSTNAME_BASE "Altruist-"

#define LEN_CFG_STRING 65
#define LEN_CFG_PASSWORD 65

#define LEN_WLANSSID 35				// credentials for wifi connection

#define LEN_WWW_USERNAME 65			// credentials for basic auth of server internal website

#define LEN_FS_SSID 33				// credentials for sensor access point mode

#define LEN_RWS_OWNER 70
#define LEN_ROBONOMICS_PUBLIC_NODE 70
#define LEN_PRIVATE_KEY 65
#define LEN_GPS_LAT 10
#define LEN_GPS_LON 10
#define LEN_GPS_COORDS 21
#define LEN_DNMS_CORRECTION 8
#define LEN_TEMP_CORRECTION 8
#define LEN_MIC_CORRECTION 8
#define LEN_LOCAL_HOSTNAME 100
#define LEN_CHOSEN_ALTRUIS_ADDRESS 20
#define LEN_TIMEZONE 10

#define LEN_HOST_INFLUX 100
#define LEN_URL_INFLUX 100
#define LEN_USER_INFLUX 65
#define LEN_MEASUREMENT_NAME_INFLUX 100

#define LEN_HOST_CUSTOM 100
#define LEN_URL_CUSTOM 100
#define LEN_USER_CUSTOM 65
#define MAX_PORT_DIGITS 5

#define LEN_DONATED_BY 100

// define debug levels
#define DEBUG_ERROR 1
#define DEBUG_WARNING 2
#define DEBUG_MIN_INFO 3
#define DEBUG_MED_INFO 4
#define DEBUG_MAX_INFO 5

/******************************************************************
 * Constants                                                      *
 ******************************************************************/
constexpr const unsigned long SAMPLETIME_MS = 30000;									// time between two measurements of the PPD42NS
// constexpr const unsigned long SAMPLETIME_SDS_MS = 1000;								// time between two measurements of the SDS011, PMSx003, Honeywell PM sensor
// constexpr const unsigned long WARMUPTIME_SDS_MS = 15000;								// time needed to "warm up" the sensor before we can take the first measurement
// constexpr const unsigned long READINGTIME_SDS_MS = 5000;								// how long we read data from the PM sensors
constexpr const unsigned long SAMPLETIME_NPM_MS = 1000;
constexpr const unsigned long WARMUPTIME_NPM_MS = 15000;
constexpr const unsigned long READINGTIME_NPM_MS = 15000;                // how long we read data from the PM sensors
constexpr const unsigned long SAMPLETIME_GPS_MS = 50;
constexpr const unsigned long SAMPLETIME_DBMETER_MS = 1000;	
constexpr const unsigned long DISPLAY_UPDATE_INTERVAL_MS = 5000;						// time between switching display to next "screen"
constexpr const unsigned long ONE_DAY_IN_MS = 24 * 60 * 60 * 1000;
constexpr const unsigned long PAUSE_BETWEEN_UPDATE_ATTEMPTS_MS = ONE_DAY_IN_MS;		// check for firmware updates once a day
constexpr const unsigned long DURATION_BEFORE_FORCED_RESTART_MS = ONE_DAY_IN_MS * 28;	// force a reboot every ~4 weeks

// STA recovery when home Wi‑Fi is configured but link drops (Urban/Insight).
constexpr const unsigned long WIFI_STA_PERIODIC_RECONNECT_MS = 18000UL;
/**
 * After we initiate a reconnect attempt, give the stack time to associate / run DHCP.
 */
constexpr const unsigned long WIFI_STA_RECOVERY_GRACE_MS = 45000UL;
/** Urban captive portal: keep setup AP up after success so the phone can read/copy STA IP. */
constexpr const unsigned long GUEST_SUCCESS_PAGE_DELAY_MS = 15000UL;
/** After association, STA can report WL_CONNECTED before IPv4; do not tear down the link during this window. */
constexpr const unsigned long WIFI_STA_DHCP_GRACE_MS = 40000UL;
/** After this long without a usable STA link, use radio off/on (WIFI_OFF) before re-begin — stronger than disconnect() alone, no MCU reboot. */
constexpr const unsigned long WIFI_STA_DEEP_RECOVER_AFTER_MS = 180000UL;
/** Minimum gap between WIFI_OFF deep recoveries (avoids IDF un-init timeouts if hammered). */
constexpr const unsigned long WIFI_STA_DEEP_RADIO_MIN_INTERVAL_MS = 90000UL;
/**
 * If deep recovery is wanted but throttled, force WIFI_OFF after this many stalled attempts
 * (~one per WIFI_STA_PERIODIC_RECONNECT_MS) so the link can return without an MCU reboot.
 */
constexpr const uint8_t WIFI_STA_DEEP_FORCE_AFTER_THROTTLED_SKIPS = 5;
/**
 * Last-resort MCU reboot if STA link never returns (rare stuck WiFi stack).
 * 0 = never reboot on STA loss (soft recovery only). 30 min = long failsafe without the old ~10 min surprise reboot.
 */
constexpr const unsigned long WIFI_STA_REBOOT_AFTER_MS = (30UL * 60UL * 1000UL);

/** On-chain datalog send must finish within this window; otherwise reboot (stuck Robonomics client). */
constexpr const unsigned long DATALOG_SEND_WATCHDOG_MS = 90UL * 1000UL;
/** loop() must complete within this window (web + Insight display); otherwise reboot. */
constexpr const unsigned long MAIN_LOOP_STALL_WATCHDOG_MS = 90UL * 1000UL;
/**
 * sensorAndAPIWorker must finish one full iteration within this window (fetch + APIs + OTA check).
 * Catches Map HTTP / fetch / datalog hangs even when loop() still runs (Insight frozen UI).
 */
constexpr const unsigned long SENSOR_WORKER_LOOP_WATCHDOG_MS = 120UL * 1000UL;

// ------------------------------------------------------------
// Urban TTL constants (Insight only)
// ------------------------------------------------------------
constexpr const uint32_t URBAN_STALE_AFTER_MS   = 10UL * 60UL * 1000UL;   // ~1 fetch interval + buffer
constexpr const uint32_t URBAN_OFFLINE_AFTER_MS = 20UL * 60UL * 1000UL;  // ~2 fetch intervals
/** Consecutive UI/LED evaluation cycles in the stale window before showing stale (reduces flicker). */
constexpr const uint8_t URBAN_STALE_CONFIRMATIONS_REQUIRED = 3;
constexpr const unsigned long URBAN_REDISCOVER_INTERVAL_MS = 5UL * 60UL * 1000UL;

// Pins Config

#if defined(CONFIG_IDF_TARGET_ESP32C3)

// i2s pins

#define I2S_PIN_BCLK     7
#define I2S_PIN_WS       6
#define I2S_PIN_DIN      8
#define I2S_PIN_DOUT     -1

// I2C pins

#define SDA_I2C_PIN 3
#define SCL_I2C_PIN 0

// PM Serial

#define PM_SERIAL_RX 1
#define PM_SERIAL_TX 10

// SPI pins

#define SPI_SCK_PIN 7
#define SPI_MISO_PIN 18
#define SPI_MOSI_PIN 6
#define SPI_CS_PIN 19

// Display

#define EPD_SCK_PIN  -1
#define EPD_MOSI_PIN -1
#define EPD_CS_PIN   -1
#define EPD_RST_PIN  -1
#define EPD_DC_PIN   -1
#define EPD_BUSY_PIN -1

// Buttons

#define BTN_DOWN_PIN -1
#define BTN_SET_PIN -1
#define BTN_UP_PIN -1

// Urban reset button pin (ESP32-C3 boards).
// Most ESP32-C3 variants of this project don't have this button populated,
// so keep it disabled by default to avoid accidental GPIO conflicts.
#ifndef URBAN_RESET_BTN_PIN
#define URBAN_RESET_BTN_PIN -1
#endif

// Led pin

#define LED_PIN -1

#elif defined(CONFIG_IDF_TARGET_ESP32C6)

// i2s pins
#ifdef ALTRUIST_URBAN
#define I2S_PIN_BCLK     10
#define I2S_PIN_WS       1
#define I2S_PIN_DIN      11
#define I2S_PIN_DOUT     -1
#endif
#ifdef ALTRUIST_INSIDE
#define I2S_PIN_BCLK     -1
#define I2S_PIN_WS       -1
#define I2S_PIN_DIN      -1
#define I2S_PIN_DOUT     -1
#endif

// I2C pins

#ifdef ALTRUIST_INSIDE
// #define SDA_I2C_PIN 19
// #define SCL_I2C_PIN 18
#ifdef PRE
#define SDA_I2C_PIN 2
#define SCL_I2C_PIN 3
#else
#define SDA_I2C_PIN 19
#define SCL_I2C_PIN 18
#endif //PRE
#endif //ALTRUIST_INSIDE
#ifdef ALTRUIST_URBAN
#define SDA_I2C_PIN 3
#define SCL_I2C_PIN 2
#endif

// PM Serial

#ifdef ALTRUIST_INSIDE
#define PM_SERIAL_RX -1
#define PM_SERIAL_TX -1
#endif
#ifdef ALTRUIST_URBAN
#define PM_SERIAL_RX 5
#define PM_SERIAL_TX 4
#endif


// SPI SD Card pins

#ifdef ALTRUIST_INSIDE
#ifdef PRE
#define SPI_SCK_PIN 5
#define SPI_MISO_PIN 18
#define SPI_MOSI_PIN 6
#define SPI_CS_PIN 19
#else
#define SPI_SCK_PIN 0
#define SPI_MISO_PIN 1
#define SPI_MOSI_PIN 7
#define SPI_CS_PIN 6
#endif //PRE
#endif

// Display

#ifdef ALTRUIST_INSIDE
#ifdef PRE
#define EPD_SCK_PIN  21
#define EPD_MOSI_PIN 20
#define EPD_CS_PIN   22
#define EPD_RST_PIN  15
#define EPD_DC_PIN   23
#define EPD_BUSY_PIN 7
#else
#define EPD_SCK_PIN  21
#define EPD_MOSI_PIN 20
#define EPD_CS_PIN   22
#define EPD_RST_PIN  15
#define EPD_DC_PIN   23
#define EPD_BUSY_PIN 4
#endif //PRE
#endif

// Buttons

#ifdef ALTRUIST_INSIDE
#ifdef PRE
#define BTN_DOWN_PIN 0
#define BTN_SET_PIN 1
#define BTN_UP_PIN 10
#else
#define BTN_DOWN_PIN 3
#define BTN_SET_PIN 2
#define BTN_UP_PIN 10
#endif //PRE
#endif
#ifdef ALTRUIST_URBAN
#define BTN_DOWN_PIN -1
// #define BTN_SET_PIN 7
#define BTN_SET_PIN -1
#define BTN_UP_PIN -1
// Urban reset button pin.
// Safe for OTA across mixed hardware: if the button is not populated, the pin
// stays pulled-up and the long-press condition never triggers.
// Can be overridden via build flags (e.g. disable on rare legacy boards):
//   -DURBAN_RESET_BTN_PIN=-1
#ifndef URBAN_RESET_BTN_PIN
#define URBAN_RESET_BTN_PIN 7
#endif
#endif

// Led pin

#ifdef ALTRUIST_INSIDE
// #define LED_PIN -1
#define LED_PIN 11
#endif
#ifdef ALTRUIST_URBAN
#define LED_PIN 0
#endif

#else
  #error Unsupported board selection.
#endif 


// TRANSFER FROM ext_def.h
// Language config
#define CURRENT_LANG INTL_LANG

// Wifi config
const char WLANSSID[] PROGMEM = "Not Set";
const char WLANPWD[] PROGMEM = "";
#define LOCAL_HOSTNAME "altruist"
#define WLANNOPWD 0

// BasicAuth config
const char WWW_USERNAME[] PROGMEM = "admin";
const char WWW_PASSWORD[] PROGMEM = "";
#define WWW_BASICAUTH_ENABLED 0

// Sensor Wifi config (config mode)
#define FS_SSID ""
#define FS_PWD "123456789"

// Where to send the data?
#define SEND2ROBONOMICS 1
#define SSL_ROBONOMICS 0
#define SSL_FSAPP 0
#define SEND2MQTT 0
#define SEND2INFLUX 0
#define SEND2LORA 0
#define SEND2CSV 0
#define SEND2CUSTOM 0

enum LoggerEntry {
    LoggerRobonomics,
    LoggerFSapp,
    LoggerInflux,
    LoggerCustom,
    LoggerCount
};

struct LoggerConfig {
    uint16_t destport;
    uint16_t errors;
#if defined(ESP8266)
    BearSSL::Session* session;
#else
    void* session;
#endif
};

// IMPORTANT: NO MORE CHANGES TO VARIABLE NAMES NEEDED FOR EXTERNAL APIS

static const char HOST_FSAPP[] PROGMEM = "server.chillibits.com";
static const char URL_FSAPP[] PROGMEM = "/data.php";
#define PORT_FSAPP 80

static const char FW_DOWNLOAD_HOST[] PROGMEM = "upd.sensors.robonomics.network";
static const char FW_DOWNLOAD_HOST_ALTERNATIVE[] PROGMEM = "updru.sensors.robonomics.network";
#define FW_DOWNLOAD_PORT 80

static const char FW_2ND_LOADER_URL[] PROGMEM = "/loader-002.bin";

static const char NTP_SERVER_1[] PROGMEM = "0.pool.ntp.org";
static const char NTP_SERVER_2[] PROGMEM = "1.pool.ntp.org";

// define own API
static const char HOST_CUSTOM[] PROGMEM = "192.168.100.73";
static const char URL_CUSTOM[] PROGMEM = "";
#define PORT_CUSTOM 5000
#define USER_CUSTOM ""
#define PWD_CUSTOM ""
#define SSL_CUSTOM 0


// Robonomics
#include "./intl.h"
static const char CURRENT_REG[] PROGMEM = "Global";
// #define PORT_ROBONOMICS 31112
#define PORT_ROBONOMICS 65
#define ROBONOMICS_PUBLIC_NODE "polkadot.rpc.robonomics.network"

// Robonomics Map (connectivity) host override / pool.
// - `robonomics_connectivity_host`: pinned single host (optional)
// - `robonomics_connectivity_hosts`: list/pool of hosts to try (optional)
#define LEN_ROBONOMICS_CONNECTIVITY_HOST 80
#define LEN_ROBONOMICS_CONNECTIVITY_HOSTS 240

// Donated by
static const char DONATED_BY[] PROGMEM = "";

// define own InfluxDB
static const char HOST_INFLUX[] PROGMEM = "influx.server";
static const char URL_INFLUX[] PROGMEM = "/write?db=sensorcommunity";
#define PORT_INFLUX 8086
#define USER_INFLUX ""
#define PWD_INFLUX ""
static const char MEASUREMENT_NAME_INFLUX[] PROGMEM = "feinstaub";
#define SSL_INFLUX 0

// GPS, preferred Neo-6M
#define GPS_READ 1
#define GPS_API_PIN 9
#define GPS_LAT "0.0"
#define GPS_LON "0.0"
#define GPS_COORDS "0.0,0.0"

// Temp compensation
#define TEMP_CORRECTION "0.0"

// Microphone compensation
#define MIC_CORRECTION "1.5"

// MHZ19 CO2 sensor
#define MHZ19_READ 0

// automatic firmware updates
// Production builds: auto-update on
// DEV builds: auto-update off
#ifdef DEV
	#define AUTO_UPDATE 0
#else
	#define AUTO_UPDATE 1
#endif

// use beta firmware
#define USE_BETA 0

// OLED Display SSD1306 connected?
#define HAS_DISPLAY 0

// OLED Display SH1106 connected?
#define HAS_SH1106 0

// OLED Display um 180° gedreht?
#define HAS_FLIPPED_DISPLAY 0

// LCD Display LCD1602 connected?
#define HAS_LCD1602 0

// LCD Display LCD1602 (0x27) connected?
#define HAS_LCD1602_27 0

// LCD Display LCD2004 connected?
#define HAS_LCD2004 0

// LCD Display LCD2004 (0x27) connected?
#define HAS_LCD2004_27 0

// Show wifi info on displays
#define DISPLAY_WIFI_INFO 1

// Show device info on displays
#define DISPLAY_DEVICE_INFO 1

// Set debug level for serial output
#ifndef DEBUG
#define DEBUG 3
#endif

// Insight standalone main: 1 = force all four footer warnings (Hum/Temp/Press/CO2) for layout preview.
// Default 0. Set to 1 while tuning UI, or build with: -DINSIGHT_DEBUG_ALL_WARNINGS=1
#ifndef INSIGHT_DEBUG_ALL_WARNINGS
#define INSIGHT_DEBUG_ALL_WARNINGS 0
#endif

#endif // __DEFINES_H__

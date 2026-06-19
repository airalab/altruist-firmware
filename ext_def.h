#ifndef __EXT_DEF_H__
#define __EXT_DEF_H__


// DHT22, temperature, humidity
#define DHT_READ 0
#define DHT_TYPE DHT22
#define DHT_API_PIN 7

// HTU21D, temperature, humidity
#define HTU21D_READ 0
#define HTU21D_API_PIN 7

// PPD42NS, the cheaper version of the particle sensor
#define PPD_READ 0
#define PPD_API_PIN 5

// Geiger Counter
#define GC_READ 0

// Noise Meters
#define DBMETER_READ 0
#define I2SNOISE_READ 1

// SDS011, the more expensive version of the particle sensor
#define SDS_READ 1
#define SDS_API_PIN 1

// PMS1003, PMS300, 3PMS5003, PMS6003, PMS7003
#define PMS_READ 0
#define PMS_API_PIN 1

// CCS811 
#define CCS811_READ 0
#define CCS811_27_READ 0

// Honeywell PM sensor
#define HPM_READ 0
#define HPM_API_PIN 1

// Tera Sensor Next PM sensor
#define NPM_READ 0
#define NPM_API_PIN 1

// Sensirion SPS30, the more expensive version of the particle sensor
#define SPS30_READ 0
#define SPS30_API_PIN 1
#define SPS30_WAITING_AFTER_LAST_READ 11000   // waiting time after last reading mesurement data in ms
#define SPS30_AUTO_CLEANING_INTERVAL 7200 // time in seconds

// BMP180, temperature, pressure
#define BMP_READ 0
#define BMP_API_PIN 3

// BMP280/BME280, temperature, pressure (humidity on BME280)
#define BMX280_READ 0
#define BMP280_API_PIN 3
#define BME280_API_PIN 11

// SHT3x, temperature, pressure
#define SHT3X_READ 0
#define SHT3X_API_PIN 7

// DS18B20, temperature
#define DS18B20_READ 0
#define DS18B20_API_PIN 13

// DNMS Noise Measurement
#define DNMS_READ 0
#define DNMS_API_PIN 15
#define DNMS_CORRECTION "0.0"
/*

// Temp compensation
#define TEMP_CORRECTION "0.0"

// Microphone compensation
#define MIC_CORRECTION "1.5"

// GPS, preferred Neo-6M
#define GPS_READ 1
#define GPS_API_PIN 9
#define GPS_LAT "0.0"
#define GPS_LON "0.0"
#define GPS_COORDS "0.0,0.0"

// MHZ19 CO2 sensor
#define MHZ19_READ 0

// automatic firmware updates
#define AUTO_UPDATE 1

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

// Set debug level for serial output?
#define DEBUG 3
*/

#endif // __EXT_DEF_H__

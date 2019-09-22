/*
 *	airRohr firmware
 *	Copyright (C) 2016-2018  Code for Stuttgart a.o.
 * 
 *  English translations
 * 
 *	Texts should be as short as possible
 *	We only have 512 kB for the entire firmware
 */

const char INTL_LANG[] = "EN";
const char INTL_PM_SENSOR[] PROGMEM = "Particulate matter sensor";
const char INTL_CONFIGURATION[] PROGMEM = "Configuration";
const char INTL_WIFI_SETTINGS[] PROGMEM = "WiFi Settings";
const char INTL_WIFI_NETWORKS[] PROGMEM = "Loading wifi networks ...";
const char INTL_LANGUAGE[] PROGMEM = "Language";
const char INTL_NO_NETWORKS[] PROGMEM =  "No WiFi Network Found";
const char INTL_NETWORKS_FOUND[] PROGMEM = "Found Networks: ";
const char INTL_AB_HIER_NUR_ANDERN[] PROGMEM = "Advanced settings (оnly if you know what you are doing)";
const char INTL_SAVE[] PROGMEM = "Save";
const char INTL_SENSORS[] PROGMEM = "Sensors";
const char INTL_SDS011[] PROGMEM = "SDS011 ({pm})";
const char INTL_PMS[] PROGMEM = "Plantower PMS(1,3,5,6,7)003 ({pm})";
const char INTL_HPM[] PROGMEM = "Honeywell PM sensor";
const char INTL_SPS30[] PROGMEM = "Sensirion SPS30 ({pm})";
const char INTL_PPD42NS[] PROGMEM = "PPD42NS";
const char INTL_DHT22[] PROGMEM = "DHT22 ({t}, {h})";
const char INTL_HTU21D[] PROGMEM = "HTU21D ({t}, {h})";
const char INTL_BMP180[] PROGMEM = "BMP180 ({t}, {p})";
const char INTL_BMX280[] PROGMEM = "BMP280/BME280 ({t}, {h}, {p})";
const char INTL_DS18B20[] PROGMEM = "DS18B20 ({t})";
const char INTL_DNMS[] PROGMEM = "DNMS ({l_a})";
const char INTL_DNMS_CORRECTION[] PROGMEM ="correction in dB(A)";
const char INTL_NEO6M[] PROGMEM = "GPS (NEO 6M)";
const char INTL_BASICAUTH[] PROGMEM = "Authorization";
const char INTL_FS_WIFI[] PROGMEM = "WiFi Sensor";
const char INTL_FS_WIFI_DESCRIPTION[] PROGMEM = "WiFi Sensor in configuration mode";
const char INTL_FS_WIFI_NAME[] PROGMEM = "Name";
const char INTL_MORE_SETTINGS[] PROGMEM ="More settings";
const char INTL_AUTO_UPDATE[] PROGMEM = "Auto update firmware";
const char INTL_USE_BETA[] PROGMEM = "Load beta firmware";
const char INTL_DISPLAY[] PROGMEM = "OLED SSD1306";
const char INTL_SH1106[] PROGMEM = "OLED SH1106";
const char INTL_FLIP_DISPLAY[] PROGMEM = "OLED display flip";
const char INTL_LCD1602_27[] PROGMEM = "LCD 1602 (I2C: 0x27)";
const char INTL_LCD1602_3F[] PROGMEM = "LCD 1602 (I2C: 0x3F)";
const char INTL_LCD2004_27[] PROGMEM = "LCD 2004 (I2C: 0x27)";
const char INTL_DISPLAY_WIFI_INFO[] PROGMEM = "Display Wifi info";
const char INTL_DISPLAY_DEVICE_INFO[] PROGMEM = "Display device info";
const char INTL_DEBUG_LEVEL[] PROGMEM = "Debug&nbsp;Level";
const char INTL_MEASUREMENT_INTERVAL[] PROGMEM = "Measuring interval";
const char INTL_DURATION_ROUTER_MODE[] PROGMEM = "Duration router mode";
const char INTL_MORE_APIS[] PROGMEM = "More APIs";
const char INTL_SEND_TO_OWN_API[] PROGMEM = "Send data to own API";
const char INTL_SERVER[] PROGMEM = "Server"; 
const char INTL_PATH[] PROGMEM = "Path"; 
const char INTL_PORT[] PROGMEM = "Port"; 
const char INTL_USER[] PROGMEM = "User"; 
const char INTL_PASSWORD[] PROGMEM = "Password"; 
const char INTL_SEND_TO[] PROGMEM = "Send to {v}"; 
const char INTL_READ_FROM[] PROGMEM = "Read from {v}";
const char INTL_SENSOR_IS_REBOOTING[] PROGMEM = "Sensor is rebooting.";
const char INTL_RESTART_DEVICE[] PROGMEM = "Restart device";
const char INTL_DELETE_CONFIG[] PROGMEM = "delete saved configuration";
const char INTL_RESTART_SENSOR[] PROGMEM = "Restart sensor";
const char INTL_HOME[] PROGMEM = "Home";
const char INTL_BACK_TO_HOME[] PROGMEM = "Back to home page";
const char INTL_CURRENT_DATA[] PROGMEM = "Current data";
const char INTL_ACTIVE_SENSORS_MAP[] PROGMEM = "Active sensors map(external link)";
const char INTL_CONFIGURATION_DELETE[] PROGMEM = "Configuration delete";
const char INTL_CONFIGURATION_REALLY_DELETE[] PROGMEM = "Are you sure you want to delete the configuration?";
const char INTL_DELETE[] PROGMEM = "Delete";
const char INTL_CANCEL[] PROGMEM = "Cancel";
const char INTL_REALLY_RESTART_SENSOR[] PROGMEM = "Are you sure you want to restart the sensor?";
const char INTL_RESTART[] PROGMEM = "Restart";
const char INTL_SAVE_AND_RESTART[] PROGMEM = "Save and restart";
const char INTL_FIRMWARE[] PROGMEM = "Firmware version";
const char INTL_DEBUG_SETTING_TO[] PROGMEM = "Debug setting to ";
const char INTL_NONE[] PROGMEM = "off";
const char INTL_ERROR[] PROGMEM = "only errors";
const char INTL_WARNING[] PROGMEM = "warnings";
const char INTL_MIN_INFO[] PROGMEM = "min. info";
const char INTL_MED_INFO[] PROGMEM = "mid. info";
const char INTL_MAX_INFO[] PROGMEM = "max. info";
const char INTL_CONFIG_DELETED[] PROGMEM = "Configuration is deleted";
const char INTL_CONFIG_CAN_NOT_BE_DELETED[] PROGMEM = "Configuration can not be deleted";
const char INTL_CONFIG_NOT_FOUND[] PROGMEM = "Configuration not found";
const char INTL_TIME_TO_FIRST_MEASUREMENT[] PROGMEM = "Still {v} seconds until first measurement.";
const char INTL_TIME_SINCE_LAST_MEASUREMENT[] PROGMEM = " seconds since last measurement.";
const char INTL_PARTICLES_PER_LITER[] PROGMEM = "particles/liter";
const char INTL_PARTICULATE_MATTER[] PROGMEM = "particulate matter";
const char INTL_TEMPERATURE[] PROGMEM = "temperature";
const char INTL_HUMIDITY[] PROGMEM = "humidity";
const char INTL_PRESSURE[] PROGMEM = "air pressure";
const char INTL_LEQ_A[] PROGMEM = "LAeq";
const char INTL_LA_MIN[] PROGMEM = "LA min";
const char INTL_LA_MAX[] PROGMEM = "LA max";
const char INTL_LATITUDE[] PROGMEM = "Latitude";
const char INTL_LONGITUDE[] PROGMEM = "Longitude";
const char INTL_ALTITUDE[] PROGMEM = "Altitude";
const char INTL_DATE[] PROGMEM = "Date";
const char INTL_TIME[] PROGMEM = "Time";
const char INTL_SIGNAL_STRENGTH[] PROGMEM = "signal strength";
const char INTL_SIGNAL_QUALITY[] PROGMEM = "signal quality";
const char INTL_NUMBER_OF_MEASUREMENTS[] PROGMEM = "Number of measurements:";
const char INTL_SENSOR[] PROGMEM = "Sensor";
const char INTL_PARAMETER[] PROGMEM = "Parameter";
const char INTL_VALUE[] PROGMEM = "Value";

#include "./airrohr-logo-common.h"

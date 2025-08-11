# airRohr Sensor Firmware for SPS30, SDS011, DHT22, BMP180, BMP/E 280 and many more

## Features:
* many environmental and air quality sensors can be used concurrently
* Integration in Sensor.Community (formerly Luftdaten.Info)
* Configuration via HTTP in local WiFi or with a Sensor-as Access-Point
* Support for OLED- and LCD-Displays (SSD1306, SH1106 and LCD1602, LCD2004)
* Wide selection of API integrations for measurement reporting
* Ability to print measurements as CSV via USB-serial
* Used with ESP8266 (NodeMCU v2/v3 and compatible) and ESP32 (experimental)

## Buttons Control

### Insight

- `UP` or `DOWN` short press - show graphs screen or change graphs screen
- `SET` short press - show main screen or refresh main screen
- `SET` + `DOWN` long press (4s) - reset WiFi configuration
- `SET` + `DOWN` are pressed while turn on - reset all configuration

### Urban

- `SET` long press (4s) - reset WiFi configuration
- `SET` is pressed while turn on - reset all configuration

## Contributing

To add your Connectivity Robonomics Server to sensors firmware fork this repository and edit [robonomics_servers.h](./robonomics_servers.h) file. Add your server in a list in the following format:
```bash
{"<server_address>", <Region>}
```
Use one of the following variables fo region:
```
INTL_REGION_GLOBAL - Global Servers
INTL_REGION_EU - Europe
INTL_REGION_AS - Asia
INTL_REGION_AF - Africa
INTL_REGION_AU - Australia
INTL_REGION_NA - North America
INTL_REGION_SA - South America
```
For example:
```
{"connectivity.robonomics.network", INTL_REGION_GLOBAL}
```

Then make a pull request.

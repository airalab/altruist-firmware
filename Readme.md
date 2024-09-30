# Firmware for Sensors of Altruist Civil Station

This repository is dedicated to the firmware for the Altruist Civil Station with support for SDS011, DHT22, BMP180, BMP/E 280, and other sensors. It is a fork of the [airRohr Sensor Firmware](https://github.com/opendata-stuttgart/sensors-software). We have added several new features, including a noise meter, a Geiger counter, and a VOC sensor. Additionally, integration with Robonomics has been added to turn sensors into a full-fledged [distributed sensor network](https://sensors.social).

More information can be found in the guides on [Robonomics Academy](https://robonomics.academy/en/learn/sensors-connectivity-course/overview/).

## Features:

* Many environmental and air quality sensors can be used concurrently
* Integration with Sensor.Community (formerly Luftdaten.Info)
* Configuration via HTTP in local WiFi or with the sensor as an access point
* Support for OLED and LCD displays (SSD1306, SH1106, LCD1602, and LCD2004)
* A wide selection of API integrations for measurement reporting
* Compatible with ESP8266 (NodeMCU v2/v3 and compatible) and ESP32 (experimental)

## Sensor Setup

You can find more information about acquiring the sensor in the [Sensor Hardware guide](https://robonomics.academy/en/learn/sensors-connectivity-course/sensor-hardware/) on Robonomics Academy.

Detailed information about sensor setup can be found in the [Setting up and connecting sensors](https://robonomics.academy/en/learn/sensors-connectivity-course/setting-up-and-connecting-sensors/) article.


## WiFi Configuration

If a (previously) configured WiFi is not reachable within 20 seconds after power-on, the firmware switches into AP mode and creates a WiFi network in the form `RobonomicsSensor-[Sensor-ID]`.

This WiFi network is by default unencrypted. When a client connects to it, they will be redirected to the sensor's web server `http://192.168.4.1/`, which allows initial configuration.

Configurable options include:
* WiFi Access Point to use
* Measurement options (sensors to poll, intervals, etc.)
* APIs to send the measurements

The unencrypted Access Point for initial configuration will turn itself off after about 10 minutes. To reactivate it, please power cycle the board.

## Build and Flash Firmware from Source

To build the firmware, you need the [Platformio](https://platformio.org/) tool. In this article, we'll provide instructions on how to build the firmware with [Platformio core](https://docs.platformio.org/en/latest/core/installation/index.html).

Clone the repo:

```bash
git clone https://github.com/airalab/sensors-software.git
cd sensors-software
```

Build the firmware:

```bash
pio run -e nodemcuv2_en
```

The firmware binary file will be in the `builds` folder.

To flash the device, connect it to the computer with a USB cable and run:

```bash
pio run -t upload -e nodemcuv2_en
```

## Contributing

To add your Connectivity Robonomics Server to the sensors firmware, fork this repository and edit the [robonomics_servers.h](./robonomics_servers.h) file. Add your server to the list in the following format:

```bash
{"<server_address>", <Region>}
```

Use one of the following variables for the region:

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

## DHT22 Humidity Reporting

The DHT22 sensor is originally an indoor sensor. For outdoor use, it appears to be rather sensitive to water condensation after reaching 100% relative humidity, which can cause it to stay at a 99.9% value for a very long time (sometimes indefinitely). Additionally, it appears to be sensitive to high UV light, which can cause the sensor to crash until it is hard power-cycled.

Better experiences have been reported with a BME280 or SHT3x sensor, so consider using those instead.

## Debug via USB-Serial

Connecting or powering via a computer USB will provide a USB serial connection with the settings 115200 baud. By default, the sensor will provide human-readable debug information of configurable granularity through this connection.


## Save as CSV

All measurements can also be read as CSV via USB-Serial when using the USB port with the settings 9600 Baud 8N1. To avoid interference from debug options (see earlier section), set debug to "None" in the configuration.

## Wiring and Sensors

![https://raw.githubusercontent.com/opendata-stuttgart/meta/master/files/nodemcu-v3-schaltplan-sds011.jpg](https://raw.githubusercontent.com/opendata-stuttgart/meta/master/files/nodemcu-v3-schaltplan-sds011.jpg)

Please refer to the [Pinout of NodeMCU v2 and v3](https://github.com/opendata-stuttgart/meta/wiki/Pinouts-NodeMCU-v2,-v3) for much more detailed information about the individual pin functions.

---

### SDS011 (serial)

> **Note**: Serial connections are always crossed (RX on one side is connected with TX on the other side).

* Pin 1 (TX)   -> (RX) Pin D1 (GPIO5)
* Pin 2 (RX)   -> (TX) Pin D2 (GPIO4)
* Pin 3 (GND)  -> GND
* Pin 4 (2.5m) -> unused
* Pin 5 (5V)   -> VU
* Pin 6 (1m)   -> unused

---

### DHT22

* Pin 1 => 3V3
* Pin 2 => Pin D7 (GPIO13)
* Pin 3 => unused
* Pin 4 => GND

---

### DS18B20 (OneWire interface)

Please check your version (pinout) at [https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf].  
The DS18B20 uses the same PIN D7 as the DHT22, so only one of them (DHT22 or DS18B20) can be used at a time.

* GND  -> Pin GND
* DQ   -> Pin D7 (GPIO 13)
* VCC  -> Pin 3V3 or Pin VU

---

### PMS1003 to PMS6003

Pinout:
   8 7 6 5 4 3 2 1

* Pin 1 (VCC)   -> VU
* Pin 2 (GND)   -> GND
* Pin 3 (SET)   -> unused
* Pin 4 (RX)    -> Pin D2 (GPIO4)
* Pin 5 (TX)    -> Pin D1 (GPIO5)
* Pin 6 (RESET) -> unused
* Pin 7	(NC)	-> unused
* Pin 8 (NC)	-> unused

---

### PMS7003

Pinout PMS7003:
   9  7  5  3  1
  10  8  6  4  2

* Pin  1/2 (VCC) -> VU
* Pin  3/4 (GND) -> GND
* Pin  5 (RESET) -> unused
* Pin  6 (NC)    -> unused
* Pin  7 (RX)    -> Pin D2 (GPIO4)
* Pin  8 (NC)    -> unused
* Pin  9 (TX)    -> Pin D1 (GPIO5)
* Pin 10 (SET)   -> unused

---

### Honeywell PM sensor

Pinout:
   8 7 6 5 4 3 2 1
* Pin 1 (3.3V)   -> unused
* Pin 2 (5V)     -> VU
* Pin 3 (NC)     -> unused
* Pin 4 (NC)     -> unused
* Pin 5 TEST)    -> unused
* Pin 6 (TX)     -> Pin D1 (GPIO5)
* Pin 7	(RX)     -> Pin D2 (GPIO4)
* Pin 8 (GND)    -> GND

---

### BMP180 / BME/P280 / HTU21D / SHT3x (I2C)

* VCC  ->  Pin 3V3
* GND  ->  Pin GND
* SCL  ->  Pin D4 (GPIO2)
* SDA  ->  Pin D3 (GPIO0)

---

### SPS30 (I2C, 5V)

Pinout:
   1 2 3 4 5
* Pin 1 (5V)     -> Pin VU/VIN
* Pin 2 (SDA)    -> Pin D3 (GPIO0)
* Pin 3 (SCL)    -> Pin D4 (GPIO2)
* Pin 4 (SEL)    -> Pin GND
* Pin 5 (GND)    -> Pin GND

---

### LCD1602 (I2C or 5V versions)

* VCC  ->  Pin VU
* GND  ->  Pin GND
* SCL  ->  Pin D4 (GPIO2)
* SDA  ->  Pin D3 (GPIO0)

---

### OLED displays with SSD1306 (I2C, 128x64 pixels)

* VCC -> Pin VU
* GND -> Pin GND
* SCL  ->  Pin D4 (GPIO2)
* SDA  ->  Pin D3 (GPIO0)

---

### GPS NEO 6M (serial)

> **Warning**: USE AT YOUR OWN RISK. In combination with the PM sensor, the firmware may crash!!!

VCC and GND can be provided by the board (use 3.3V only).

> **Note**: Serial connections are always crossed (RX on one side is connected with TX on the other side).

* TX von Neo -> Pin D5 (RX)
* RX von Neo -> Pin D6 (TX)

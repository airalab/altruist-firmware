#ifndef _HTML_CONTENT_H
#define _HTML_CONTENT_H

#include "../intl.h"
#include "../defines.h"
#include "css-styles.h"

const char TXT_CONTENT_TYPE_JSON[] PROGMEM = "application/json";
const char TXT_CONTENT_TYPE_INFLUXDB[] PROGMEM = "application/x-www-form-urlencoded";
const char TXT_CONTENT_TYPE_TEXT_HTML[] PROGMEM = "text/html; charset=utf-8";
const char TXT_CONTENT_TYPE_TEXT_CSS[] PROGMEM = "text/css";
const char TXT_CONTENT_TYPE_TEXT_JS[] PROGMEM = "text/javascript";
const char TXT_CONTENT_TYPE_TEXT_PLAIN[] PROGMEM = "text/plain";
const char TXT_CONTENT_TYPE_IMAGE_PNG[] PROGMEM = "image/png";
const char TXT_CONTENT_TYPE_IMAGE_SVG[] PROGMEM = "image/svg+xml";

const char SENSORS_SDS011[] PROGMEM = "SDS011";
const char SENSORS_GC[] PROGMEM = "Geiger Counter";
const char SENSORS_CCS811[] PROGMEM = "CCS811";
const char SENSORS_PPD42NS[] PROGMEM = "PPD42NS";
const char SENSORS_PMSx003[] PROGMEM = "PMSx003";
const char SENSORS_HPM[] PROGMEM = "Honeywell PM";
const char SENSORS_NPM[] PROGMEM = "Tera Sensor Next PM";
const char SENSORS_SPS30[] PROGMEM = "Sensirion SPS30";
const char SENSORS_DHT22[] PROGMEM = "DHT22";
const char SENSORS_DS18B20[] PROGMEM = "DS18B20";
const char SENSORS_HTU21D[] PROGMEM = "HTU21D";
const char SENSORS_DBMETER[] PROGMEM = "Noise Sensor";
const char SENSORS_SHT3X[] PROGMEM = "SHT3x";
const char SENSORS_BMP180[] PROGMEM = "BMP180";
const char SENSORS_BME280[] PROGMEM = "BME280";
const char SENSORS_BMP280[] PROGMEM = "BMP280";
const char SENSORS_DNMS[] PROGMEM = "DNMS";

const char WEB_PAGE_HEADER[] PROGMEM = "<!DOCTYPE html><html lang='" INTL_LANG "'>\
<head>\
<meta charset='utf-8'/>\
<title>{t}</title>";

#define STATIC_PREFIX "/" INTL_LANG "_s1.4"
// Bust browser cache after PNG→SVG / logo changes; `m` reflects build (insight vs urban).
#define WEB_HEADER_LOGO_SRC STATIC_PREFIX "?r=logo&v=" SOFTWARE_VERSION_STR "&m=" DEVICE_MODEL

const char WEB_PAGE_HEADER_HEAD[] PROGMEM = "<meta name='viewport' content='width=device-width'/>\
    <link rel='stylesheet' href='" STATIC_PREFIX "?r=css'>\
    </style>\
    </head><body>\
    <div class='canvas'>\
    <a class='b' href='/' style='background:none;display:inline'>\
    <img src='" WEB_HEADER_LOGO_SRC "' alt='" INTL_BACK_TO_HOME "' style='float:left;margin:12px' width='160' height='160'/></a>";

const char WEB_PAGE_HEADER_CONFIG_HEAD[] PROGMEM = "<meta name='viewport' content='width=device-width'/>\
<link rel='stylesheet' href='" STATIC_PREFIX "?r=css'>\
</style>\
<script src='" STATIC_PREFIX "?r=js' defer></script>\
</head><body>\
<div class='canvas'>\
<a class='b' href='/' style='background:none;display:inline'>\
<img src='" WEB_HEADER_LOGO_SRC "' alt='" INTL_BACK_TO_HOME "' style='float:left;margin:12px' width='160' height='160'/></a>";

const char WEB_PAGE_HEADER_BODY[] PROGMEM = "<div class='canvas-info'>\
    <h3>" PM_SENSOR_NAME "</h3>\
    <small>\
    <span>ID</span>: {id}<br />\
    <span>" INTL_FIRMWARE "</span>: " SOFTWARE_VERSION_STR "/" INTL_LANG "&nbsp;(" __DATE__ ")<br/>\
    <span>" INTL_ROBONOMICS_ADDR "</span>: <span style='cursor:pointer;border-bottom:1px dashed #999;font-weight:normal' title='Click to copy' onclick='var s=this,t=document.createElement(\"textarea\");t.value=s.innerText;t.style.position=\"fixed\";t.style.opacity=\"0\";document.body.appendChild(t);t.select();document.execCommand(\"copy\");document.body.removeChild(t);s.style.opacity=0.5;setTimeout(function(){s.style.opacity=1},300)'>{addr}</span>\
    </small>\
    </div>\
    </div><div class='content'><h4 class='content-subtitle'><a href='/' style='background:none;display:inline'>" INTL_HOME "</a> {n} {t}</h4>";

const char WEB_PAGE_DEBUG_HEADER_BODY[] PROGMEM = "<div class='canvas-info'>\
    <h3>" PM_SENSOR_NAME "</h3>\
    <small>\
    <span>ID</span>: {id}<br />\
    <span>" INTL_FIRMWARE "</span>: " SOFTWARE_VERSION_STR "/" INTL_LANG "&nbsp;(" __DATE__ ")<br/>\
    <span>" INTL_ROBONOMICS_ADDR "</span>: <span style='cursor:pointer;border-bottom:1px dashed #999;font-weight:normal' title='Click to copy' onclick='var s=this,t=document.createElement(\"textarea\");t.value=s.innerText;t.style.position=\"fixed\";t.style.opacity=\"0\";document.body.appendChild(t);t.select();document.execCommand(\"copy\");document.body.removeChild(t);s.style.opacity=0.5;setTimeout(function(){s.style.opacity=1},300)'>{addr}</span>\
    </small>\
    </div>\
    </div><div class='content content-debug'><h4 class='content-subtitle'><a href='/' style='background:none;display:inline'>" INTL_HOME "</a> {n} {t}</h4>";

const char WEB_PAGE_CONFIG_HEADER_BODY[] PROGMEM = "<div class='canvas-info'>\
    <h3>" PM_SENSOR_NAME "</h3>\
    <small>\
    <span>ID</span>: {id}<br />\
    <span>" INTL_FIRMWARE "</span>: " SOFTWARE_VERSION_STR "/" INTL_LANG "&nbsp;(" __DATE__ ")<br/>\
    <span>" INTL_ROBONOMICS_ADDR "</span>: <span style='cursor:pointer;border-bottom:1px dashed #999;font-weight:normal' title='Click to copy' onclick='var s=this,t=document.createElement(\"textarea\");t.value=s.innerText;t.style.position=\"fixed\";t.style.opacity=\"0\";document.body.appendChild(t);t.select();document.execCommand(\"copy\");document.body.removeChild(t);s.style.opacity=0.5;setTimeout(function(){s.style.opacity=1},300)'>{addr}</span>\
    </small>\
    </div>\
    </div><div class='content content-config'><h4 class='content-subtitle'><a href='/' style='background:none;display:inline'>" INTL_HOME "</a> {n} {t}</h4>";



const char BR_TAG[] PROGMEM = "<br/>";
const char WEB_DIV_PANEL[] PROGMEM = "</div><div class='panel' id='panel{v}'>";
const char TABLE_TAG_OPEN[] PROGMEM = "<table class='content-table'>";
const char TABLE_TAG_CLOSE_BR[] PROGMEM = "</table>";
const char EMPTY_ROW[] PROGMEM = "<tr><td colspan='3' style='background: #f4f4f4;'>&nbsp;</td></tr>";

const char WEB_PAGE_FOOTER[] PROGMEM = "<br/><br/>"
	"<a class='b home-btn' href='/'>" INTL_BACK_TO_HOME "</a><br/><br/><br/>"
	"</div><footer class='footer'><div style='padding:16px'>"
	"<a href='https://robonomics.network/' target='_blank' rel='noreferrer' style='color:#fff;'>© Robonomics Network</a>&nbsp;&nbsp;(<a href='https://github.com/airalab/altruist-firmware/issues' target='_blank' rel='noreferrer' style='color:#fff;'>" INTL_REPORT_ISSUE "</a>)<br/><span class='footer-polkadot'>Secured by</span>"
	"</div></footer></body></html>\r\n";

const char WEB_PAGE_ROOT_FOOTER[] PROGMEM = "<br/><br/>"
    "</div><footer class='footer'><div style='padding:16px'>"
    "<a href='https://robonomics.network/' target='_blank' rel='noreferrer' style='color:#fff;'>© Robonomics Network</a>&nbsp;&nbsp;(<a href='https://github.com/airalab/altruist-firmware/issues' target='_blank' rel='noreferrer' style='color:#fff;'>" INTL_REPORT_ISSUE "</a>)<br/><span class='footer-polkadot'>Secured by</span>"
    "</div></footer></body></html>\r\n";


const char WEB_ROOT_PAGE_CONTENT[] PROGMEM = "<a class='b' href='/values'>{t}</a>\
<a class='b' href='/status'>{s}</a>\
<a class='b' href='https://sensors.social/' target='_blank' rel='noreferrer'>" INTL_ACTIVE_SENSORS_MAP "</a>\
<a class='b' href='/config'>{conf}</a>\
<a class='b' href='/ota'>" INTL_OTA_UPDATE "</a>\
<a class='b danger' href='/removeConfig'>" INTL_CONFIGURATION_DELETE "</a>\
<a class='b danger' href='/restart'>{restart}</a>\
<a class='b' href='/debug'>{debug}</a>\
";

const char WEB_CONFIG_SCRIPT[] PROGMEM = "<script>\
function setSSID(ssid){document.getElementById('wlanssid').value=ssid.innerText||ssid.textContent;document.getElementById('wlanpwd').focus();}\
function load_wifi_list(){var x=new XMLHttpRequest();x.open('GET','/wifi');x.onload = function(){if (x.status === 200) {document.getElementById('wifilist').innerHTML = x.responseText;}};x.send();}\
</script>";

const char WEB_REMOVE_CONFIG_CONTENT[] PROGMEM = "<h3>" INTL_CONFIGURATION_REALLY_DELETE "</h3>\
<table class='content-table delete-table'><tr><td><form method='POST' action='/removeConfig'>\
<input type='radio' id='allConfig' name='configType' value='all' class='radio-input' checked>\
<label for='allConfig'>All Configuration</label><br />\
<input type='radio' id='wifiConfig' name='configType' value='wifi' class='radio-input'>\
<label for='wifiConfig'>WiFi Configuration</label><br />\
<input type='submit' class='s_red submit-btn--config' name='submit' value='" INTL_DELETE "'/></form></td>\
</tr><tr><td><a class='b' href='/'>" INTL_CANCEL "</a></td></tr></table>\
";

const char WEB_RESET_CONTENT[] PROGMEM = "<h3>" INTL_REALLY_RESTART_SENSOR "</h3>" \
"<table class='content-table delete-table'><tr><td><form method='POST' action'/reset'>" \
"<input type='submit' class='s_red' name='submit' value='" INTL_RESTART "'/>"\
"</form></td><td><a class='b' href='/'>" INTL_CANCEL "</a></td></tr></table>";


const char WEB_IOS_REDIRECT[] PROGMEM = "<html><body>Redirecting...\
<script type=\"text/javascript\">\
window.location = \"http://192.168.4.1/config\";\
</script>\
</body></html>";

const char WEB_B_BR_BR[] PROGMEM = "</b><br/><br/>";
const char WEB_BRACE_BR[] PROGMEM = ")<br/>";
const char WEB_BRACE_BRE[] PROGMEM = "<br/>";
const char WEB_B_BR[] PROGMEM = "</b><br/>";
const char WEB_BR_BR[] PROGMEM = "<br/><br/>";
const char WEB_BR_FORM[] PROGMEM = "<br/></form>";
const char WEB_BR_LF_B[] PROGMEM = "<br/>\n<b>";
const char WEB_LF_B[] PROGMEM = "\n<b>";
const char WEB_CSV[] PROGMEM = "CSV";
const char WEB_FEINSTAUB_APP[] PROGMEM = "<a target='_blank' href='https://chillibits.com/pmapp'>Feinstaub-App</a>";
const char WEB_ROBONOMICS[] PROGMEM = "<a target='_blank' href='https://robonomics.network/'>Robonomics</a>";
const char WEB_HTTPS[] PROGMEM = "HTTPS";
const char WEB_NBSP_NBSP_BRACE[] PROGMEM = "&nbsp;&nbsp;(";
const char WEB_REPLN_REPLV[] PROGMEM = "\"{n}\":\"{v}\",";
const char WEB_PM1[] PROGMEM = "PM1";
const char WEB_PM25[] PROGMEM = "PM2.5";
const char WEB_PM10[] PROGMEM = "PM10";
const char WEB_CO2[] PROGMEM = "CO2";
const char WEB_TVOC[] PROGMEM = "TVOC";
const char WEB_PM4[] PROGMEM = "PM4";
const char WEB_NC0k5[] PROGMEM = "NC0.5";
const char WEB_NC1k0[] PROGMEM = "NC1.0";
const char WEB_NC2k5[] PROGMEM = "NC2.5";
const char WEB_NC4k0[] PROGMEM = "NC4.0";
const char WEB_NC10[] PROGMEM = "NC10";
const char WEB_TPS[] PROGMEM = "TPS";
const char WEB_GPS[] PROGMEM = "GPS";

#endif // _HTML_CONTENT_H
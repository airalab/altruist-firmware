#ifndef _HTML_CONTENT_H
#define _HTML_CONTENT_H

#include "../intl.h"
#include "../defines.h"
#if defined(ALTRUIST_URBAN_C3_LITE)
#include "css-styles-c3.h"
#else
#include "css-styles.h"
#endif

const char TXT_CONTENT_TYPE_JSON[] PROGMEM = "application/json";
const char TXT_CONTENT_TYPE_INFLUXDB[] PROGMEM = "application/x-www-form-urlencoded";
const char TXT_CONTENT_TYPE_TEXT_HTML[] PROGMEM = "text/html; charset=utf-8";
const char TXT_CONTENT_TYPE_TEXT_CSS[] PROGMEM = "text/css";
const char TXT_CONTENT_TYPE_TEXT_JS[] PROGMEM = "text/javascript";
const char TXT_CONTENT_TYPE_TEXT_PLAIN[] PROGMEM = "text/plain";
const char TXT_CONTENT_TYPE_IMAGE_PNG[] PROGMEM = "image/png";
const char TXT_CONTENT_TYPE_IMAGE_SVG[] PROGMEM = "image/svg+xml";

const char WEB_PAGE_HEADER[] PROGMEM = "<!DOCTYPE html><html lang='" INTL_LANG "'>\
<head>\
<meta charset='utf-8'/>\
<title>{t}</title>";

#define WEB_PAGE_FAVICON_LINKS \
	"<link rel='icon' href='/favicon.ico' sizes='any'>" \
	"<link rel='icon' type='image/png' href='/favicon.ico' media='(prefers-color-scheme: light)'>" \
	"<link rel='icon' type='image/png' href='/favicon-dark.ico' media='(prefers-color-scheme: dark)'>"

#define STATIC_PREFIX "/" INTL_LANG "_s3.119"
// Bust browser cache after PNG→SVG / logo changes; `m` reflects build (insight vs urban).
#define WEB_HEADER_LOGO_SRC STATIC_PREFIX "?r=logo&v=" SOFTWARE_VERSION_STR "&m=" DEVICE_MODEL
#define WEB_NAV_ICON_LOCAL_SRC STATIC_PREFIX "?r=nav-local&v=" SOFTWARE_VERSION_STR
#define WEB_NAV_ICON_MAP_SRC STATIC_PREFIX "?r=nav-map&v=" SOFTWARE_VERSION_STR
#define WEB_NAV_ICON_CUSTOM_SRC STATIC_PREFIX "?r=nav-custom&v=" SOFTWARE_VERSION_STR
#define WEB_NAV_ICON_SYSTEM_SRC STATIC_PREFIX "?r=nav-system&v=" SOFTWARE_VERSION_STR
#define WEB_STATIC_CSS_HREF STATIC_PREFIX "?r=css&v=" SOFTWARE_VERSION_STR
#define WEB_STATIC_JS_HREF STATIC_PREFIX "?r=js&v=" SOFTWARE_VERSION_STR
#if defined(ALTRUIST_URBAN_C3_LITE)
// C3 flash budget: text-only tabs (PNG nav icons are ~47 KB in PROGMEM).
#define WEB_APP_TAB_LOCAL "<a class='app-tab' data-tab='local' href='/'><span class='app-tab__label'>" INTL_HUB_LOCAL_TITLE "</span></a>"
#define WEB_APP_TAB_SOCIAL "<a class='app-tab' data-tab='social' href='/social'><span class='app-tab__label'>" INTL_DASH_CAT_MAP "</span></a>"
#define WEB_APP_TAB_CUSTOM "<a class='app-tab' data-tab='custom' href='/custom'><span class='app-tab__label'>" INTL_DASH_GROUP_CUSTOM_TITLE "</span></a>"
#define WEB_APP_TAB_ADVANCED "<a class='app-tab' data-tab='advanced' href='/advanced'><span class='app-tab__label'>" INTL_NAV_ADVANCED "</span></a>"
#else
#define WEB_APP_TAB_LOCAL "<a class='app-tab' data-tab='local' href='/'><img class='app-tab__icon' src='" WEB_NAV_ICON_LOCAL_SRC "' alt='' width='24' height='24' decoding='async'/><span class='app-tab__label'>" INTL_HUB_LOCAL_TITLE "</span></a>"
#define WEB_APP_TAB_SOCIAL "<a class='app-tab' data-tab='social' href='/social'><img class='app-tab__icon' src='" WEB_NAV_ICON_MAP_SRC "' alt='' width='24' height='24' decoding='async'/><span class='app-tab__label'>" INTL_DASH_CAT_MAP "</span></a>"
#define WEB_APP_TAB_CUSTOM "<a class='app-tab' data-tab='custom' href='/custom'><img class='app-tab__icon' src='" WEB_NAV_ICON_CUSTOM_SRC "' alt='' width='24' height='24' decoding='async'/><span class='app-tab__label'>" INTL_DASH_GROUP_CUSTOM_TITLE "</span></a>"
#define WEB_APP_TAB_ADVANCED "<a class='app-tab' data-tab='advanced' href='/advanced'><img class='app-tab__icon' src='" WEB_NAV_ICON_SYSTEM_SRC "' alt='' width='24' height='24' decoding='async'/><span class='app-tab__label'>" INTL_NAV_ADVANCED "</span></a>"
#endif
#define WEB_APP_BREADCRUMB "<nav class='app-breadcrumb' aria-label='" INTL_BREADCRUMB_ARIA "'><a class='app-breadcrumb__link' href='/'>{home}</a><span class='app-breadcrumb__sep' aria-hidden='true'>&rsaquo;</span><span class='app-breadcrumb__current' aria-current='page'>{t}</span></nav>"

#if defined(ALTRUIST_URBAN_C3_LITE)
#define WEB_FW_BUILD_INFO SOFTWARE_VERSION_STR "/" INTL_LANG
#define WEB_ROBONOMICS_ADDR_FIELD "<span class='canvas-address'>{addr}</span>"
#else
#define WEB_FW_BUILD_INFO SOFTWARE_VERSION_STR "/" INTL_LANG "&nbsp;(" __DATE__ ")"
#define WEB_ROBONOMICS_ADDR_FIELD "<span class='canvas-address' title='Click to copy' onclick='var s=this,t=document.createElement(\"textarea\");t.value=s.innerText;t.style.position=\"fixed\";t.style.opacity=\"0\";document.body.appendChild(t);t.select();document.execCommand(\"copy\");document.body.removeChild(t);s.style.opacity=0.5;setTimeout(function(){s.style.opacity=1},300)'>{addr}</span>"
#endif

const char WEB_PAGE_HEADER_HEAD[] PROGMEM = "<meta name='viewport' content='width=device-width'/>" \
    WEB_PAGE_FAVICON_LINKS \
    "<link rel='stylesheet' href='" WEB_STATIC_CSS_HREF "'>\
    </style>\
    </head><body>\
    <div class='canvas'>\
    <a class='b' href='/' style='background:none;display:inline'>\
    <img class='canvas-logo' src='" WEB_HEADER_LOGO_SRC "' alt='" INTL_BACK_TO_HOME "' width='160' height='160'/></a>";

const char WEB_PAGE_HEADER_CONFIG_HEAD[] PROGMEM = "<meta name='viewport' content='width=device-width'/>" \
WEB_PAGE_FAVICON_LINKS \
"<link rel='stylesheet' href='" WEB_STATIC_CSS_HREF "'>\
</style>\
<script src='" WEB_STATIC_JS_HREF "' defer></script>\
</head><body class='configuration'>\
<div class='canvas'>\
<a class='b' href='/' style='background:none;display:inline'>\
<img class='canvas-logo' src='" WEB_HEADER_LOGO_SRC "' alt='" INTL_BACK_TO_HOME "' width='160' height='160'/></a>";

const char WEB_PAGE_HEADER_BODY[] PROGMEM = "<div class='canvas-info'>\
    <h3>" PM_SENSOR_NAME "</h3>\
    <small>\
    <span>ID</span>: {id}<br />\
    <span>" INTL_FIRMWARE "</span>: " WEB_FW_BUILD_INFO "<br/>\
    <span>" INTL_ROBONOMICS_ADDR "</span>: " WEB_ROBONOMICS_ADDR_FIELD "\
    </small>\
    </div>\
    </div><div class='content'><h4 class='content-subtitle'><a href='/' style='background:none;display:inline'>" INTL_HOME "</a> {n} {t}</h4>";

const char WEB_PAGE_DEBUG_HEADER_BODY[] PROGMEM = "<div class='canvas-info'>\
    <h3>" PM_SENSOR_NAME "</h3>\
    <small>\
    <span>ID</span>: {id}<br />\
    <span>" INTL_FIRMWARE "</span>: " WEB_FW_BUILD_INFO "<br/>\
    <span>" INTL_ROBONOMICS_ADDR "</span>: " WEB_ROBONOMICS_ADDR_FIELD "\
    </small>\
    </div>\
    </div><div class='content content-debug'><h4 class='content-subtitle'><a href='/' style='background:none;display:inline'>" INTL_HOME "</a> {n} {t}</h4>";

const char WEB_PAGE_CONFIG_HEADER_BODY[] PROGMEM = "<div class='canvas-info'>\
    <h3>" PM_SENSOR_NAME "</h3>\
    <small>\
    <span>ID</span>: {id}<br />\
    <span>" INTL_FIRMWARE "</span>: " WEB_FW_BUILD_INFO "<br/>\
    <span>" INTL_ROBONOMICS_ADDR "</span>: " WEB_ROBONOMICS_ADDR_FIELD "\
    </small>\
    </div>\
    </div><div class='content content-config'><h4 class='content-subtitle'><a href='/' style='background:none;display:inline'>" INTL_HOME "</a> {n} {t}</h4>";

const char WEB_PAGE_GUEST_HEADER_BODY[] PROGMEM = "<div class='canvas-info'>\
    <h3>" PM_SENSOR_NAME "</h3>\
    <small>\
    <span>ID</span>: {id}<br />\
    <span>" INTL_FIRMWARE "</span>: " WEB_FW_BUILD_INFO "<br/>\
    </small>\
    </div>\
    </div><div class='content content-config content-guest'>";

const char WEB_PAGE_DATA_HEADER_BODY[] PROGMEM = "<div class='canvas-info'>\
    <h3>" PM_SENSOR_NAME "</h3>\
    <small>\
    <span>ID</span>: {id}<br />\
    <span>" INTL_FIRMWARE "</span>: " WEB_FW_BUILD_INFO "<br/>\
    <span>" INTL_ROBONOMICS_ADDR "</span>: " WEB_ROBONOMICS_ADDR_FIELD "\
    </small>\
    </div>\
    </div><div class='content content-data'><h4 class='content-subtitle'><a href='/' style='background:none;display:inline'>" INTL_HOME "</a> {n} {t}</h4>";

const char WEB_PAGE_APP_HEADER_HEAD[] PROGMEM = "<meta name='viewport' content='width=device-width'/>" \
    WEB_PAGE_FAVICON_LINKS \
    "<link rel='stylesheet' href='" WEB_STATIC_CSS_HREF "'>\
    </style>\
    </head><body class='app-shell' data-page='{page}'>";

const char WEB_PAGE_APP_CONFIG_HEADER_HEAD[] PROGMEM = "<meta name='viewport' content='width=device-width'/>" \
    WEB_PAGE_FAVICON_LINKS \
    "<link rel='stylesheet' href='" WEB_STATIC_CSS_HREF "'>\
    </style>\
    <script src='" WEB_STATIC_JS_HREF "' defer></script>\
    </head><body class='app-shell configuration' data-page='{page}'>";

const char WEB_PAGE_APP_TOPBAR_BODY[] PROGMEM = "<header class='app-topbar'>\
<div class='app-topbar__inner'>\
<a class='app-topbar__brand' href='/'>\
<img class='app-topbar__logo' src='" WEB_HEADER_LOGO_SRC "' alt='" INTL_BACK_TO_HOME "' width='48' height='48'/></a>\
<div class='app-topbar__primary'>\
<div class='app-topbar__heading'>\
<h1 class='app-topbar__title'>" PM_SENSOR_NAME " — {device}</h1>\
{tags}\
</div></div>\
<div class='app-topbar__chips'>\
{device_chip}\
{send_chip}\
<button type='button' class='app-topbar__chip app-topbar__chip--copy' data-copy='{ip}' data-copied='" INTL_COPIED "'>\
<span class='app-topbar__chip-lbl'>" INTL_IP_ADDRESS "</span>\
<span class='app-topbar__chip-val app-topbar__chip-val--mono app-topbar__chip-val--ip'>{ip}</span>\
<span class='app-topbar__chip-sub app-topbar__chip-sub--muted'>{host}</span>\
<span class='app-topbar__chip-hint'>" INTL_TOPBAR_TAP_COPY "</span>\
</button>\
</div></div>\
<script>(function(){function c(t){if(navigator.clipboard&&navigator.clipboard.writeText)return navigator.clipboard.writeText(t);return new Promise(function(r,j){try{var o=document.createElement('textarea');o.value=t;o.style.position='fixed';o.style.opacity='0';document.body.appendChild(o);o.select();document.execCommand('copy');document.body.removeChild(o);r()}catch(e){j(e)}})}function bindCopy(el,hintSel){el.addEventListener('click',function(ev){ev.stopPropagation();var v=el.getAttribute('data-copy')||'';if(!v||v==='—')return;var h=hintSel?el.querySelector(hintSel):null;var p=h?h.textContent:'';c(v).then(function(){if(h){h.textContent=el.getAttribute('data-copied')||'Copied';setTimeout(function(){h.textContent=p},1200)}}).catch(function(){})})}document.querySelectorAll('.app-topbar__chip--copy').forEach(function(b){bindCopy(b,'.app-topbar__chip-hint')});document.querySelectorAll('.app-topbar__chip-copy').forEach(function(b){bindCopy(b,null)})})();</script>\
</header>";

const char WEB_PAGE_APP_LAYOUT_OPEN[] PROGMEM = "<div class='app-layout'>";

const char WEB_PAGE_APP_MAIN_OPEN[] PROGMEM = "<div class='content content-app app-main'>" WEB_APP_BREADCRUMB "<h2 class='app-page-title'>{t}</h2>";

const char WEB_PAGE_APP_CONFIG_MAIN_OPEN[] PROGMEM = "<div class='content content-app content-config app-main'>" WEB_APP_BREADCRUMB "<h2 class='app-page-title'>{t}</h2>";



const char BR_TAG[] PROGMEM = "<br/>";
const char WEB_DIV_PANEL[] PROGMEM = "</div><div class='panel' id='panel{v}'>";
const char TABLE_TAG_OPEN[] PROGMEM = "<table class='content-table'>";
const char TABLE_TAG_CLOSE_BR[] PROGMEM = "</table>";

#if defined(ALTRUIST_URBAN_C3_LITE)
const char WEB_PAGE_APP_FOOTER[] PROGMEM = "</div></div>\
<nav class='app-bottom-nav' aria-label='" INTL_NAV_MAIN "'>\
" WEB_APP_TAB_LOCAL WEB_APP_TAB_SOCIAL WEB_APP_TAB_CUSTOM WEB_APP_TAB_ADVANCED "\
</nav>\
<footer class='footer footer--app'><div style='padding:16px'>\
<a href='https://github.com/airalab/altruist-firmware/issues' target='_blank' rel='noreferrer' style='color:#fff;'>" INTL_REPORT_ISSUE "</a>\
</div></footer></body></html>\r\n";

const char WEB_PAGE_GUEST_FOOTER[] PROGMEM = "<br/><br/>"
	"</div><footer class='footer'><div style='padding:16px'>"
	"<a href='https://github.com/airalab/altruist-firmware/issues' target='_blank' rel='noreferrer' style='color:#fff;'>" INTL_REPORT_ISSUE "</a>"
	"</div></footer></body></html>\r\n";

const char WEB_PAGE_FOOTER[] PROGMEM = "<br/><br/>"
	"<a class='b home-btn' href='/'>" INTL_BACK_TO_HOME "</a><br/><br/><br/>"
	"</div><footer class='footer'><div style='padding:16px'>"
	"<a href='https://github.com/airalab/altruist-firmware/issues' target='_blank' rel='noreferrer' style='color:#fff;'>" INTL_REPORT_ISSUE "</a>"
	"</div></footer></body></html>\r\n";

const char WEB_PAGE_ROOT_FOOTER[] PROGMEM = "<br/><br/>"
    "</div><footer class='footer'><div style='padding:16px'>"
    "<a href='https://github.com/airalab/altruist-firmware/issues' target='_blank' rel='noreferrer' style='color:#fff;'>" INTL_REPORT_ISSUE "</a>"
    "</div></footer></body></html>\r\n";
#else
const char WEB_PAGE_APP_FOOTER[] PROGMEM = "</div></div>\
<nav class='app-bottom-nav' aria-label='" INTL_NAV_MAIN "'>\
" WEB_APP_TAB_LOCAL WEB_APP_TAB_SOCIAL WEB_APP_TAB_CUSTOM WEB_APP_TAB_ADVANCED "\
</nav>\
<footer class='footer footer--app'><div style='padding:16px'>\
<a href='https://robonomics.network/' target='_blank' rel='noreferrer' style='color:#fff;'>© Robonomics Network</a>&nbsp;&nbsp;(<a href='https://github.com/airalab/altruist-firmware/issues' target='_blank' rel='noreferrer' style='color:#fff;'>" INTL_REPORT_ISSUE "</a>)<br/><span class='footer-polkadot'>Secured by</span>\
</div></footer></body></html>\r\n";

const char WEB_PAGE_GUEST_FOOTER[] PROGMEM = "<br/><br/>"
	"</div><footer class='footer'><div style='padding:16px'>"
	"<a href='https://robonomics.network/' target='_blank' rel='noreferrer' style='color:#fff;'>© Robonomics Network</a>&nbsp;&nbsp;(<a href='https://github.com/airalab/altruist-firmware/issues' target='_blank' rel='noreferrer' style='color:#fff;'>" INTL_REPORT_ISSUE "</a>)<br/><span class='footer-polkadot'>Secured by</span>"
	"</div></footer></body></html>\r\n";

const char WEB_PAGE_FOOTER[] PROGMEM = "<br/><br/>"
	"<a class='b home-btn' href='/'>" INTL_BACK_TO_HOME "</a><br/><br/><br/>"
	"</div><footer class='footer'><div style='padding:16px'>"
	"<a href='https://robonomics.network/' target='_blank' rel='noreferrer' style='color:#fff;'>© Robonomics Network</a>&nbsp;&nbsp;(<a href='https://github.com/airalab/altruist-firmware/issues' target='_blank' rel='noreferrer' style='color:#fff;'>" INTL_REPORT_ISSUE "</a>)<br/><span class='footer-polkadot'>Secured by</span>"
	"</div></footer></body></html>\r\n";

const char WEB_PAGE_ROOT_FOOTER[] PROGMEM = "<br/><br/>"
    "</div><footer class='footer'><div style='padding:16px'>"
    "<a href='https://robonomics.network/' target='_blank' rel='noreferrer' style='color:#fff;'>© Robonomics Network</a>&nbsp;&nbsp;(<a href='https://github.com/airalab/altruist-firmware/issues' target='_blank' rel='noreferrer' style='color:#fff;'>" INTL_REPORT_ISSUE "</a>)<br/><span class='footer-polkadot'>Secured by</span>"
    "</div></footer></body></html>\r\n";
#endif


#if defined(ALTRUIST_URBAN_C3_LITE)
const char WEB_ROOT_PAGE_CONTENT[] PROGMEM =
"<nav class='home-nav'>"
"<section class='nav-section'>"
"<h3 class='nav-section-title'>" INTL_NAV_MONITOR "</h3>"
"<div class='nav-grid'>"
"<a class='b' href='/values'>{t}</a>"
"<a class='b' href='/status'>{s}</a>"
"</div></section>"
"<section class='nav-section'>"
"<h3 class='nav-section-title'>" INTL_NAV_SETTINGS "</h3>"
"<div class='nav-grid'>"
"<a class='b' href='/config'>{conf}</a>"
"{screen}"
"<a class='b' href='/group'>{group}</a>"
"<a class='b b-secondary' href='/ota'>" INTL_OTA_UPDATE "</a>"
"</div></section>"
"<section class='nav-section nav-section--danger'>"
"<h3 class='nav-section-title'>" INTL_NAV_MAINTENANCE "</h3>"
"<div class='nav-grid'>"
"<a class='b b-secondary' href='/debug'>{debug}</a>"
"<a class='b danger' href='/restart'>{restart}</a>"
"<a class='b danger' href='/removeConfig'>" INTL_CONFIGURATION_DELETE "</a>"
"</div></section>"
"</nav>";
#else
const char WEB_ROOT_PAGE_CONTENT[] PROGMEM =
"<nav class='home-nav'>"
"<section class='nav-section'>"
"<h3 class='nav-section-title'>" INTL_NAV_MONITOR "</h3>"
"<div class='nav-grid'>"
"<a class='b' href='/values'>{t}</a>"
"<a class='b' href='/status'>{s}</a>"
"<a class='b b-secondary' href='https://sensors.social/' target='_blank' rel='noreferrer'>" INTL_ACTIVE_SENSORS_MAP "</a>"
"</div></section>"
"<section class='nav-section'>"
"<h3 class='nav-section-title'>" INTL_NAV_SETTINGS "</h3>"
"<div class='nav-grid'>"
"<a class='b' href='/config'>{conf}</a>"
"{screen}"
"<a class='b' href='/group'>{group}</a>"
"<a class='b b-secondary' href='/ota'>" INTL_OTA_UPDATE "</a>"
"</div></section>"
"<section class='nav-section nav-section--danger'>"
"<h3 class='nav-section-title'>" INTL_NAV_MAINTENANCE "</h3>"
"<div class='nav-grid'>"
"<a class='b b-secondary' href='/debug'>{debug}</a>"
"<a class='b danger' href='/restart'>{restart}</a>"
"<a class='b danger' href='/removeConfig'>" INTL_CONFIGURATION_DELETE "</a>"
"</div></section>"
"</nav>";
#endif

const char WEB_CONFIG_SCRIPT[] PROGMEM = "<script>\
function setSSID(ssid){document.getElementById('wlanssid').value=ssid.innerText||ssid.textContent;document.getElementById('wlanpwd').focus();}\
function load_wifi_list(){var x=new XMLHttpRequest();x.open('GET','/wifi');x.onload = function(){if (x.status === 200) {document.getElementById('wifilist').innerHTML = x.responseText;}};x.send();}\
</script>";

const char WEB_GUEST_WIZARD_SUBMIT_JS[] PROGMEM = "<script>document.addEventListener('submit',function(e){var f=e.target;if(!f.classList||!f.classList.contains('guest-wizard-form'))return;try{var x=new XMLHttpRequest();x.open('POST','/guest_setup_ack',false);x.send();}catch(err){}var btns=f.querySelectorAll('button[type=submit],input[type=submit]');for(var i=0;i<btns.length;i++){var b=btns[i];if(b.disabled)continue;b.disabled=true;b.classList.add('is-loading');}},true);</script>";

const char WEB_COPY_IP_JS[] PROGMEM = "<script>function copyText(){const e=document.querySelector('.guest-ip,.ip-address');if(!e)return;var t=e.innerText||e.textContent;if(navigator.clipboard)navigator.clipboard.writeText(t).then((function(){alert('Copied to clipboard')})).catch((function(){alert('Failed to copy text')}));else{const o=document.createElement('textarea');o.value=t,document.body.appendChild(o),o.select(),document.execCommand('copy'),document.body.removeChild(o),alert('Copied to clipboard (fallback)')}}</script>";

const char WEB_GUEST_CONNECT_STATUS[] PROGMEM =
	"<div class='guest-page guest-page--connecting' id='guest-connecting'>"
	"<div class='guest-card guest-card--connect'>"
	"<div class='guest__connect-status guest__connect-status--initial'>"
	"<div class='loader' aria-hidden='true'></div>"
	"<h2 class='guest__connect-subtitle'>" INTL_GUEST_CONNECTING "</h2>"
	"<p class='form-hint guest__connect-hint'>" INTL_GUEST_CONNECTING_HINT "</p>"
	"</div></div></div>";

const char WEB_REMOVE_CONFIG_CONTENT[] PROGMEM =
"<div class='confirm-action'>"
"<form method='POST' action='/removeConfig' class='confirm-action__form js-delete-config'>"
"<div class='confirm-action__options radio-list'>"
"<label class='guest-option' for='allConfig'>"
"<input type='radio' id='allConfig' name='configType' value='all' checked>"
"<span><strong>" INTL_DELETE_CONFIG_ALL "</strong>"
"<span class='dash-row__desc'>" INTL_DELETE_CONFIG_ALL_DESC "</span></span></label>"
"<label class='guest-option' for='wifiConfig'>"
"<input type='radio' id='wifiConfig' name='configType' value='wifi'>"
"<span><strong>" INTL_DELETE_CONFIG_WIFI "</strong>"
"<span class='dash-row__desc'>" INTL_DELETE_CONFIG_WIFI_DESC "</span></span></label>"
"</div>"
"<div class='confirm-action__step' data-delete-step='ask'>"
"<div class='confirm-action__buttons'>"
"<button type='button' class='confirm-btn confirm-btn--danger js-delete-ask'>" INTL_DELETE "</button>"
"<a class='confirm-btn confirm-btn--cancel' href='/'>" INTL_CANCEL "</a>"
"</div></div>"
"<div class='confirm-action__step' data-delete-step='confirm' hidden>"
"<div class='ui-notice ui-notice--err' role='alert'>"
"<strong>" INTL_CONFIGURATION_REALLY_DELETE "</strong>"
"<p class='dash-row__desc'>" INTL_CONFIGURATION_DELETE_WARNING "</p></div>"
"<div class='confirm-action__buttons'>"
"<button type='submit' class='confirm-btn confirm-btn--danger' name='submit'>" INTL_CONFIGURATION_DELETE_CONFIRM "</button>"
"<button type='button' class='confirm-btn confirm-btn--cancel js-delete-cancel'>" INTL_CANCEL "</button>"
"</div></div></form></div>";

const char WEB_RESET_CONTENT[] PROGMEM =
"<div class='confirm-action'>"
"<p class='confirm-action__question'>" INTL_REALLY_RESTART_SENSOR "</p>"
"<form method='POST' action='/restart' class='confirm-action__form'>"
"<div class='confirm-action__buttons'>"
"<button type='submit' class='confirm-btn confirm-btn--danger' name='submit'>" INTL_RESTART "</button>"
"<a class='confirm-btn confirm-btn--cancel' href='/'>" INTL_CANCEL "</a>"
"</div></form></div>";


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
const char WEB_HTTPS[] PROGMEM = "HTTPS";

#endif // _HTML_CONTENT_H
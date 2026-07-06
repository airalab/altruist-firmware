#include "pages.h"
#include "../../intl.h"
#include "../html-content.h"
#include "../../config_manager/config_helpers.h"

void webserver_root(String &page_content, const String &robonomics_address) {

    // Enable Pagination
    page_content += FPSTR(WEB_ROOT_PAGE_CONTENT);
    page_content.replace(F("{t}"), FPSTR(INTL_CURRENT_DATA));
    page_content.replace(F("{s}"), FPSTR(INTL_DEVICE_STATUS));
    page_content.replace(F("{conf}"), FPSTR(INTL_CONFIGURATION));
#ifdef ALTRUIST_INSIGHT
    page_content.replace(F("{screen}"),
        String(F("<a class='b' href='/screen'>")) + FPSTR(INTL_SCREEN_MENU) + F("</a>"));
#else
    page_content.replace(F("{screen}"), F(""));
#endif
    page_content.replace(F("{group}"), FPSTR(INTL_GROUP_MENU));
    page_content.replace(F("{restart}"), FPSTR(INTL_RESTART_SENSOR));
    page_content.replace(F("{debug}"), FPSTR(INTL_DEBUG_LEVEL));

    const char* sensor_ss58 = (robonomics_address.length() > 0) ? robonomics_address.c_str() : nullptr;
    const String map_url = buildSensorsSocialMapUrl(sensor_ss58);
    page_content.replace(F("https://sensors.social/"), map_url);
}
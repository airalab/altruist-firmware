#ifndef WEB_HEADER_LOGO_SELECT_H
#define WEB_HEADER_LOGO_SELECT_H

// Picks the SVG served at `/{INTL_LANG}_s?r=logo` in the page header (device-specific).
// Favicon: icons/favicon-black.png + icons/favicon-white.png → webserver/favicon.h

#if defined(ALTRUIST_INSIGHT)
#include "web-header-logo-insight.h"
#elif defined(ALTRUIST_URBAN)
#include "web-header-logo-urban.h"
#else
#error "Define ALTRUIST_INSIGHT or ALTRUIST_URBAN for the web UI header logo."
#endif

#endif

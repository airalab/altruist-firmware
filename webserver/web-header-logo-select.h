#ifndef WEB_HEADER_LOGO_SELECT_H
#define WEB_HEADER_LOGO_SELECT_H

// Picks the SVG served at `/{INTL_LANG}_s1.4?r=logo` and as `/favicon.ico` (device-specific).
// Source art: `display/icons/svg/altruist-Insight-logo.svg` / `altruist-urban-logo.svg` — edit there, then refresh the matching `web-header-logo-*.h` body.

#if defined(ALTRUIST_INSIDE)
#include "web-header-logo-insight.h"
#elif defined(ALTRUIST_URBAN)
#include "web-header-logo-urban.h"
#else
#error "Define ALTRUIST_INSIDE or ALTRUIST_URBAN for the web UI header logo."
#endif

#endif

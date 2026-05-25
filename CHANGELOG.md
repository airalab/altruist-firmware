# Changelog

All notable changes to the Altruist Firmware project will be documented in this file.

## [R_2026-05.01](https://github.com/airalab/altruist-firmware/releases/tag/v_R_2026-05.01) — 2026-05-25

### Improvements

- **Limited ZMOD4510 sensor info wait time** — reduced ZMOD4510 read_sensor_info timeout from 200s to 10s to avoid blocking Urban boot when the sensor stays busy or does not respond.
- **Urban LED states** — simplified Urban LED behavior: steady green for normal operation, blue for configuration mode and active datalog sending, 3-second green/red result after datalog transmission, and steady red only after sustained Wi-Fi/API errors.

### Bug Fixes

- **Renamed Robonomics datalog keys for CO and CO2** :
  - co2 -> co2
  - co -> co
- **SDS011: no placeholder PM values** — stopped publishing `P1`/`P2` with `-1` before the first valid SDS011 measurement; PM values are now written only after a valid averaging window.
- **SDS011 runtime recovery** — added detection of consecutive empty SDS011 measurement windows and UART/protocol recovery without reboot (drain UART, reinitialize Serial1 on ESP32, reset parser state, and restart the measurement cycle).
- **Urban SDS cache cleanup on Insight** — Insight now removes stale Urban `SDS_P1`/`SDS_P2` values from its local cache if Urban no longer includes them in `/data.json`, so UI and payloads do not keep outdated PM values.
- **Telemetry payload safety** — API sends now use a JSON snapshot copied under mutex, and sensor JSON overflow is logged after fetches to help diagnose disappearing fields.

## [R_2026-05](https://github.com/airalab/altruist-firmware/releases/tag/v_R_2026-05) — 2026-05-15

### Features

- **STANDALONE INSIGHT** - Insight can now operate as a fully standalone device, no Urban connection required.

### Bug Fixes

- **Wi-Fi recovery stability (Urban & Insight)** — improved STA reconnect behavior after router/Wi-Fi outages to prevent stuck reconnect states and unnecessary reboots.
- **STA connection state detection** — fixed cases where Wi-Fi could be incorrectly treated as disconnected during network recovery.
- **Web UI recovery after reconnect** — improved HTTP/Web UI listener restart after STA IP changes.
- **Insight ↔ Urban communication recovery** — improved HTTP/mDNS reconnection flow after Wi-Fi interruptions and increased timeout tolerance for slow LAN networks.

### Improvements

- **Wi-Fi reconnect behavior (ESP32 STA)** — improved recovery handling and service refresh after reconnect or DHCP renewal.
- **STA connection tuning** — optimized Wi-Fi join flow and reduced reconnect latency during device startup.
- **Urban** - removed SDS011 publishing when PM values are -1.
- **Insight** - Removed gas heater since the device do not use its measurements.
- **Web UI header** — `/{lang}_s1.4?r=logo` serves **device-specific SVG** logos; **`/favicon.ico`** uses the **colored Robonomics PNG** for tab visibility. Web canvas `<h3>` uses **`PM_SENSOR_NAME`** (**“Altruist Insight”** / **“Altruist Urban”**). With **Insight standalone**, **Urban pairing** fields on the config GPS tab are hidden and the standalone hint is shown.
- **Sleep Analytics** - default analytics time changed from 22:00–10:00 to 22:00–07:00 local time. Added support for custom hours and minutes (e.g. 07:15).

## [R_2026-04.5](https://github.com/airalab/altruist-firmware/releases/tag/v_R_2026-04.5) — 2026-04-27

### Bug Fixes

- **Insight LEDs: segment order aligned with main screen** — updated LED segment mapping to match the main screen metric order (Noise → PM → CO₂ → Temp → Hum → Pressure) after UI column/layout changes. Reduced frequency of “forced white” fallback.

## [R_2026-04.4](https://github.com/airalab/altruist-firmware/releases/tag/v_R_2026-04.4) — 2026-04-27

### Bug Fixes

- **Urban “false offline” on Insight** — Urban connectivity TTL (`service_data.urban_last_ok_ms`) is now refreshed on every successful Urban HTTP `200 OK`, even if Urban `data.json` fails to parse, so UI/LEDs only show `offline` when Urban is actually unreachable.
- **Urban TTL label recovery (Insight main screen)** — fixed a case where the `offline/stale` label could remain visible after Urban connectivity was restored by always resetting `urban_ttl_state` back to “online” when TTL is fresh.
- **Night Analytics: humidity calculation corrected (Insight)** — fixed an aggregation typo that could show unrealistically low humidity (e.g. `8%`) by summing the actual hourly humidity values.

### New

- **ZMOD4510 gas sensor support (Urban)** — added support for Renesas ZMOD4510 (O₃ / NO₂) sensor, including initialization, measurement flow, status handling (warmup/valid/error), AQI metrics (FAST_AQI, EPA_AQI), temperature/humidity compensation, and exposure in local JSON/UI pipeline.

## [R_2026-04.3](https://github.com/airalab/altruist-firmware/releases/tag/v_R_2026-04.3) — 2026-04-22

### Bug Fixes

- **Insight (ESP32-C6): prevent reboot loop during time sync** — replaced time setup path with a safe SNTP init to avoid lwIP asserts during boot (e.g. `udp_new_ip_type`, `tcpip_timeouts_mbox_fetch`, `sys_mutex_unlock`).
- **Robonomics Map: avoid send attempts before time is synced** — Map send is skipped until `getLocalTime()` is available, preventing empty signatures and reducing DNS/HTTP retry spam on networks without internet.

### Improvements

- **Main screen readability** — numeric value separator is rendered as a visible dot and its vertical position was adjusted for better visual centering on e-ink.
- **Urban disconnect handling (Insight)** — added TTL-based stale/offline detection for Urban data so Insight UI/LEDs stop showing outdated Urban readings after Urban is powered off.

## [R_2026-04.2](https://github.com/airalab/altruist-firmware/releases/tag/v_R_2026-04.2) — 2026-04-15

### Improvements

- **Publish to Map: additional Urban sensors** — added optional toggles for Radiation, O3, NO2, FAST AQI, and EPA AQI; shown under a dedicated “Additional sensors (optional)” section (off by default).

## [R_2026-04.1](https://github.com/airalab/altruist-firmware/releases/tag/v_R_2026-04.1) — 2026-04-09

### Bug Fixes

- **Robonomics RPC custom endpoints (mirrors) made reliable** — accept `host` or full URL in config, normalize to `https://.../rpc/`, and retry once on HTTP redirects (301/302/307/308) to avoid failures on mirror endpoints.

### Improvements

- **Robonomics Map connectivity hosts are now configurable** — added config options to use a pinned host or a custom host pool (with the same health checks as the default pool) instead of the built-in `connectivity.robonomics.network` list.
- **Analytics (Insight) clarity & layout polish** — analytics metric cards now explicitly indicate values are the _night average_ via an in-card label (`night avg`) and improved vertical spacing for better readability.
- **Main screen (Insight) source clarity** — Urban-only metrics (Noise, PM) are labeled as `Urban only` to avoid confusion when viewing combined Urban/Insight data.

## [R_2026-04](https://github.com/airalab/altruist-firmware/releases/tag/v_R_2026-04) — 2026-04-01

### Bug Fixes

- **Robonomics on-chain datalog compatibility with runtime specVersion=42** — fixed extrinsic encoding to match current Robonomics `TxExtension` (removed unsupported subscription extension; adjusted signed extra/payload layout) so `author_submitExtrinsic` no longer fails with `code:1002 wasm unreachable` for valid calls.
- **RWS fee-less path restored via `rws.call`** — fixed SCALE encoding for `rws.call(owner, Box<Call>)` parameters to prevent runtime decode traps (`code:1002`) and allow fee-less subscription execution when the device is linked to the owner.
- **On-chain error reporting** — improved detection of JSON-RPC error objects returned by `author_submitExtrinsic` so failures like `code:1002` / `code:1010` are not misreported as success.

### Improvements

- **Main screen UX redesign (Insight)** — rework of the main interface to improve usability and readability on e-ink.
- **Typography tuning for key metrics (Insight main)** — increased key text sizes where layout permits and tightened label/value spacing for faster scanning.
- **Main metrics layout restructuring** — refined left/right metric grouping and alignment for a cleaner, more consistent visual hierarchy.
- **Warning indicators refinement (main screen)** — adjusted warning icon positioning and visibility logic to improve legibility near labels.
- **Icon asset cleanup** — removed unused generated icon headers (including unused `30x30`, most `35x35`/`40x40`, and stale helper headers) to reduce asset clutter and maintenance overhead.
- **Main header and top-strip polish** — title updated to `URBAN/INSIGHT`; WiFi and source icons (`urban`/`insight`) were resized/repositioned for cleaner spacing and better visual hierarchy.
- **Footer redesign with source grouping** — bottom status text now groups warnings by source (`Urban:` / `Insight:`), includes dew point in Urban context, and supports wrapped multi-line layout with icon-led footer entry.
- **Footer info icon integration** — added `info.svg` conversion pipeline and replaced text label with dedicated monochrome icon on main screen.
- **Sidebar visual cleanup** — removed the outer bottom sidebar border line while preserving internal navigation separator lines.
- **Insight LED mapping aligned to main screen order** — LED segments now follow displayed measurement order with pressure at the end; noise and PM remain split (`avg/max`, `PM10/PM2.5`) for clearer diagnostics.
- **Insight LED transition behavior softened** — replaced abrupt color-change blink with subtle short dim pulse on changed segments only, reducing visual annoyance.
- **Sensors Map screen redesign and stabilization (Insight)** — rebuilt layout to promo-style composition (new title/subtitle hierarchy, centered QR, `SENSORS.SOCIAL` footer), tightened header/sidebar alignment with other screens.
- **Night Analytics wording and hierarchy update** — renamed `Conservative score` to `Total score`, updated secondary labels (`general` / `biohacking`, with RU variants), and adjusted vertical ordering/spacing for clearer score reading on the circle panel.
- **Urban hardware reset + dual-LED indication (ESP32-C6)** — added GPIO7 long-press reset (hold >10s, release to confirm) that clears only Wi-Fi + Web UI credentials while preserving Robonomics identity; enabled 2-pixel addressable LED status (steady state) + activity (error-only, rate-limited pulse) indications for a calmer UX.

### Build & tooling

- **Font generator split by script** — `display/fontgen/ttf_to_bitmap.py` can now generate ASCII and Cyrillic glyph fonts from different TTFs (default: Orbitron for ASCII, `font.ttf` for Cyrillic), improving RU support with stylized Latin fonts.
- **New glyph font sizes** — added wiring for `font_10_*` and `font_22_*` in `fonts.h` for UI use.

---

## [R_2026-03](https://github.com/airalab/altruist-firmware/releases/tag/v_R_2026-03) — 2026-03-16

### New Features

- **Night-only Analytics screen (Insight/e-ink)** — redesigned Analytics into a single smartwatch-style nightly recovery page with a large central score ring and compact metrics table.

### Improvements

- **Night score methodology table** — added side-by-side methodology scores with on-screen legend (`C = Conservative`, `B = Biohacking`) to make score interpretation explicit.
- **Analytics storage and rendering path** — analytics now uses night-focused 24h/hourly data flow only.
- **Analytics NVS-first mode** — analytics persistence moved to a compact 48-hour hourly NVS ring, and night rendering now reads NVS/RAM history directly.
- **Storage simplification for Analytics** — removed analytics SD cache/migration dependencies from runtime; SD is no longer required to open Analytics screen.
- **Analytics DEV observability** — added DEV logs for persistence state, category/metric coverage, and save telemetry (first/last save time, save count, last reason, save policy).
- **Night scoring methodology update** — switched Conservative/Biohacking scoring to sleep-impact formulas per metric (CO2, PM2.5, noise, temperature, humidity) with score mapping `100 + impact*2`.
- **Night score aggregation update** — final Conservative/Biohacking scores now use summed model impact (`100 + total_impact*2`) to match the strict methodology examples.
- **Night summary line refresh** — replaced generic footer with a single-line human-readable summary for both models (`score + grade + sleep impact`).
- **Night data readiness state** — when not enough night hours are collected, Analytics shows explicit "collecting data" status (no table box) instead of a misleading score.
- **Night analytics qr code** - added qr code with sensor map for more thorough and proper analytics.
- **Background analytics ingest** — hourly analytics ingestion was decoupled from Analytics screen rendering and now runs in the sensor worker, so night history keeps updating even when Analytics screen is not opened.
- **Hourly persistence policy** — persistence now targets hourly boundaries with catch-up behavior after reboot/missed windows, improving next-morning report reliability.
- **Morning analytics recompute window** — night report selection now supports progressive hourly refresh between `06:00` and configured night end hour (default `10:00`) instead of waiting for a single daily switch.
- **Analytics screen priority window (Insight)** — analytics can auto-open as default between `06:00` and `12:00`, and auto-return to `MAIN` after the window ends while preserving manual navigation.
- **Typography and units upgrade (e-ink analytics/main)** — added glyph support for `°`, `µ`, and superscripts; updated UI units to full forms (`°C`, `µg/m³`).
- **LED resilience under mutex contention (Insight)** — added LED mutex diagnostics and a guarded daytime fallback that forces neutral LED ON state when updates are blocked too long.
- **Configurable LED night schedule (Insight)** — added web-configurable `LED off hour` / `LED on hour` (defaults `00` and `06`) and switched logic from hardcoded quiet hours.
- **Main screen lock-time reduction (Insight)** — removed serialize/deserialize JSON path from display refresh; main screen now uses typed cached snapshot extracted under a short mutex hold.
- **Main screen UX redesign (Insight, WIP)** — ongoing iterative rework of the main interface to improve clarity and usability: increased key font sizes where layout allows, improved e-ink readability (icons/spacing/alignment), and simplified, more structured metric presentation.
- **Network/API hardening on unstable WiFi** — guarded API send paths and skip send cycle when WiFi remains disconnected after reconnect attempt.
- **Crash diagnostics in status page** — added reset reason code, last crash section, previous uptime, and previous free heap.

### Bug Fixes

- **SD rollup path handling** — fixed CSV open paths in rollup builders to always use absolute paths (`/...`) and avoid VFS `does not start with /` errors.
- **SD-card robustness and retention** — extended retention worker and SD mutex usage for logging/cleanup/read paths; retention now covers raw graph files, analytics daily rollups for dev, and boot diagnostics.
- **CSV logging regression after analytics refactor** — fixed one-shot `jsonUpdated()` flag consumption so SD CSV writes and analytics ingest can run together without dropping graph data.

## [R_2026-02.1](https://github.com/airalab/altruist-firmware/releases/tag/R_2026-02.1) — 2026-02-20

### Bug Fixes

- **Fix e-paper display freeze** — display could get permanently stuck when the EPD busy pin failed to release; the driver now detects the timeout and triggers immediate hardware reset + full refresh recovery
- **Reduce EPD busy timeout** — decreased from 30s to 10s for faster stuck-state detection (normal operations complete in 2–4s)
- **Extend EPD watchdog to all screens** — the periodic 25-minute recovery watchdog now runs on all navigable screens (GRAPHS, SENSOR_MAP, SETTINGS), not only MAIN

---

## [R_2026-02](https://github.com/airalab/altruist-firmware/releases/tag/R_2026-02) — 2026-02-20

### New Features

- **Altruist Insight (Inside) support** — full support for the new Altruist Insight board variant with ESP32-C6, including e-paper display, SD card logging, LED strip, and button controls
- **Publish to Map** — new configurable data sharing feature allowing users to select which sensor values (temperature, humidity, pressure, CO2, PM, noise) are published to the public sensors map
- **Russian language support** — full Russian localization for firmware UI, display screens, and web interface
- **Language switch** — ability to choose between English and Russian firmware builds
- **New display screens and navigation** — completely redesigned e-paper display with Main, Graphs, Sensor Map, and Settings screens; new icon set and right-side navigation stack
- **SD card data graphs** — graph screen with historical sensor data read from SD card CSV files, supporting 12-hour rolling window with today + yesterday data

### Improvements

- **Redesigned main screen layout** — updated Urban 2-column layout with QR code, device IP, and Robonomics addresses
- **Improved buttons logic** — short press and long press actions for UP, SET, DOWN buttons; long DOWN press enters sleep mode
- **LED enhancements** — extended LED indicators for more sensor values; time-based night dimming schedule (22:00–06:00); updated noise LED ranges
- **Partial e-paper display refresh** — 10 partial refreshes between full refreshes for faster screen updates; EPD watchdog re-init every 25 minutes
- **WiFi AP password protection** — access point mode now requires a password (default: `123456789`) instead of being open
- **Improved OTA updates** — two-stage OTA with dual download hosts, progress percentage display, MD5 verification, and retry logic
- **Separated dev/prod logging** — different log levels for development and production firmware builds
- **Custom Altruist address in config** — ability to set a custom Altruist Urban device address

### Bug Fixes

- Fix memory leak on display refresh (#52)
- Fix negative temperature display
- Fix "lost Urban for Insight" bug
- Fix WiFi connectivity issue
- Fix QR code rendering
- Fix SD card data display and screen switch updates

### Build & Infrastructure

- Build configurations for all ESP32-C6 variants: Urban EN/RU, Insight EN/RU (plus dev builds)
- Updated pin mappings for Insight boards
- DEV postfix added to development firmware filenames

---

## [R_2025-06](https://github.com/airalab/altruist-firmware/releases/tag/R_2025-06) — 2025-06-23

- Fix chipid
- Add Restart Reason to device status page
- Handle Webserver in separate task
- Manually reconnect to WiFi
- Add Custom API

---

## [R_2025-04](https://github.com/airalab/altruist-firmware/releases/tag/R_2025-04) — 2025-04-10

- Fix mobile UI styles

---

## [R_2025-03](https://github.com/airalab/altruist-firmware/releases/tag/R_2025-03) — 2025-03-26

- Add possibility to remove only wifi configuration
- Show new sensor IP address after first configuration
- Add local domain to the config
- Add SCD4x Sensor
- Add RadSens Sensor
- Show location on the map

---

## [R_2025-02.2](https://github.com/airalab/altruist-firmware/releases/tag/R_2025-02.2) — 2025-02-26

- Fix noise calculations
- Change web server footer
- Fix private key saving

---

## [R_2025-02.1](https://github.com/airalab/altruist-firmware/releases/tag/R_2025-02.1) — 2025-02-24

- Automatically choose connected sensors
- Code refactoring
- Classes for sensors and APIs
- Change Web interface

---

## [R_2025-02](https://github.com/airalab/altruist-firmware/releases/tag/R_2025-02) — 2025-02-12

- Robonomics public node hostname can be set in config
- Changes for Home Assistant discovery service
- Independent timeout for Datalog sending
- Fix BME280 I2C sensor
- Signed requests to Sensors Connectivity

---

## [R_2025-01](https://github.com/airalab/altruist-firmware/releases/tag/R_2025-01) — 2025-01-13

- Split GPS fields in config
- Add auto update for ESP32
- Build firmware for ESP32-C3
- Generate and save private key for Robonomics Account
- Send Robonomics Datalog Extrinsic with sensors data

---

## [2024.09](https://github.com/airalab/altruist-firmware/releases/tag/2024.09) — 2024-09-18

- Send noise every 5 seconds

---

## [R_2024-06](https://github.com/airalab/altruist-firmware/releases/tag/R_2024-06) — 2024-06-13

- Added Noise meter sensor PCBA

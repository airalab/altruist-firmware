# Diagnostic profiles and USB acceptance

L2а source baseline: `344f1239a395df83ec00c5be3d73a455e615990a`,
2026-11-06. Changes are local source changes; no firmware build, flash, or
hardware acceptance was performed. Stable/Testing select publication identity;
`ALTRUIST_BUILD_DEBUG` selects diagnostics. A successful debug run does not
validate release behavior.

## Supported matrix

Urban: ESP32-C3 and ESP32-C6; Insight: ESP32-C6 (`inside` environment names are
compatibility names). Each has EN/RU release and `_debug` environments: 12
PlatformIO targets. C3 debug retains size optimization and has no protobuf;
its metadata now reports `profile=debug` despite PlatformIO's release build type.
Experimental radiation firmware is outside this baseline and acceptance scope.

## Inventory and policy

| Path | Before L2а | Current source policy / tester use | Functional boundary |
| --- | --- | --- | --- |
| BUILD, LOG in setup | Direct Serial in all profiles | Debug only; build identity and effective levels | Config loading, registration repair, LoRa setup remain active |
| BOOT in `logBootStatus` | Direct Serial in all profiles | Debug only; reset context | Metric initialization and crash-context lifecycle remain active |
| HEALTH in `logMetrics` | Health flag; metadata enabled it by default in release too | Debug AND health flag; default on in debug, off in release | Metrics worker still updates uptime and crash breadcrumbs in release |
| PAYLOAD in message formatter | Direct Serial; plaintext samples in release when unencrypted | Debug only; legacy compact samples and encrypted plaintext diagnostics | Formatting, encryption, signing, payload limits and send results unchanged |
| DATALOG / CONNECTIVITY | Direct Serial in all profiles | Debug only; attempts/results, aggregate route evidence | RPC/HTTP handling, counters and error paths remain active |
| SUBSYSTEM / project error/info/verbose | SUBSYSTEM direct; other logs runtime/forced level | Debug only; release effective level is zero | Recovery, watchdog, error returns and resource cleanup stay outside guards |
| PROTO | Debug metadata via verbose helper | Remains debug only; not sensor readings or delivery proof | Envelope creation and transport remain active in release |
| Sensor JSON | Already debug-only in all reviewed drivers | Retained; BME680/SCD4x snapshots now end with newline | Measurement, calibration, VOC persistence and API JSON unchanged |
| Direct BMx280/I2S/EPD/LoRa prints | Mixed guards, including unguarded errors | Debug only; BME680_DEBUG additionally requires debug | I2C results, I2S calls, display recovery, handshake and LoRa frames preserved |
| Waveshare `Debug` macro | USE_DEBUG=1 | Requires debug | Display control preserved |
| `LoggingSerial` queue, UART log writes, web `/serial` | Both profiles | No diagnostic queue/writes in release; `/serial` returns 204 | Structured LoRa writes bypass this diagnostic path and remain active |
| `/debug`, embedded advanced logs, crash breadcrumb rows | Both profiles | Debug controls/logs/crash detail require debug; release explains availability | Config, backup, device/owner export, ordinary status and sensor API preserved |
| SD `exceptions/boot_*.txt` | Insight in both profiles | Created only in debug | Context consumption preserved after successful mount |
| SD `exceptions/runtime.log*` | Already debug-only | Remains debug-only | Existing files are not erased; existing retention stays active |
| SD `/data` CSV, analytics, VOC SPIFFS | Functional storage | Both profiles, unchanged | Not diagnostic logs |
| ESPRobonomicsClient direct Serial | Unguarded library prints | Build middleware force-includes a silent Print sink only in that library's C++ sources for release | Dependency files/version unchanged; application Serial and Improv untouched |
| libc printf/puts | USB wrapper only in C6 Urban | All targets wrap known library emitters; release discards, debug uses USB | No transport payload is sent through these console functions |
| Arduino/core and project ESP_LOG | CORE_DEBUG_LEVEL=0 by default in release | Forced policy header clamps CORE_DEBUG_LEVEL and LOG_LOCAL_LEVEL to zero even if flags request 5 | Prebuilt SDK output handled separately below |
| ROM/IDF/libc console after setup | Redirected only when LoRa owns UART0 | Release installs existing console hooks after USB begin, discards console output | Improv protocol still uses real USB Serial.write |

`ALTRUIST_HEALTH_TELEMETRY=0` disables health output in debug. Setting it to 1
in release does not enable it. Runtime `cfg::debug`, forced log levels, channel
and web query parameters cannot enable release project diagnostics.

The ESPRobonomicsClient middleware is scoped by its installed library directory
name and `.cpp` suffix. The local cached library was inspected, including
Robonomics, PayloadParamsUtils, JsonUtils and HTTPRequests. A dependency layout
or logging API change requires another audit. Cached dependencies are evidence
of available sources, not a lock or a freshly resolved build.

## Tester data availability

| Input / route | Debug USB data | Release USB data |
| --- | --- | --- |
| Urban sensor fetch | BME280 climate/pressure, SDS011 PM, I2S noise JSON | No project diagnostic readings |
| Insight sensor fetch | SCD4x CO2/climate; BME680 climate/pressure, VOC index/level/count, gas resistance/baseline JSON | No project diagnostic readings |
| Legacy plain | Local JSON plus compact PAYLOAD when formatted | None |
| Legacy encrypted / datalog encrypted | Local JSON; debug plaintext sample when legacy formatter runs | None |
| Successful C6 protobuf | Local sensor JSON before upload selection; no requirement for legacy PAYLOAD | None |
| Uploads disabled / unavailable | Local JSON when the sensor returns data | None |

These are source-level paths, not observed device results. JSON does not yet
carry a dedicated freshness/sequence contract. Cross-source deduplication,
late attachment, sensor read failures and stale cached samples remain L3 work.
Route attribution remains L4; build repetition, sensor inventory and positive
SD-ready/write evidence remain L7. No new tag/schema was added in L2а.

## Agreed system boundary and remaining checks

The user approved on 2026-11-06 that ROM/bootloader output, startup before
application hooks, and panic output from the precompiled ESP-IDF are system
limitations. Suppressing all of those requires separate SDK/bootloader work.
Normal project diagnostics and known dependency prints are not exceptions.
Functional Improv frames, LoRa frames, UI/API and working SD data are preserved.

Run `python3 tests/diagnostics/check_profiles.py -q` for static policy checks
(requires host `cpp`). It preprocesses source with platform includes omitted;
it does not compile C++, link SDK libraries, invoke PlatformIO, or create a
firmware artifact. It checks 12 metadata targets × both channels × health 0/1,
release guards under requested maximum verbosity, core level clamping,
debug JSON/tags, and retained watchdog/metrics/Improv/LoRa paths.

Still required from the operator: build all 12 environments for both channels;
check release with custom maximum log flags and runtime level 5, and verify
the dependency middleware is applied in actual compiler commands. On Urban
and Insight, capture debug/release boot and runtime USB logs; exercise plain,
encrypted and successful C6 protobuf uploads, sensor errors, Wi-Fi recovery,
watchdog, SD CSV/analytics, Improv and LoRa where available. Check debug SD logs
and absence of new release diagnostic files. Keep full captures and exact
revision/flags. M2 additionally needs an Insight VOC run longer than one hour.

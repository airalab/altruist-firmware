# Diagnostic profiles and operational support logs

Current source baseline: `7e4189a` on `17ab154`; policy revised by the user
2026-11-09. The original L2а changes on `344f123` disabled all project release
logs. The user subsequently approved a bounded operational release log for
local web support and the webflasher USB console. Detailed sensor acceptance
remains debug-only. Stable/Testing are publication channels, release/debug are
build profiles; no third profile is introduced.

These source changes have not been compiled, flashed or checked on hardware.
The earlier 15-minute captures of `48ab87d` validate the previous silent-release
policy only. They are not validation of this revision.

## Profiles and inventory

Urban C3/C6 and Insight C6 (`inside` target names), EN/RU, release/debug: 12
targets. C3 debug remains size-optimized and reports profile=debug. Existing
publication workflows build release targets for both channels; the webflasher
therefore receives operational USB logs without needing debug firmware.

| Path | Release | Debug / boundary |
| --- | --- | --- |
| BUILD identity | Startup after config load and setup, or explicit status request | Existing startup BUILD plus optional support snapshot |
| BOOT / HEALTH | No detailed BOOT/periodic HEALTH; reset reason and counters in requested LOG status | Existing machine-readable tags |
| PAYLOAD / sensor JSON / PROTO | None | Existing local readings and formatter/protocol diagnostics |
| Project debug_out/info/error/verbose | Still compiled as no-ops | Existing detailed project messages and runtime/forced levels |
| Curated operational events | LOG timestamp, error/event level, subsystem, fixed event name, numeric code | Curated events plus existing detailed messages |
| SUBSYSTEM | Only subsystem/reason names through operational LOG, without details | Existing SUBSYSTEM event with details |
| Connectivity / datalog | Attempt, result, numeric HTTP/RPC code; no host/body/owner/payload | Existing route details and upload tags |
| Wi-Fi | Connected/disconnected and existing recovery reasons | Existing detailed network diagnostics |
| Sensor initialization | Internal sensor names with ready/unavailable; explicit standard sensor read/init errors | Existing detailed driver diagnostics |
| OTA / config / SD / display | Existing error hooks without paths/details; OTA started/verified, SD-ready | Existing detailed logs |
| Watchdog | Curated restart event and numeric section | Existing detailed restart reason; watchdog actions unchanged |
| Web /serial and log pages | Same configured authentication as /debug; shared RAM log tail and levels 0/1/3 | Same interface, level 5 also available |
| USB | Curated operational lines through actual Serial.write | Existing detailed USB output |
| UART0 / LoRa | Support log never writes HardwareSerial; functional structured writes remain active | Existing structured LoRa protection |
| Improv | Binary protocol preserved; USB support lines muted by debugSetUsbQuiet | Existing provisioning behavior |
| SD exceptions boot/runtime files | Not created; old files not automatically erased | Existing debug-only files |
| SD sensor CSV, analytics, VOC SPIFFS | Preserved | Functional storage, not diagnostic logs |
| Core/library/libc/ROM hooks | Existing release clamp and silent hooks retained | Existing diagnostic output |

## Operational vocabulary and levels

Automatic release logging follows the saved cfg::debug, capped at 3. Forced
project levels do not override that cap. Default is 3. Level 0 is off, 1 is
errors, 3 adds events. Legacy level 2 also admits errors; 4/5 are capped at 3.
A web log-page level selection changes RAM state until reboot, and reports the
effective level. Permanent changes use the existing configuration form. Debug
retains its forced level behavior; selecting 0 does not override a debug force.

The application does not re-enable all INFO/ERROR messages: their existing
content can include transport internals. A separate small event emitter accepts
curated subsystem/event names and an integer code. Existing subsystem hooks pass
only constant names to it; details (paths, URLs, bodies) stay debug-only. Numeric
codes are part of the throttle key so a protobuf 403 does not suppress a legacy
500. Other messages include sensor availability/read failures, Wi-Fi transitions,
OTA lifecycle and readiness. Logs describe firmware observations, not independent
proof of delivery. `unavailable` may describe an optional uninstalled sensor.

Examples:

```text
[BUILD] version=R-URB_2026-11-testing+abcdef0 channel=testing commit=abcdef0 model=urban target=esp32c6 language=en profile=release
[LOG] uptime=82 level=error subsystem=connectivity event=send_failed code=403
[LOG] uptime=85 level=event subsystem=wifi event=connected code=0
[LOG] status uptime=100 boot=3 reset_reason=power_on_reset heap=200000 wifi=1 wifi_errors=0 sensor_errors=0 sd_errors=0
```

Status snapshot on /debug, /advanced or /serial?status=1 explicitly emits BUILD
and LOG status to the web queue and USB, even with automatic logging off. It
includes no measured values, identifiers/credentials from network configuration,
transport body or signature. wifi=1 means station association, not verified IP
readiness. Periodic HEALTH and acceptance sensor readings are not introduced in
release. Ordinary /serial polling does not generate status records.

Release queue: fixed 2048 bytes in RAM; full old records are evicted before
inserting support lines. Queue writes never wait for queue space; a slow/absent browser
cannot accumulate unbounded memory. Debug uses its existing 511-byte capacity;
its queue writes also no longer wait for a reader. A mutex coordinates writers
and draining/eviction. USB output uses the real USB Serial, without forwarding
support lines to UART0. Snapshot and curated events share the same writer.

Repeated automatic events are suppressed for 30 seconds per subsystem/event/code
using 16 recent keys protected by a critical section. Older keys may be evicted
when more than 16 distinct combinations occur. This is diagnostic throttling,
not counters of failures or attempts. Explicit snapshots are not throttled.
Web polling destructively drains the shared queue: multiple browsers and the
debug SD consumer do not each receive an independent copy. Persistent storage
and multi-viewer replay are outside this change.

## Acceptance boundary and system limitations

Local debug JSON still carries standard Urban/Insight readings, including
BME680 VOC index/level/count and gas resistance/baseline, before uplink selection.
Successful protobuf skips legacy PAYLOAD but does not eliminate local JSON.
Freshness, deduplication and late attachment remain L3. Route attribution remains
L4, build repetition/inventory/positive SD contract remains L7. The ordinary USB
sensor tester may still fail on release because operational logs are not its
measurement/HEALTH contract. Debug PASS does not validate release.

The agreed system exceptions remain ROM/bootloader output, early startup before
application hooks and panic output from the precompiled ESP-IDF. Suppressing all
of those requires separate SDK/bootloader work. Functional Improv/LoRa/UI/API
and working SD data remain available. Experimental radiation firmware is outside
the tested standard target scope.

## Verification and operator protocol

`python3 tests/diagnostics/check_profiles.py -q` runs 9 static checks using host
cpp with platform includes omitted. They cover the 12 metadata targets × two
channels × health 0/1; maximum release verbosity flags; debug-only JSON/tags,
operational sink routing/snapshot fields, event redaction, web controls/auth,
queue/throttle guards and lifecycle hooks. This does not test C++ types, link
SDK libraries, instantiate the actual FreeRTOS queue or exercise web requests.
No PlatformIO invocation or firmware build is performed by these tests.

Still required from the operator:

1. Compile the full target/channel matrix, checking C3 size and RAM use plus
   actual library middleware/link wrappers. No new dependency was introduced.
2. Capture release startup in the webflasher console: BUILD and short LOG events
   at level 3, only errors at level 1, no automatic logs at 0. Custom forced/core
   flags=5 and either publication channel must not expose detailed diagnostics.
3. Verify /debug and /advanced levels, persistent config versus transient page
   levels, queue updates and Status snapshot even with logging off. When web
   auth is configured, /serial must request the same credentials.
4. Exercise HTTP/RPC failure codes, Wi-Fi disconnect/reconnect, sensor and SD
   errors; check repeat suppression and both distinct fallback result codes.
   Confirm no bodies, signatures, credentials or measured values in release logs.
5. Check Improv provisioning with USB logging active, LoRa frames, normal UI/API,
   SD CSV/analytics, and no new release SD diagnostic files. Check responsiveness
   with no browser polling and with web polling plus USB monitoring.
6. Repeat debug sensor/HEALTH capture and a >1h Insight VOC run. Keep raw logs,
   precise revision/flags and observations. The prior captures do not close this.

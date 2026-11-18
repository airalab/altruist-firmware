# Diagnostic profiles and operational support logs

Release provides bounded operational logs for local web support and the
webflasher USB console. Detailed sensor and protocol diagnostics require debug.
Stable/Testing are publication channels; release/debug are build profiles.

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
| Web /serial and log pages | Same configured authentication as /debug; shared RAM log tail and named modes | Same interface, level 5 also available |
| USB | Curated operational lines through actual Serial.write | Existing detailed USB output |
| UART0 / LoRa | Support log never writes HardwareSerial; functional structured writes remain active | Existing structured LoRa protection |
| Improv | Binary protocol preserved; USB support lines muted by debugSetUsbQuiet | Existing provisioning behavior |
| SD exceptions boot/runtime files | Not created; old files not automatically erased | Existing debug-only files |
| SD sensor CSV, analytics, VOC SPIFFS | Preserved | Functional storage, not diagnostic logs |
| Core/library/libc/ROM hooks | Existing release clamp and silent hooks retained | Existing diagnostic output |

## Operational vocabulary and levels

Automatic release logging follows the saved cfg::debug, capped at 3. Forced
project levels do not override that cap. Default is 3. The UI presents only named
modes: Off, Errors only, Events; it highlights the active mode and shows no numeric
input or numeric current-level indicator. The named modes use values 0/1/3.
Requests for value 2 are ignored. Release caps levels
4/5 to Events; runtime verbosity cannot enable detailed diagnostics.

Debug adds Detailed (4) and Most detailed (5). Its configured force remains a
minimum: the standard force=4 build offers only Detailed and Most detailed,
with a visible explanation, instead of buttons that cannot take effect.
A live mode change does not save config or restart. The existing Save and restart
form persists the current selection through a hidden `debug` field; there is no
second numeric control. Saving that form deliberately reboots the device.
Automatic logs may therefore be quiet after reboot at Errors only. A compile-time
default change or app-only reflash does not necessarily replace persisted config.

Status snapshot is separate from mode controls. It preserves the mode even when
a request includes both status=1 and lvl; the status action takes precedence.
Selecting Most detailed and then taking a snapshot keeps Most detailed selected.
Explicit mode changes emit a short mode_changed acknowledgement, even for Off;
it is not an automatic event. The polling endpoint and log page are not cached.

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
its queue writes do not wait for a reader. A mutex coordinates writers
and draining/eviction. USB output uses the real USB Serial, without forwarding
support lines to UART0. Snapshot and curated events share the same writer.

Repeated automatic events are suppressed for 30 seconds per subsystem/event/code
using 16 recent keys protected by a critical section. Older keys may be evicted
when more than 16 distinct combinations occur. This is diagnostic throttling,
not counters of failures or attempts. Explicit snapshots are not throttled.
Web polling destructively drains the shared queue: multiple browsers and the
debug SD consumer do not each receive an independent copy. Persistent storage
and multi-viewer replay are not supported.

## Sensor telemetry and system limitations

Local debug JSON carries standard Urban/Insight readings, including
BME680 `voc_ppb`, VOC index/level/count and gas resistance/baseline, before uplink
selection. `voc_ppb` is an integer relative MOS/ABC scale, 0–10000 with JSON
unit `ppb`, not laboratory TVOC. Constant zero or nonzero values are valid;
`voc_level=-1` signals an invalid gas reading. Successful protobuf skips legacy
PAYLOAD but does not eliminate local JSON.
Versioned UART snapshots add measurement identity, timing and device context for
late attachment. Release emits no periodic HEALTH or sensor measurements.

System output outside application logging controls includes ROM/bootloader
output, early startup before application hooks and panic output from the precompiled ESP-IDF. Suppressing all
of those requires separate SDK/bootloader work. Functional Improv/LoRa/UI/API
and working SD data remain available.

Map server selection failures use debug `[CONNECTIVITY] failed ... stage=health
reason=health_get_failed|health_begin_failed host=... code=...`. These GET probes
are diagnostics, not POST delivery outcomes. Operational LOG uses the distinct
`connectivity/health_failed` event at Info, without host/payload/response data.
POST and local send failures use delivery events.

PAYLOAD diagnostics contain only channel, encoding, encrypted flag and payload
length. Sensor samples are not included. Local sensor measurements are emitted
separately as versioned UART snapshots.

## Versioned sensor UART snapshots

Debug emits a separate per-physical-sensor UART JSON snapshot with root
`_observations` version 2 and per-numeric-metric `observation`. Supported sensors:
BME280/BMP280, BME680, SCD4x, SDS, ICS43434. API/uplink/SD documents receive no
metadata. Release does not allocate or emit this telemetry, regardless of
Testing or runtime verbosity.

Each record carries MAC/model/boot identity, monotonic esp_timer milliseconds,
per-sensor snapshot/cycle sequences, effective settings, per-channel completed
cycle/ok/failed counters, event times and a ring of 16 failed cycles. Clock and
counter values are decimal strings. An init error does not invent measurement
cycles; an uninitialized sensor remains visible diagnostically without entering
acquisition/API/SD. Equal new readings advance measurement identity, while
failed cycles preserve prior successful values/times. Environment and gas are
separate BME680 channels. The failed-cycle journal and cumulative counters
survive publication and aggregate resets.

BME/SCD snapshots follow completed readings; SDS snapshots follow completed PM
windows. Initial/state publications are limited to one per second. Unchanged
cache is not repeated periodically. Microphone publications are coalesced to
30 seconds, with separately limited state transitions. Noise time statistics
use two completed 300-second active windows and a current window; only the
bounded 20-second allowance per actual SDS cycle can be excluded. Blocking
worker operations may delay publication; intervals are rate limits, not deadlines.

BME680 gas and ICS43434 currently declare acquisition-only `probes` in the same
snapshot. They report raw gas transfer/status/resistance, or I²S result/byte
count/raw signal extrema. Their measurement counters and identities remain zero
and values null with `waiting/driver_unverified`. A complete varying I²S block
and new nonzero gas data demonstrate activity without certifying calibrated noise
or valid gas/VOC measurements. Probe collection does not alter driver acceptance,
FFT/calibration, aggregate reset behavior or API/uplink/SD values.

The tester requires an explicitly selected activity profile to accept these
channels with WARN; ordinary full acceptance rejects acquisition-only evidence.
New activity after capture baseline is required. Full measurement checks remain
in force for all other channels. Detailed probe schema and limitations belong to
the tester `docs/sensor-log-patterns.md`, section "Acquisition probes and limited
activity profiles".

Up to 16 metrics and 8192 serialized UTF-8 bytes per line are supported. A
16384-byte temporary JSON pool is checked independently from serialized length;
overflow emits the v2 snapshot_overflow marker. One Serial.write emits the
complete LF-terminated record. A formed but lost record still consumes
snapshot_seq and the noise interval, allowing the tester to detect coverage loss.

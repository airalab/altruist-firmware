# Altruist Firmware

Firmware for the Altruist environmental sensor station, built on ESP32-C6 and
legacy ESP32-C3 Urban hardware.

## Architecture Overview

In the Robonomics sensors architecture, the Altruist plays two roles:

- **Datalog reporting**: Signs and sends measurement datalogs directly to the Robonomics parachain via RPC node (every 10 minutes).
- **Connectivity reporting**: Signs and sends measurement data to Sensors Connectivity Provider nodes via HTTP POST on port 65 (every 30 seconds).

```
Altruist (ESP32-C6)
  |
  |-- Signed extrinsic --> Robonomics Parachain (Polkadot)
  |                            |
  |                  RoSeMAN indexes --> MongoDB --> sensors.social
  |
  +-- Signed msg HTTP:65 --> Sensors Connectivity Provider
        |-- Real-time: IPFS pubsub --> Robonomics dApp
        +-- Batch: IPFS pin --> datalog hash --> Parachain
```

An ED25519 keypair is generated on first boot and stored in SPIFFS (`/config.json`). This identity is used to sign both datalog extrinsics and connectivity messages.

The connectivity server pool is defined in `robonomics_servers.h` (currently 3 servers). On startup, the device polls all servers and picks one where it is already registered, or the least loaded one.

Hardware reset (GPIO7) clears WiFi credentials and password but preserves the Robonomics identity.

## Hardware Variants

### Altruist Urban

Outdoor station (ESP32-C6 or ESP32-C3). Provides environmental and air quality measurements. **ESP32-C6:** discovered by Insight via mDNS (`altruist._tcp`). **ESP32-C3:** no mDNS (flash budget) — pair Insight with **custom Urban IP** in config; open the device by LAN IP in a browser.

### Altruist Insight

Indoor station (ESP32-C6) with display and QR code support. Can aggregate data from nearby Urban devices over the local network.

## Supported Sensors

| Sensor               | Measurement                                     |
| -------------------- | ----------------------------------------------- |
| SDS011               | PM2.5, PM10                                     |
| BMx280 (BMP/BME 280) | Temperature, humidity, pressure                 |
| BME680               | Temperature, humidity, pressure, gas resistance |
| SCD4x (SCD40/SCD41)  | CO2, temperature, humidity                      |
| RadSens              | Radiation (counts per minute)                   |
| I2S microphone       | Noise level (dBA)                               |
| GPS (Neo-6M)         | Latitude, longitude                             |
| HTTP Altruist sensor | Data from linked Urban devices                  |

## Building and Flashing

The project uses [PlatformIO](https://platformio.org/install/cli). Build environments are defined in `platformio.ini`.

Build for a specific target:

```bash
pio run -e esp32c6_urban_en
pio run -e esp32c6_inside_en
```

Flash:

```bash
pio run -e esp32c6_urban_en --target upload
```

### Build Model

The PlatformIO environment and Git branch control different things:

- **Environment** selects hardware, language, and build profile.
- **Branch** selects the firmware publication channel.

| Selection | Result |
| --------- | ------ |
| Environment without `_debug` | Release profile with normal project logs |
| Environment with `_debug` | Local Debug profile with verbose project and framework logs |
| Branch `esp32` | Stable channel |
| Branch `esp32-dev` or a feature branch | Testing channel |
| Detached/no-Git local build | Stable channel unless explicitly overridden |

Only release builds are published to webflasher. Debug builds are intended for
local diagnostics and do not create publishable artifacts. The Debug profile
does not change the firmware channel: a Debug build on `esp32-dev` is still
Testing firmware, just compiled with verbose diagnostics.

### Environments

Release environments:

- `esp32c3_urban_en`, `esp32c3_urban_ru`
- `esp32c6_urban_en`, `esp32c6_urban_ru`
- `esp32c6_inside_en`, `esp32c6_inside_ru`

Debug environments use the same names with the `_debug` suffix. ESP32-C3 Debug
builds keep the C3-lite feature set to stay within the old hardware limits. The
legacy `inside` environment name builds Insight firmware.

### Common Scenarios

- On `esp32-dev`, build `esp32c6_urban_en` to test the same Testing release
  profile that CI publishes to webflasher.
- On `esp32`, build `esp32c6_urban_en` to reproduce a Stable release.
- Use `esp32c6_urban_en_debug` only when verbose diagnostics or JTAG are needed.
- Use `esp32c3_urban_en_debug` to debug old ESP32-C3 Urban hardware with the
  same reduced feature set as the C3 release build.

Local builds infer the channel from the branch, enable health telemetry, and
read the source commit from Git. Detached or no-Git local builds fall back to
Stable unless `ALTRUIST_CHANNEL_TESTING=1` is set explicitly. The build output
prints the resolved branch, channel, profile, telemetry state, and commit before
compilation.

Stable and Testing release builds use the same runtime logging behavior. The
channel controls firmware identity, artifact names, and OTA policy; the build
profile controls diagnostic verbosity. Debug builds force verbose project logs
with `ALTRUIST_FORCE_LOG_LEVEL=4`.

The tester UART contract is independent from runtime project logs: `[BOOT]`,
`[BUILD]`, `[LOG]`, `[HEALTH]`, `[PAYLOAD]`, `[DATALOG]`, `[CONNECTIVITY]`, and
`[SUBSYSTEM]` are emitted directly to Serial in release builds, while full
transport payloads, sensor internals, and HTTP details stay behind verbose/debug
logging.

CI sets `ALTRUIST_CHANNEL_TESTING`, `ALTRUIST_HEALTH_TELEMETRY`, and
`ALTRUIST_BUILD_COMMIT` explicitly, so published builds do not depend on local
Git state. These variables can also override the automatic local defaults.

Stable artifacts use names such as `latest32c6urb_en.bin`; Testing artifacts use
`latest32c6urb_en_testing.bin`. Legacy `_dev.bin` aliases are also produced for
external compatibility and point to the same Testing firmware.

Stable firmware keeps the base version, for example `R-URB_2026-06.1`. Testing
firmware includes its source revision, for example
`R-URB_2026-06.1-testing+7445b03`. UART startup logs and the status page expose
the channel, commit, model, target, language, and profile.

Insight builds use the `ALTRUIST_INSIGHT` compile-time flag. The previous
`ALTRUIST_INSIDE` name remains available as a compatibility alias; new code
should use `ALTRUIST_INSIGHT`.

OTA updates are pinned to Stable artifacts for now: every firmware build requests the normal language artifact without the `_testing` suffix. Testing firmware is installed explicitly through webflasher or local flashing, and automatic OTA is disabled in Testing builds so devices do not immediately return to Stable. Manual `/ota` remains available as an explicit Stable rollback path. Changing the build profile, runtime log level, or legacy `use_beta` configuration cannot switch OTA away from Stable.

## Configuration

On first boot (or after reset), the device starts in Access Point mode. Connect to its WiFi network and open the configuration page to set:

- WiFi credentials
- GPS coordinates
- Sensor enable/disable
- API endpoints

After configuration, the device restarts and connects to the specified WiFi network. The web UI remains available on the local network for reconfiguration.

## Button Controls

### Insight

- `UP` short press - previous screen
- `DOWN` short press - next screen
- `UP` / `DOWN` short press on **Graphs** screen - switch graph (at edges switches screen). Long press changes screen
- `SET` long press - sleep
- `SET` + `DOWN` long press (4s) - reset WiFi configuration
- `SET` + `DOWN` pressed while powering on - reset all configuration

### Urban (ESP32-C6 with hardware UI)

**Reset button (GPIO7)**

- **While powering on** (hold before/at power-on) — factory reset: deletes the full `config.json` including the Robonomics identity (same as Insight `SET` + `DOWN` at boot).
- **While running** — hold for more than 10 seconds, then release: clears Wi‑Fi credentials and the web UI password only; Robonomics identity is preserved and the device reboots into the setup captive portal. Boot-time and runtime actions do not overlap — a power-on hold is consumed as factory reset only.
- LEDs turn blue briefly while the runtime Wi‑Fi reset is applied.

**Status LEDs** (NeoPixel ring on ESP32-C6 Urban; both pixels show the same color)

| Color            | Meaning                                                                         |
| ---------------- | ------------------------------------------------------------------------------- |
| **Green**        | Normal operation — Wi‑Fi connected and on-chain Robonomics datalog is healthy.  |
| **Blue**         | Setup mode — no saved Wi‑Fi yet, or the captive configuration portal is active. |
| **Blue**         | Also shown while an on-chain datalog transmission is in progress.               |
| **Green** (~3 s) | Last datalog send succeeded (brief flash after transmission).                   |
| **Red** (~3 s)   | Last datalog send failed (brief flash after transmission).                      |
| **Red** (steady) | Wi‑Fi disconnected or datalog unhealthy for more than 10 minutes.               |

Map/connectivity HTTP errors do not drive the steady red state — LED status reflects **Robonomics datalog** health, not the sensors map POST.

LED indication can be disabled in the web configuration.

## Contributing

Submit development changes against the **esp32-dev** branch first. After they
are validated, promote them to **esp32**, which is the Stable release branch.

To add a Connectivity Robonomics Server, fork this repository and edit `robonomics_servers.h`. Add your server:

```c
{"<server_address>", REGION_XX}
```

Available regions: `REGION_GLOBAL`, `REGION_EU`, `REGION_AS`, `REGION_AF`, `REGION_AU`, `REGION_NA`, `REGION_SA`.

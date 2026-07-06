## Debugging Insight / Urban firmware

This document describes how to build and debug the **Insight** and **Urban** firmware variants, and how to use the SD‑card based logging that was added for long‑running issues (freezes, sporadic resets, missing Urban data, etc.).

---

## Quick Start

### What `_debug` builds give you

All `_debug` environments (`esp32c3_urban_en_debug`, `esp32c3_urban_ru_debug`, `esp32c6_inside_en_debug`, `esp32c6_inside_ru_debug`, `esp32c6_urban_en_debug`, `esp32c6_urban_ru_debug`) include:

| Feature | What it does |
| ------- | ------------ |
| **Project logs** (`ALTRUIST_FORCE_LOG_LEVEL=4`) | Keeps `debug_outln_error/info/verbose` enabled even when saved `cfg::debug` is lower |
| **Arduino core logs** (`CORE_DEBUG_LEVEL=4`) | Enables Arduino framework informational and debug output at level 4 |
| **Health telemetry** (`ALTRUIST_HEALTH_TELEMETRY`) | Emits the stable `[HEALTH]` UART snapshot once per minute |
| **Debug symbols** (`-g -Og`) | Enables breakpoints and variable inspection |
| **Built-in JTAG** (`debug_tool = esp-builtin`) | No external hardware needed on supported ESP32-C3/C6 boards |

### 3-step debugging

**Step 1: Flash a `_debug` build**

```bash
pio run -e esp32c6_inside_en_debug -t upload
```

**Step 2: Watch verbose logs**

```bash
pio device monitor
```

You'll see timestamped logs like:

```
[123456] [DEBUG] [Display] Drawing sensor map screen
[123457] [INFO] [MEM] Free heap bytes: 180000
[123458] [ERROR] [HTTP] Request failed, code: -1
```

**Step 3: Start JTAG debugging (optional)**

Just press **F5** in VS Code — that's it!

The debugger will:

- Build and flash the firmware
- Break at `setup()` automatically
- Let you set breakpoints, step through code, inspect variables

---

### 1. Environments overview

The main PlatformIO environments relevant for Insight/Urban are:

- **Urban (ESP32‑C6)**
  - `env:esp32c6_urban_en` – production EN
  - `env:esp32c6_urban_ru` – production RU
  - `env:esp32c6_urban_en_debug` – **technical debug EN**
  - `env:esp32c6_urban_ru_debug` – **technical debug RU**

- **Urban (ESP32‑C3, old hardware)**
  - `env:esp32c3_urban_en` – production EN
  - `env:esp32c3_urban_ru` – production RU
  - `env:esp32c3_urban_en_debug` – **technical debug EN**
  - `env:esp32c3_urban_ru_debug` – **technical debug RU**

- **Insight (ESP32‑C6, “inside”)**
  - `env:esp32c6_inside_en` – production EN
  - `env:esp32c6_inside_ru` – production RU
  - `env:esp32c6_inside_en_debug` – **technical debug EN**
  - `env:esp32c6_inside_ru_debug` – **technical debug RU**

Use the `_debug` environments for local diagnostics or an attached debugger.
They are not published to webflasher. The firmware channel still follows the
selected branch, so `_debug` builds on `esp32-dev` are Testing firmware with a
Debug build profile.

### 2. Building and uploading with PlatformIO

You can build and upload any environment either from the **VS Code** UI or from the command line.

#### 2.1. From the IDE

- Open the project folder in VS Code.
- In the PlatformIO panel, select the environment you want (for example `esp32c6_inside_en_debug`).
- Use **Build** to compile and **Upload** to flash the firmware to your board.

#### 2.2. From the command line

From the project root:

```bash
# Build Insight EN debug
pio run -e esp32c6_inside_en_debug

# Build & upload Insight EN debug
pio run -e esp32c6_inside_en_debug -t upload

# Build & upload Insight RU debug
pio run -e esp32c6_inside_ru_debug -t upload

# Similarly for Urban EN/RU debug
pio run -e esp32c6_urban_en_debug -t upload
pio run -e esp32c6_urban_ru_debug -t upload
pio run -e esp32c3_urban_en_debug -t upload
pio run -e esp32c3_urban_ru_debug -t upload
```

### 3. Serial logging

The firmware has three independent logging layers:

1. **Project logs** use `debug_outln_error`, `debug_outln_info`, and
   `debug_outln_verbose` from `utils.cpp`. `ALTRUIST_DEFAULT_LOG_LEVEL` initializes
   `cfg::debug`, while `ALTRUIST_FORCE_LOG_LEVEL` sets its minimum effective value.
   Debug builds force level 4, so an older saved configuration cannot suppress
   project diagnostics.
2. **Arduino core logs** are controlled at compile time by `CORE_DEBUG_LEVEL`.
   Debug builds set it to 4 and route diagnostics to `Serial` through
   `DEBUG_ESP_PORT`. Precompiled ESP-IDF library logs remain limited by the
   SDK configuration bundled with the Arduino package.
3. **Health telemetry** is controlled by `ALTRUIST_HEALTH_TELEMETRY`. It is a
   stable machine-readable snapshot for the tester and does not depend on either
   project or framework log levels.

Project log lines are prefixed with a level:

- `[INFO]` – general information
- `[ERROR]` – errors and failures
- `[DEBUG]` – extra verbose / development information

Each line also contains a millisecond timestamp in square brackets (time since boot). See the Quick Start section above for example output.

Builds with `ALTRUIST_HEALTH_TELEMETRY` emit a stable UART health snapshot once
per minute:

```text
[HEALTH] uptime=3600 boot=4 heap=219584 rssi=-62 tx=12 errors=0 wifi=1 wifi_errors=0 sensor_errors=0 sd_errors=0
```

The initial fields are kept stable for simple parsers. The trailing fields add
the Wi-Fi link state and per-subsystem error counters for diagnostics.

At boot, firmware reports the effective project and framework levels after the
saved configuration has been loaded:

```text
[LOG] runtime=4 configured=1 forced=4 core=4
```

Here `configured` is the persisted `cfg::debug`, `forced` is the compile-time
minimum, and `runtime` is the effective project log level.

To view these logs live:

1. Connect the board over USB.
2. Use PlatformIO **Monitor** or any serial terminal (115200 baud, 8‑N‑1).
3. Select the correct serial port for your device.

#### 3.1. Saving serial logs to a file

For long-running debug sessions (hours or days), you can capture serial output to a file using terminal tools:

**Basic capture with timestamp:**

```bash
pio device monitor | tee "debug_$(date +%Y%m%d_%H%M%S).log"
```

This shows output on screen AND saves to a timestamped file like `debug_20260206_143052.log`.

**Append to existing file (multiple sessions):**

```bash
pio device monitor | tee -a debug_log.txt
```

**Auto-reconnect for very long sessions:**

```bash
while true; do pio device monitor | tee -a "debug_$(date +%Y%m%d).log"; sleep 2; done
```

This automatically reconnects if the device disconnects/resets and appends to the same daily log file. Press `Ctrl+C` twice to exit.

**Using `script` command (captures everything):**

```bash
script debug_log.txt
pio device monitor
# Press Ctrl+D when done to save and exit
```

### 4. SD‑card runtime logging

For long‑running issues (for example freezes after 5–7 days) it is not practical to keep a USB cable connected. For this reason the firmware can mirror logs to the SD card (Insight builds with `-DUSE_SD_CARD`).

#### 4.1. Log files

On the SD card (usually mounted as `NO NAME` on macOS) you will find:

- `/exceptions/boot_*.txt` – one file per boot, containing reset reason and crash breadcrumbs.
- `/exceptions/runtime.log` – rolling runtime log with the same messages you see on serial.
- `/exceptions/runtime.log.1`, `/exceptions/runtime.log.2` – rotated backups when the main log reaches its size limit.

Log rotation avoids filling the SD card: when `runtime.log` exceeds the configured size (about 128 KiB), it is rotated and a new file is started.

#### 4.2. What is logged

Examples of important SD‑logged messages:

- **Display / E‑Ink watchdog**
  - `[Display]` and `[EPD]` messages when screens are drawn or the E‑Ink driver is re‑initialized.
  - Helps to see if the screen froze while the main loop is still running.

- **Urban discovery & HTTP errors**
  - Messages from `HTTPAltruistSensor`, for example:
    - `HTTPAltruistSensor: scheduled rediscovery attempt ...`
    - `Request to Altruist Urban failed, code: ...`
    - `HTTPAltruistSensor: reached max discovery attempts, Urban assumed absent`

- **Memory**
  - `[MEM] Free heap bytes: ...` – printed every 60 seconds.
  - `[MEM][WARN] Low free heap bytes: ...` – when free heap is below a safe threshold.

- **SD‑card state**
  - `[SDCardLogger]` messages when the card is inserted/removed or when write errors happen. Card type is logged only when it changes or on error, to avoid log spam.

### 5. Crash and reset diagnostics (boot logs)

On every boot the firmware writes a short report to `/exceptions/boot_*.txt` that includes:

- **Reset reason** (power‑on, watchdog, panic, etc.) and numeric code.
- **Crash data validity** — indicates whether crash breadcrumbs saved to NVS are valid:
  - `yes` — crash data was saved before the reset, breadcrumbs are meaningful.
  - `no` — power cycle, first boot, or no saved context.
- **NVS breadcrumbs** (only valid when `crash_data_valid: yes`):
  - `last_section_id` / `last_section_name` — which code path was active when crash context was last saved:
    - 0 = Idle/MainLoop
    - 1 = FetchSensors
    - 2 = RobonomicsDatalog
    - 3 = RobonomicsHTTPMap
    - 4 = CustomHTTP
    - 5 = DisplayUpdate
    - 6 = LEDUpdate
    - 7 = WiFiReconnect
    - 8 = SDWrite
  - `prev_uptime_sec` — how long the device ran (at last NVS save, every 30s).
  - `prev_free_heap` — heap memory available at last NVS save.
- **Current state** at boot: free heap, WiFi RSSI.

**How it works:** Every 30 seconds, the firmware saves the current code section, uptime, and free heap to NVS flash. If a panic or watchdog reset occurs, this data survives and is read on the next boot.

**Example boot file:**

```
reset_reason: Panic reset (e.g., unhandled exception)
reset_reason_code: 4
boot_counter: 123
crash_data_valid: yes
prev_uptime_sec: 86420
prev_free_heap: 45000
last_section_id: 3
last_section_name: RobonomicsHTTPMap
current_free_heap: 214000
rssi: -65
```

This tells you:

- The crash was a real panic (code 4).
- Device ran for ~24 hours before crashing.
- Last known code section was Robonomics HTTP Map API call.
- Heap was getting low (45KB) before crash — possible memory issue.

### 6. PlatformIO debugging (JTAG)

Supported ESP32-C3/C6 boards expose **built-in USB JTAG** — no external probe is
needed when that interface is available. The `_debug` environments are
pre-configured to use it.

#### 6.1. Debug configuration in platformio.ini

The `_debug` environments include these debugging options (from [PlatformIO Debugging docs](https://docs.platformio.org/en/latest/plus/debugging.html)):

```ini
build_type = debug
debug_tool = esp-builtin       ; Use built-in USB JTAG
debug_init_break = tbreak setup ; Break at setup() on start
```

Build flags for debugging:

- `-g -Og -fno-inline` — Include debug symbols, optimize for debugging
- `-DUSING_JTAG_DEBUGGER_PINS=1` — Reserve JTAG pins
- `-DDEBUG_ESP_PORT=Serial` — Route framework diagnostics to `Serial`
- `-DCORE_DEBUG_LEVEL=4` — Enable Arduino framework logs at level 4
- `-DALTRUIST_BUILD_DEBUG` — Enable heavyweight development diagnostics
- `-DALTRUIST_DEFAULT_LOG_LEVEL=4` — Initialize `cfg::debug` to verbose output
- `-DALTRUIST_FORCE_LOG_LEVEL=4` — Keep project logs verbose despite saved `cfg::debug`
- `-DALTRUIST_HEALTH_TELEMETRY` — Emit periodic tester-oriented health snapshots
- `NDEBUG` is intentionally omitted so assertions and debug-only library paths remain available

#### 6.2. Starting a debug session

**From VS Code:**

1. Select the `_debug` environment in the PlatformIO sidebar (for example `esp32c6_inside_en_debug`).
2. Click the **Debug** icon in the sidebar, or press `F5`, or choose **Run** → **Start Debugging**.
3. PlatformIO will build, flash, and launch GDB automatically.

**From the command line:**

```bash
# Start debugging
pio debug -e esp32c6_inside_en_debug

# Or build first, then debug
pio run -e esp32c6_inside_en_debug
pio debug -e esp32c6_inside_en_debug --interface=gdb
```

#### 6.3. Debug features available

Once the debugger is attached, you can:

- **Set breakpoints** — Click in the gutter next to any line in `airrohr-firmware.ino`, `display_manager.cpp`, `http_altruist_sensor.cpp`, etc.
- **Step through code** — Use F10 (step over), F11 (step into), Shift+F11 (step out).
- **Inspect variables** — View `sensors_data`, `deviceStatus`, `crash_last_section`, heap usage in the Variables pane.
- **Watch expressions** — Add custom watch expressions like `ESP.getFreeHeap()`.
- **View call stack** — See the full call stack when stopped at a breakpoint.
- **Peripheral registers** — Inspect hardware register states (advanced).

#### 6.4. Using an external probe (ESP-PROG)

If you prefer an external JTAG probe instead of the built-in USB JTAG:

1. Connect ESP-PROG to the JTAG pins of the target board.
2. Update the environment in `platformio.ini`:

```ini
debug_tool = esp-prog
upload_protocol = esp-prog
```

See [PlatformIO ESP-Prog docs](https://docs.platformio.org/en/latest/plus/debug-tools/esp-prog.html) for wiring details.

#### 6.5. Troubleshooting debugging

- **"No debug adapter found"** — Ensure the USB cable supports data (not charge-only). Try a different USB port.
- **Breakpoints not hitting** — Make sure you're using a `_debug` environment with `build_type = debug`.
- **Debugging is slow** — Set `debug_speed = 5000` in platformio.ini to increase JTAG clock speed.

### 7. Typical debugging workflows

#### 7.1. Investigating screen freezes on Insight

1. Flash a `_debug` Insight build (for example `esp32c6_inside_en_debug`).
2. Let the device run as usual, with an SD card inserted.
3. When you observe a screen freeze:
   - Do **not** power‑cycle immediately if possible; first note if LEDs / network are still active.
   - Then reboot the device once.
4. Remove the SD card and inspect:
   - Latest `/exceptions/runtime.log` – look for:
     - `[Display]` / `[EPD]` messages before the freeze.
     - `[MEM][WARN]` low‑heap warnings.
     - Repeated HTTP or SD errors.
   - The latest `/exceptions/boot_*.txt` – check reset reason and last code section.

#### 7.2. Investigating Urban missing data or discovery issues

1. Use a `_debug` Urban or Insight build.
2. Watch for lines mentioning `HTTPAltruistSensor` in serial or SD logs:
   - Rediscovery attempts.
   - Maximum discovery attempts reached.
   - HTTP error codes.
3. Confirm whether Urban’s `/data.json` is reachable (from a browser or `curl`) while the device is running.

#### 7.3. Watching for memory leaks or fragmentation

1. Run a `_debug` Insight build with SD logging enabled.
2. Let the device run for several days.
3. Periodically review `/exceptions/runtime.log`:
   - Check the `[MEM] Free heap bytes` series over time.
   - Look for `[MEM][WARN]` events.
4. If free heap continuously decreases without recovering, you likely have a leak in one of the tasks (for example JSON allocations, API clients, or display buffers).

---

If you run into a bug, the most useful artifacts to collect are:

- The latest `/exceptions/runtime.log` (and `.1`, `.2` if present).
- The latest one or two `/exceptions/boot_*.txt` files.
- The exact firmware environment name used (for example `esp32c6_inside_en_debug`).

Attach those to an issue or share them when asking for help, and you can usually pinpoint the problem quickly.

---

## Cheat Sheet

### Essential Commands

```bash
# Build debug firmware
pio run -e esp32c6_inside_en_debug

# Build and flash
pio run -e esp32c6_inside_en_debug -t upload

# Watch serial logs (115200 baud)
pio device monitor

# Save serial logs to file (long debug sessions)
pio device monitor | tee "debug_$(date +%Y%m%d_%H%M%S).log"

# Auto-reconnect and log (for multi-day sessions)
while true; do pio device monitor | tee -a "debug_$(date +%Y%m%d).log"; sleep 2; done

# Start JTAG debugger from CLI
pio debug -e esp32c6_inside_en_debug

# Clean build (if things are weird)
pio run -e esp32c6_inside_en_debug -t clean
```

### VS Code Shortcuts

| Action            | Shortcut               |
| ----------------- | ---------------------- |
| Start debugging   | **F5**                 |
| Stop debugging    | **Shift + F5**         |
| Step over         | **F10**                |
| Step into         | **F11**                |
| Step out          | **Shift + F11**        |
| Continue          | **F5**                 |
| Toggle breakpoint | **F9** or click gutter |

### Log Levels

| Level     | Macro                      | When to use                              |
| --------- | -------------------------- | ---------------------------------------- |
| `[ERROR]` | `debug_outln_error(...)`   | Failures, exceptions                     |
| `[INFO]`  | `debug_outln_info(...)`    | Normal operation events                  |
| `[DEBUG]` | `debug_outln_verbose(...)` | Detailed tracing (only in `_debug` builds) |

### SD Card Log Files

| File                        | Contents                          |
| --------------------------- | --------------------------------- |
| `/exceptions/boot_*.txt`    | Reset reason, crash breadcrumbs   |
| `/exceptions/runtime.log`   | Live rolling log (mirrors serial) |
| `/exceptions/runtime.log.1` | Rotated backup                    |

---

## Отладка прошивки Insight / Urban (RU)

Этот документ описывает сборку и отладку прошивок **Insight** и **Urban**, а также использование логирования на SD‑карту для диагностики длительных проблем (зависания, спонтанные перезагрузки, отсутствие данных Urban и т.д.).

---

## Быстрый старт

### Что дают сборки `_debug`

Все `_debug`‑окружения (`esp32c3_urban_en_debug`, `esp32c3_urban_ru_debug`, `esp32c6_inside_en_debug`, `esp32c6_inside_ru_debug`, `esp32c6_urban_en_debug`, `esp32c6_urban_ru_debug`) включают:

| Функция | Описание |
| ------- | -------- |
| **Логи проекта** (`ALTRUIST_FORCE_LOG_LEVEL=4`) | Не позволяет сохраненному `cfg::debug` отключить `debug_outln_error/info/verbose` |
| **Логи Arduino core** (`CORE_DEBUG_LEVEL=4`) | Включает информационный и отладочный вывод Arduino framework на уровне 4 |
| **Health telemetry** (`ALTRUIST_HEALTH_TELEMETRY`) | Раз в минуту выводит стабильную строку `[HEALTH]` |
| **Символы отладки** (`-g -Og`) | Позволяют ставить точки останова и инспектировать переменные |
| **Встроенный JTAG** (`debug_tool = esp-builtin`) | Внешнее оборудование не нужно на поддерживаемых платах ESP32‑C3/C6 |

### Отладка в 3 шага

**Шаг 1: Прошить `_debug` сборку**

```bash
pio run -e esp32c6_inside_en_debug -t upload
```

**Шаг 2: Смотреть логи**

```bash
pio device monitor
```

Вы увидите логи с метками времени:

```
[123456] [DEBUG] [Display] Drawing sensor map screen
[123457] [INFO] [MEM] Free heap bytes: 180000
[123458] [ERROR] [HTTP] Request failed, code: -1
```

**Шаг 3: Запустить JTAG‑отладку (опционально)**

Нажмите **F5** в VS Code — и всё!

Отладчик:

- Соберёт и прошьёт прошивку
- Остановится на `setup()` автоматически
- Позволит ставить точки останова, пошагово выполнять код, инспектировать переменные

---

### 1. Обзор окружений

Основные окружения PlatformIO для Insight/Urban:

- **Urban (ESP32‑C6)**
  - `env:esp32c6_urban_en` – продакшн EN
  - `env:esp32c6_urban_ru` – продакшн RU
  - `env:esp32c6_urban_en_debug` – **техническая отладка EN**
  - `env:esp32c6_urban_ru_debug` – **техническая отладка RU**

- **Urban (ESP32‑C3, старое железо)**
  - `env:esp32c3_urban_en` – продакшн EN
  - `env:esp32c3_urban_ru` – продакшн RU
  - `env:esp32c3_urban_en_debug` – **техническая отладка EN**
  - `env:esp32c3_urban_ru_debug` – **техническая отладка RU**

- **Insight (ESP32‑C6, "inside")**
  - `env:esp32c6_inside_en` – продакшн EN
  - `env:esp32c6_inside_ru` – продакшн RU
  - `env:esp32c6_inside_en_debug` – **техническая отладка EN**
  - `env:esp32c6_inside_ru_debug` – **техническая отладка RU**

Используйте `_debug`‑окружения для локальной диагностики или подключения
отладчика. Они не публикуются в webflasher. Канал прошивки по-прежнему зависит
от выбранной ветки, поэтому `_debug` сборки на `esp32-dev` остаются
Testing-прошивками с Debug-профилем сборки.

### 2. Сборка и загрузка через PlatformIO

Собрать и загрузить можно через **VS Code** или через командную строку.

#### 2.1. Из IDE

- Откройте папку проекта в VS Code.
- В панели PlatformIO выберите нужное окружение (например `esp32c6_inside_en_debug`).
- Нажмите **Build** для компиляции и **Upload** для прошивки.

#### 2.2. Из командной строки

Из корня проекта:

```bash
# Собрать Insight EN debug
pio run -e esp32c6_inside_en_debug

# Собрать и прошить Insight EN debug
pio run -e esp32c6_inside_en_debug -t upload

# Собрать и прошить Insight RU debug
pio run -e esp32c6_inside_ru_debug -t upload

# Аналогично для Urban EN/RU debug
pio run -e esp32c6_urban_en_debug -t upload
pio run -e esp32c6_urban_ru_debug -t upload
pio run -e esp32c3_urban_en_debug -t upload
pio run -e esp32c3_urban_ru_debug -t upload
```

### 3. Логирование через serial

В прошивке есть три независимых уровня логирования:

1. **Логи проекта** используют `debug_outln_error`, `debug_outln_info` и
   `debug_outln_verbose` из `utils.cpp`. `ALTRUIST_DEFAULT_LOG_LEVEL` задает
   начальное значение `cfg::debug`, а `ALTRUIST_FORCE_LOG_LEVEL` — минимальный
   эффективный уровень. В Debug-сборках уровень 4 принудителен, поэтому старое
   сохраненное значение не может отключить диагностические сообщения.
2. **Логи Arduino core** управляются compile-time флагом `CORE_DEBUG_LEVEL`.
   Debug-сборки задают уровень 4 и направляют вывод в `Serial` через
   `DEBUG_ESP_PORT`. Логи precompiled ESP-IDF библиотек остаются ограничены
   SDK-конфигурацией, поставляемой вместе с Arduino package.
3. **Health telemetry** управляется `ALTRUIST_HEALTH_TELEMETRY`. Это стабильная,
   машиночитаемая строка для тестера, не зависящая от уровней логов проекта и
   фреймворка.

Строки логов проекта имеют префикс уровня:

- `[INFO]` – общая информация
- `[ERROR]` – ошибки и сбои
- `[DEBUG]` – подробная информация (только в `_debug` сборках)

Каждая строка содержит метку времени в миллисекундах (с момента загрузки). Пример вывода см. в разделе «Быстрый старт» выше.

Сборки с `ALTRUIST_HEALTH_TELEMETRY` раз в минуту выводят стабильную строку
состояния в UART:

```text
[HEALTH] uptime=3600 boot=4 heap=219584 rssi=-62 tx=12 errors=0 wifi=1 wifi_errors=0 sensor_errors=0 sd_errors=0
```

Начальные поля сохраняются стабильными для простых парсеров. Поля в конце
добавляют состояние Wi-Fi и отдельные счетчики ошибок по подсистемам.

После загрузки сохраненной конфигурации прошивка сообщает фактически применяемые
уровни:

```text
[LOG] runtime=4 configured=1 forced=4 core=4
```

`configured` — сохраненный `cfg::debug`, `forced` — compile-time минимум, а
`runtime` — эффективный уровень логов проекта.

Для просмотра логов в реальном времени:

1. Подключите плату по USB.
2. Используйте PlatformIO **Monitor** или любой serial‑терминал (115200 бод, 8‑N‑1).
3. Выберите правильный COM‑порт устройства.

#### 3.1. Сохранение serial‑логов в файл

Для длительных сеансов отладки (часы или дни) можно сохранять serial‑вывод в файл:

**Базовый захват с меткой времени:**

```bash
pio device monitor | tee "debug_$(date +%Y%m%d_%H%M%S).log"
```

Показывает вывод на экране И сохраняет в файл вида `debug_20260206_143052.log`.

**Дополнение к существующему файлу (несколько сеансов):**

```bash
pio device monitor | tee -a debug_log.txt
```

**Авто-переподключение для очень длительных сеансов:**

```bash
while true; do pio device monitor | tee -a "debug_$(date +%Y%m%d).log"; sleep 2; done
```

Автоматически переподключается при отключении/перезагрузке устройства и дописывает в тот же дневной файл. Для выхода нажмите `Ctrl+C` дважды.

**Через команду `script` (захватывает всё):**

```bash
script debug_log.txt
pio device monitor
# Нажмите Ctrl+D для сохранения и выхода
```

### 4. Логирование на SD‑карту

Для длительных проблем (например зависания через 5–7 дней) непрактично держать USB‑кабель подключённым. Поэтому прошивка может зеркалировать логи на SD‑карту (сборки Insight с `-DUSE_SD_CARD`).

#### 4.1. Файлы логов

На SD‑карте (обычно монтируется как `NO NAME` на macOS):

- `/exceptions/boot_*.txt` – один файл на каждую загрузку, содержит причину перезагрузки и breadcrumbs.
- `/exceptions/runtime.log` – ротируемый лог с теми же сообщениями, что видны в serial.
- `/exceptions/runtime.log.1`, `/exceptions/runtime.log.2` – ротированные копии при превышении лимита.

Ротация логов предотвращает заполнение SD‑карты: когда `runtime.log` превышает заданный размер (~128 КиБ), он ротируется и создаётся новый файл.

#### 4.2. Что логируется

Примеры важных сообщений в логах SD:

- **Дисплей / E‑Ink watchdog**
  - Сообщения `[Display]` и `[EPD]` при отрисовке экранов или переинициализации драйвера E‑Ink.
  - Помогает понять, завис ли экран, пока основной цикл продолжает работать.

- **Обнаружение Urban и ошибки HTTP**
  - Сообщения от `HTTPAltruistSensor`:
    - `HTTPAltruistSensor: scheduled rediscovery attempt ...`
    - `Request to Altruist Urban failed, code: ...`
    - `HTTPAltruistSensor: reached max discovery attempts, Urban assumed absent`

- **Память**
  - `[MEM] Free heap bytes: ...` – выводится каждые 60 секунд.
  - `[MEM][WARN] Low free heap bytes: ...` – когда свободная память ниже безопасного порога.

- **Состояние SD‑карты**
  - Сообщения `[SDCardLogger]` при вставке/извлечении карты или ошибках записи.

### 5. Диагностика сбоев и перезагрузок (boot‑логи)

При каждой загрузке прошивка записывает краткий отчёт в `/exceptions/boot_*.txt`:

- **Причина перезагрузки** (включение питания, watchdog, panic и т.д.) и числовой код.
- **Валидность данных о сбое** — указывает, были ли breadcrumbs сохранены в NVS:
  - `yes` — данные сохранены перед перезагрузкой, breadcrumbs актуальны.
  - `no` — цикл питания, первая загрузка или нет сохранённого контекста.
- **NVS breadcrumbs** (только при `crash_data_valid: yes`):
  - `last_section_id` / `last_section_name` — какой участок кода был активен при последнем сохранении:
    - 0 = Idle/MainLoop
    - 1 = FetchSensors
    - 2 = RobonomicsDatalog
    - 3 = RobonomicsHTTPMap
    - 4 = CustomHTTP
    - 5 = DisplayUpdate
    - 6 = LEDUpdate
    - 7 = WiFiReconnect
    - 8 = SDWrite
  - `prev_uptime_sec` — сколько устройство проработало (на момент последнего сохранения, каждые 30 с).
  - `prev_free_heap` — доступная память на момент последнего сохранения.
- **Текущее состояние** при загрузке: свободная память, WiFi RSSI.

**Как это работает:** Каждые 30 секунд прошивка сохраняет текущую секцию кода, uptime и свободную память в NVS flash. При panic или watchdog перезагрузке эти данные сохраняются и считываются при следующей загрузке.

**Пример boot‑файла:**

```
reset_reason: Panic reset (e.g., unhandled exception)
reset_reason_code: 4
boot_counter: 123
crash_data_valid: yes
prev_uptime_sec: 86420
prev_free_heap: 45000
last_section_id: 3
last_section_name: RobonomicsHTTPMap
current_free_heap: 214000
rssi: -65
```

Это говорит о том, что:

- Сбой был настоящей паникой (код 4).
- Устройство проработало ~24 часа до сбоя.
- Последний известный участок кода — Robonomics HTTP Map API.
- Память была на исходе (45 КБ) перед сбоем — возможна проблема с памятью.

### 6. Отладка через PlatformIO (JTAG)

Поддерживаемые платы ESP32-C3/C6 имеют **встроенный USB JTAG** — внешний адаптер
не нужен, когда этот интерфейс доступен. `_debug`‑окружения уже настроены для
его использования.

#### 6.1. Конфигурация отладки в platformio.ini

`_debug`‑окружения включают следующие опции ([документация PlatformIO Debugging](https://docs.platformio.org/en/latest/plus/debugging.html)):

```ini
build_type = debug
debug_tool = esp-builtin       ; Использовать встроенный USB JTAG
debug_init_break = tbreak setup ; Остановиться на setup() при запуске
```

Флаги сборки для отладки:

- `-g -Og -fno-inline` — символы отладки, оптимизация для отладки
- `-DUSING_JTAG_DEBUGGER_PINS=1` — резервирование пинов JTAG
- `-DDEBUG_ESP_PORT=Serial` — направление логов фреймворка в `Serial`
- `-DCORE_DEBUG_LEVEL=4` — логи Arduino framework на уровне 4
- `-DALTRUIST_BUILD_DEBUG` — тяжёлая диагностическая функциональность
- `-DALTRUIST_DEFAULT_LOG_LEVEL=4` — начальное значение `cfg::debug`
- `-DALTRUIST_FORCE_LOG_LEVEL=4` — принудительный минимум для логов проекта
- `-DALTRUIST_HEALTH_TELEMETRY` — периодические health snapshots для тестера
- `NDEBUG` намеренно не задаётся, чтобы сохранялись проверки `assert` и отладочные пути библиотек

#### 6.2. Запуск сеанса отладки

**Из VS Code:**

1. Выберите `_debug`‑окружение в боковой панели PlatformIO (например `esp32c6_inside_en_debug`).
2. Нажмите иконку **Debug** на боковой панели, или **F5**, или **Run** → **Start Debugging**.
3. PlatformIO автоматически соберёт, прошьёт и запустит GDB.

**Из командной строки:**

```bash
# Запустить отладку
pio debug -e esp32c6_inside_en_debug

# Или сначала собрать, потом отлаживать
pio run -e esp32c6_inside_en_debug
pio debug -e esp32c6_inside_en_debug --interface=gdb
```

#### 6.3. Доступные функции отладки

После подключения отладчика можно:

- **Ставить точки останова** — кликните рядом с нужной строкой в `airrohr-firmware.ino`, `display_manager.cpp`, `http_altruist_sensor.cpp` и др.
- **Пошаговое выполнение** — F10 (шаг через), F11 (шаг внутрь), Shift+F11 (шаг наружу).
- **Инспекция переменных** — просмотр `sensors_data`, `deviceStatus`, `crash_last_section`, использования памяти.
- **Watch‑выражения** — добавьте выражения вроде `ESP.getFreeHeap()`.
- **Стек вызовов** — просмотр полного стека вызовов при остановке.
- **Регистры периферии** — инспекция состояния аппаратных регистров (для продвинутых).

#### 6.4. Использование внешнего адаптера (ESP-PROG)

Если нужен внешний JTAG‑адаптер вместо встроенного USB JTAG:

1. Подключите ESP-PROG к JTAG‑пинам целевой платы.
2. Обновите окружение в `platformio.ini`:

```ini
debug_tool = esp-prog
upload_protocol = esp-prog
```

Подробности подключения — в [документации PlatformIO ESP-Prog](https://docs.platformio.org/en/latest/plus/debug-tools/esp-prog.html).

#### 6.5. Решение проблем с отладкой

- **«No debug adapter found»** — убедитесь, что USB‑кабель поддерживает передачу данных (не только зарядка). Попробуйте другой USB‑порт.
- **Точки останова не срабатывают** — убедитесь, что используете `_debug`‑окружение с `build_type = debug`.
- **Отладка медленная** — добавьте `debug_speed = 5000` в platformio.ini для увеличения скорости JTAG.

### 7. Типичные сценарии отладки

#### 7.1. Расследование зависаний экрана на Insight

1. Прошейте `_debug` сборку Insight (например `esp32c6_inside_en_debug`).
2. Дайте устройству поработать как обычно, с вставленной SD‑картой.
3. Когда заметите зависание экрана:
   - По возможности **не** отключайте питание сразу; сначала проверьте, работают ли светодиоды / сеть.
   - Затем перезагрузите устройство.
4. Извлеките SD‑карту и проверьте:
   - Последний `/exceptions/runtime.log` — ищите:
     - Сообщения `[Display]` / `[EPD]` перед зависанием.
     - Предупреждения `[MEM][WARN]` о малой памяти.
     - Повторяющиеся ошибки HTTP или SD.
   - Последний `/exceptions/boot_*.txt` — проверьте причину перезагрузки и последнюю секцию кода.

#### 7.2. Расследование отсутствия данных Urban или проблем обнаружения

1. Используйте `_debug` сборку Urban или Insight.
2. Ищите строки с `HTTPAltruistSensor` в serial или SD логах:
   - Попытки переоткрытия.
   - Достижение максимума попыток обнаружения.
   - Коды ошибок HTTP.
3. Убедитесь, что `/data.json` Urban доступен (через браузер или `curl`) пока устройство работает.

#### 7.3. Отслеживание утечек памяти или фрагментации

1. Запустите `_debug` сборку Insight с включённым SD логированием.
2. Дайте устройству поработать несколько дней.
3. Периодически проверяйте `/exceptions/runtime.log`:
   - Следите за серией `[MEM] Free heap bytes` во времени.
   - Ищите события `[MEM][WARN]`.
4. Если свободная память постоянно уменьшается без восстановления — скорее всего, утечка памяти в одной из задач (например JSON‑аллокации, API‑клиенты или буферы дисплея).

---

При обнаружении бага наиболее полезные артефакты для сбора:

- Последний `/exceptions/runtime.log` (и `.1`, `.2` если есть).
- Последние один‑два файла `/exceptions/boot_*.txt`.
- Точное название окружения прошивки (например `esp32c6_inside_en_debug`).

Приложите их к issue или поделитесь при обращении за помощью — обычно этого достаточно для быстрой диагностики.

---

## Шпаргалка

### Основные команды

```bash
# Собрать debug‑прошивку
pio run -e esp32c6_inside_en_debug

# Собрать и прошить
pio run -e esp32c6_inside_en_debug -t upload

# Смотреть serial‑логи (115200 бод)
pio device monitor

# Сохранить serial‑логи в файл (длительная отладка)
pio device monitor | tee "debug_$(date +%Y%m%d_%H%M%S).log"

# Авто‑переподключение и логирование (на несколько дней)
while true; do pio device monitor | tee -a "debug_$(date +%Y%m%d).log"; sleep 2; done

# Запустить JTAG‑отладчик из CLI
pio debug -e esp32c6_inside_en_debug

# Очистить сборку (если что-то странное)
pio run -e esp32c6_inside_en_debug -t clean
```

### Горячие клавиши VS Code

| Действие           | Клавиша                 |
| ------------------ | ----------------------- |
| Начать отладку     | **F5**                  |
| Остановить отладку | **Shift + F5**          |
| Шаг через          | **F10**                 |
| Шаг внутрь         | **F11**                 |
| Шаг наружу         | **Shift + F11**         |
| Продолжить         | **F5**                  |
| Точка останова     | **F9** или клик на поле |

### Уровни логирования

| Уровень   | Макрос                     | Когда использовать                              |
| --------- | -------------------------- | ----------------------------------------------- |
| `[ERROR]` | `debug_outln_error(...)`   | Сбои, исключения                                |
| `[INFO]`  | `debug_outln_info(...)`    | Обычные события                                 |
| `[DEBUG]` | `debug_outln_verbose(...)` | Подробная трассировка (только в `_debug` сборках) |

### Файлы логов на SD‑карте

| Файл                        | Содержимое                        |
| --------------------------- | --------------------------------- |
| `/exceptions/boot_*.txt`    | Причина перезагрузки, breadcrumbs |
| `/exceptions/runtime.log`   | Ротируемый лог (зеркало serial)   |
| `/exceptions/runtime.log.1` | Ротированная копия                |

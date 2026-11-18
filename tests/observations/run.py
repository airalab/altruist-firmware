#!/usr/bin/env python3
"""Compile/run firmware UART state tests; optionally verify with a tester checkout.

Uses the repository's installed ArduinoJson headers and a host C++ compiler.
No dependency installation, PlatformIO build, serial access or device writes.
"""

import argparse
import json
import subprocess
import sys
import tempfile
from collections import defaultdict
from datetime import UTC, datetime, timedelta
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--arduino-json", type=Path, help="ArduinoJson 6 src directory")
    parser.add_argument(
        "--tester-root", type=Path, help="Tester checkout for wire/ledger checks"
    )
    args = parser.parse_args()
    headers = args.arduino_json
    if headers is None:
        headers = next(
            (ROOT / ".pio/libdeps").glob("*/ArduinoJson/src/ArduinoJson.h"), None
        )
        if headers is None:
            parser.error(
                "Supply --arduino-json; installed ArduinoJson headers not found"
            )
        headers = headers.parent
    if not (headers / "ArduinoJson.h").is_file():
        parser.error("--arduino-json must contain ArduinoJson.h")

    with tempfile.TemporaryDirectory(prefix="altruist-uart-v2-") as directory:
        binary = str(Path(directory) / "uart-v2")
        subprocess.run(
            [
                "g++",
                "-std=c++11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-pedantic",
                "-DALTRUIST_BUILD_DEBUG",
                "-I" + str(ROOT),
                "-I" + str(headers),
                str(Path(__file__).with_name("test_uart_v2.cpp")),
                "-o",
                binary,
            ],
            check=True,
        )
        output = subprocess.check_output([binary], text=True)
        tests = ROOT / "tests/observations"
        adapter = str(Path(directory) / "sensor-adapter")
        subprocess.run(
            [
                "g++",
                "-std=c++11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-pedantic",
                "-DALTRUIST_BUILD_DEBUG",
                "-Dutils_h",
                "-DARDUINOJSON_ENABLE_ARDUINO_STRING=1",
                "-I" + str(tests / "stubs"),
                "-I" + str(ROOT),
                "-I" + str(headers),
                str(tests / "test_sensor_adapter.cpp"),
                "-o",
                adapter,
            ],
            check=True,
        )
        output += subprocess.check_output([adapter], text=True)
        i2s = str(Path(directory) / "i2s-probe")
        drivers = ROOT / "sensors/drivers/i2s_noise"
        subprocess.run(
            [
                "g++",
                "-std=gnu++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-Wno-unused-variable",
                "-include",
                "Arduino.h",
                "-DALTRUIST_BUILD_DEBUG",
                "-DESP32",
                "-D__DEFINES_H__",
                "-DI2S_PIN_BCLK=7",
                "-DI2S_PIN_WS=6",
                "-DI2S_PIN_DOUT=-1",
                "-DI2S_PIN_DIN=8",
                "-I" + str(tests / "stubs"),
                "-I" + str(ROOT),
                str(tests / "test_i2s_probe.cpp"),
                *[
                    str(drivers / name)
                    for name in (
                        "i2s_noise.cpp",
                        "soundsensor.cpp",
                        "arduinoFFT.cpp",
                        "measurement.cpp",
                    )
                ],
                "-o",
                i2s,
            ],
            check=True,
        )
        subprocess.run([i2s], check=True)
        # No telemetry implementation is compiled into a release translation unit.
        released = subprocess.check_output(
            ["g++", "-std=c++11", "-E", "-P", "-x", "c++", "-I" + str(ROOT), "-"],
            input='#include "sensors/uart_v2_json.h"\nint main() { return 0; }\n',
            text=True,
        )
        assert released.strip() == "int main() { return 0; }", released

    streams = defaultdict(list)
    for line in output.splitlines():
        scenario, raw = line.split("\t", 1)
        streams[scenario].append(raw)
    print(
        f"PASS firmware host assertions; {sum(map(len, streams.values()))} snapshots, "
        f"{len(streams)} streams; release preprocessing"
    )
    if args.tester_root is None:
        print("Tester integration not requested (use --tester-root)")
        return

    if not (args.tester_root / "altruist_tester/sensor_contract.py").is_file():
        parser.error("--tester-root must contain the UART v2 tester checkout")
    sys.path.insert(0, str(args.tester_root.resolve()))
    from altruist_tester.sensor_observations import (
        SensorObservationTracker,
        observation_findings,
    )

    origin = datetime(2026, 11, 18, tzinfo=UTC)
    trackers = {}
    for scenario, lines in streams.items():
        tracker = SensorObservationTracker(
            capture_started_at=origin,
            activity_only=("ICS43434.main",)
            if scenario == "probe_noise"
            else ("BME680.gas",)
            if scenario == "probe_gas"
            else (),
        )
        for raw in lines:
            time = int(json.loads(raw)["_observations"]["time_ms"])
            tracker.feed(raw, origin + timedelta(milliseconds=time))
            assert not tracker.issues, (scenario, tracker.issues)
            assert tracker.profile_error is None, (scenario, tracker.profile_error)
            if (
                scenario.startswith("probe_")
                and next(iter(json.loads(raw)["_observations"]["probes"].values()))[
                    "result"
                ]
                == "activity"
            ):
                findings = observation_findings(tracker.as_dict(), reference_time=None)
                assert not any(f["severity"] == "fail" for f in findings), (
                    scenario,
                    findings,
                )
                assert any(f["code"] == "SENSOR_ACTIVITY_OBSERVED" for f in findings)

        trackers[scenario] = tracker

    for count, severity in ((600, None), (599, "warn"), (300, "warn"), (299, "fail")):
        findings = observation_findings(
            trackers[f"rate_{count}"].as_dict(), reference_time=None
        )
        rates = [f["severity"] for f in findings if f["code"] == "SENSOR_NOISE_RATE"]
        assert rates == ([] if severity is None else [severity]), (count, rates)

    assert trackers["gas"].coverage["failure_tail_eviction"] > 0
    noise = trackers["noise"].as_dict()
    assert any(
        f["code"] == "SENSOR_NOISE_GAP" and f["severity"] == "fail"
        for f in observation_findings(noise, reference_time=None)
    )
    # Equal readings are distinct measurements; no intermediate cached samples.
    traffic = trackers["traffic"]
    assert traffic.snapshots == 3 and traffic.duplicates == 0
    assert all(item["seq"] == 2 for item in traffic.latest.values())

    # Use the actual supplied profiles, including their normalized null fields.
    from altruist_tester.config import load_tester_config

    for filename in ("urban.example.toml", "insight.example.toml"):
        profile = load_tester_config(args.tester_root / "configs" / filename)
        expected = profile.observation_settings
        assert expected
        for sensor in expected:
            tracker = SensorObservationTracker(expected_settings=expected)
            for raw in streams["profile_" + sensor]:
                tracker.feed(raw, origin)
            assert not tracker.issues, (filename, sensor, tracker.issues)
            assert tracker.profile_error is None, (
                filename,
                sensor,
                tracker.profile_error,
            )
    print(
        "PASS tester parser/transitions, failure-tail coverage, "
        "equal-value identities, 2/1 Hz boundaries and sticky gap FAIL"
    )


if __name__ == "__main__":
    main()

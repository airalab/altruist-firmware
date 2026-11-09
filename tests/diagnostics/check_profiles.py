"""Static profile checks; does not invoke PlatformIO, compile, or access a device."""
import configparser
import os
from pathlib import Path
import re
import shlex
import subprocess
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
CONFIG = configparser.ConfigParser(interpolation=None)
CONFIG.read(ROOT / "platformio.ini")


def flags(section, option="build_flags"):
    value = CONFIG[section][option]
    return re.sub(r"\$\{([^}.]+)\.([^}]+)\}", lambda m: flags(m[1], m[2]), value)


class BuildEnv(dict):
    def __init__(self, section):
        super().__init__(BUILD_FLAGS=flags(section))
        self.section = section

    def GetProjectOption(self, name, default=""):
        return CONFIG[self.section].get(name, default)

    def ParseFlags(self, value):
        return {"CPPDEFINES": [s[2:].split("=", 1)[0] for s in shlex.split(value) if s.startswith("-D")]}

    def StringifyMacro(self, value):
        return '"' + value + '"'

    def subst(self, value):
        return value.replace("$PROJECT_DIR", str(ROOT))

    def Append(self, **kwargs):
        for key, value in kwargs.items():
            self.setdefault(key, []).extend(value)

    def AddBuildMiddleware(self, callback):
        self.middleware = callback


def preprocess(path, defines):
    # This is a policy check, not a C++ build: platform headers are intentionally
    # omitted. The real project guards are evaluated by the host preprocessor.
    source = re.sub(r'^\s*#include[^\n]*', '', path.read_text(), flags=re.M)
    return subprocess.check_output(
        ["cpp", "-P", "-x", "c++", "-include", str(ROOT / "diagnostics.h"),
         *["-D" + d for d in defines], "-"], input=source, text=True,
    )


class DiagnosticsProfiles(unittest.TestCase):
    def test_metadata_matrix(self):
        script = (ROOT / "platformio_build_metadata.py").read_text()
        targets = [s for s in CONFIG.sections() if s.startswith("env:")]
        self.assertEqual(len(targets), 12)
        for section in targets:
            for channel in ("0", "1"):
                for health in ("0", "1"):
                    with self.subTest(section=section, channel=channel, health=health):
                        env = BuildEnv(section)
                        namespace = {"env": env, "Import": lambda _: None}
                        with patch.dict(os.environ, {
                            "ALTRUIST_CHANNEL_TESTING": channel,
                            "ALTRUIST_HEALTH_TELEMETRY": health,
                            "ALTRUIST_BUILD_COMMIT": "344f123",
                        }), patch("subprocess.check_output", return_value="esp32"), patch("builtins.print"):
                            exec(compile(script, "platformio_build_metadata.py", "exec"), namespace)
                        debug = section.endswith("_debug")
                        metadata = dict(d for d in env["CPPDEFINES"] if isinstance(d, tuple))
                        self.assertEqual(metadata["ALTRUIST_BUILD_PROFILE"], '"debug"' if debug else '"release"')
                        self.assertEqual(namespace["health_telemetry"], debug and health == "1")
                        self.assertEqual(namespace["testing_channel"], channel == "1")

    def test_release_output_and_functional_paths(self):
        sources = [ROOT / "airrohr-firmware.ino", *ROOT.glob("apis/**/*.cpp"),
                   *ROOT.glob("sensors/**/*.cpp"), *ROOT.glob("display/**/*.cpp")]
        sources = [p for p in sources if re.search(r'\bSerial\.', p.read_text())]
        for model, target in (("URBAN", "ESP32C3"), ("URBAN", "ESP32C6"), ("INSIGHT", "ESP32C6")):
            for language in ("EN", "RU"):
                for channel in ([], ["ALTRUIST_CHANNEL_TESTING"]):
                    defines = ["ESP32", "ALTRUIST_" + model, "CONFIG_IDF_TARGET_" + target,
                               "INTL_" + language, "USE_SD_CARD", "ALTRUIST_HEALTH_TELEMETRY",
                               "CORE_DEBUG_LEVEL=5", "LOG_LOCAL_LEVEL=5", "ALTRUIST_FORCE_LOG_LEVEL=5", *channel]
                    for path in sources:
                        with self.subTest(path=path, defines=defines):
                            output = preprocess(path, defines)
                            self.assertNotRegex(output, r'\bSerial\.(?:print\w*|flush)\(')
                            if path.name == "airrohr-firmware.ino":
                                self.assertIn("checkSystemWatchdogs();", output)
                                self.assertIn("saveCrashContext();", output)
                                self.assertIn("metricsWorker,", output)
                                self.assertNotIn("writeBootFile", output)
                    output = preprocess(ROOT / "improv/improv_serial.cpp", defines)
                    self.assertIn("Serial.write(buf, pos);", output)
                    output = preprocess(ROOT / "utils.cpp", defines)
                    self.assertIn("HardwareSerial::write(data, len)", output)
                    self.assertNotIn('"[SUBSYSTEM] "', output)
                    self.assertNotIn('"[HEALTH] uptime="', output)
                    self.assertIn("project_diagnostics_enabled = false", output)

    def test_debug_sensor_snapshots(self):
        for name in ("bmx280i2c", "sds011", "i2s_noise", "scd4x", "bmx680i2c"):
            with self.subTest(sensor=name):
                output = preprocess(ROOT / f"sensors/{name}_sensor.cpp", ["ALTRUIST_BUILD_DEBUG", "ESP32"])
                self.assertIn("serializeJson(data, Serial);", output)
        output = preprocess(ROOT / "sensors/bmx680i2c_sensor.cpp", ["ALTRUIST_BUILD_DEBUG", "ESP32"])
        for field in ("voc_index", "voc_level", "voc_spikes_today", "gas_resistance", "gas_baseline"):
            self.assertIn('"' + field + '"', output)

    def test_debug_structured_events(self):
        paths = ["airrohr-firmware.ino", "utils.cpp", "apis/helpers/message_formatter.cpp",
                 "apis/robonomics_http_api.cpp", "apis/robonomics_datalog_api.cpp"]
        output = "\n".join(preprocess(ROOT / p, ["ALTRUIST_BUILD_DEBUG", "ALTRUIST_HEALTH_TELEMETRY", "ESP32"])
                           for p in paths)
        for tag in ("BUILD", "BOOT", "LOG", "HEALTH", "PAYLOAD", "DATALOG", "CONNECTIVITY", "SUBSYSTEM"):
            self.assertIn("[" + tag + "]", output)
        for debug in (False, True):
            output = preprocess(ROOT / "library_diagnostics.h", ["ALTRUIST_BUILD_DEBUG"] if debug else [])
            self.assertEqual("class AltruistSilentLibrarySerial" in output, not debug)

    def test_core_levels_are_compile_time_clamped(self):
        with subprocess.Popen(["cpp", "-dM", "-DCORE_DEBUG_LEVEL=5", "-DLOG_LOCAL_LEVEL=5",
                               str(ROOT / "diagnostics.h")], stdout=subprocess.PIPE, text=True) as process:
            output = process.communicate()[0]
            self.assertEqual(process.returncode, 0)
        self.assertIn("#define CORE_DEBUG_LEVEL 0", output)
        self.assertIn("#define LOG_LOCAL_LEVEL 0", output)


if __name__ == "__main__":
    unittest.main()

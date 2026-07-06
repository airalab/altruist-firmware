Import("env")

import os
import re
import subprocess


_TRUE_VALUES = {"1", "true", "yes", "on"}
_FALSE_VALUES = {"", "0", "false", "no", "off"}
_COMMIT_PATTERN = re.compile(r"^[0-9a-fA-F]{7,40}$")


def _read_optional_boolean_env(name):
    raw_value = os.getenv(name)
    if raw_value is None:
        return None

    value = raw_value.strip().lower()
    if value in _TRUE_VALUES:
        return True
    if value in _FALSE_VALUES:
        return False
    raise ValueError(f"{name} must be a boolean value, got {value!r}")


def _project_option(name):
    value = env.GetProjectOption(name, "").strip().lower()
    if not value:
        raise ValueError(f"Missing required build metadata: {name}")
    return value


def _git_output(*args):
    try:
        return subprocess.check_output(
            ["git", *args],
            cwd=env.subst("$PROJECT_DIR"),
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip()
    except (OSError, subprocess.CalledProcessError):
        return ""


def _infer_testing_channel(branch):
    if not branch:
        return False, "local branch default (detached/unknown -> stable)"
    return branch != "esp32", "local branch default"


esp_target = _project_option("custom_esp_target")
model = _project_option("custom_model")
language = _project_option("custom_language")
build_profile = env.GetBuildType()
git_branch = _git_output("symbolic-ref", "--short", "HEAD")

defines = [
    ("ALTRUIST_BUILD_TARGET", env.StringifyMacro(esp_target)),
    ("ALTRUIST_BUILD_MODEL", env.StringifyMacro(model)),
    ("ALTRUIST_BUILD_LANGUAGE", env.StringifyMacro(language)),
    ("ALTRUIST_BUILD_PROFILE", env.StringifyMacro(build_profile)),
]
configured_testing_channel = _read_optional_boolean_env(
    "ALTRUIST_CHANNEL_TESTING"
)
if configured_testing_channel is None:
    testing_channel, channel_source = _infer_testing_channel(git_branch)
else:
    testing_channel = configured_testing_channel
    channel_source = "ALTRUIST_CHANNEL_TESTING"

firmware_channel = "testing" if testing_channel else "stable"
env["ALTRUIST_ARTIFACT_CHANNEL"] = "testing" if testing_channel else "stable"

if testing_channel:
    defines.append("ALTRUIST_CHANNEL_TESTING")

configured_health_telemetry = _read_optional_boolean_env(
    "ALTRUIST_HEALTH_TELEMETRY"
)
health_telemetry = (
    True
    if configured_health_telemetry is None
    else configured_health_telemetry
)
if health_telemetry:
    defines.append("ALTRUIST_HEALTH_TELEMETRY")

build_commit = os.getenv("ALTRUIST_BUILD_COMMIT", "").strip()
commit_source = "ALTRUIST_BUILD_COMMIT"
if not build_commit:
    build_commit = _git_output("rev-parse", "--short=7", "HEAD")
    commit_source = "local Git"

if build_commit:
    if not _COMMIT_PATTERN.fullmatch(build_commit):
        raise ValueError(
            "ALTRUIST_BUILD_COMMIT must contain a 7-40 character hexadecimal SHA"
        )
    defines.append(
        ("ALTRUIST_BUILD_COMMIT", env.StringifyMacro(build_commit[:7].lower()))
    )

env.Append(CPPDEFINES=defines)
print("Altruist build:")
print(f"  source: {channel_source}")
print(f"  branch: {git_branch or 'detached/unknown'}")
print(f"  firmware channel: {firmware_channel}")
print(f"  profile: {build_profile}")
print(f"  health telemetry: {'enabled' if health_telemetry else 'disabled'}")
print(f"  commit: {build_commit[:7].lower() if build_commit else 'unknown'}")
print(f"  commit source: {commit_source}")

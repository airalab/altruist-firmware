Import("env")

import hashlib
import os
import pathlib
import shutil


_ARTIFACT_PREFIXES = {
    ("esp32c3", "urban"): "latest32c3",
    ("esp32c6", "urban"): "latest32c6urb",
    ("esp32c6", "insight"): "latest32c6ins",
}


def _project_option(name):
    value = env.GetProjectOption(name, "").strip().lower()
    if not value:
        raise ValueError(f"Missing required artifact metadata: {name}")
    return value


def _write_artifact(source_path, output_dir, file_name, digest):
    output_path = output_dir / file_name
    shutil.copy2(source_path, output_path)
    output_path.with_suffix(output_path.suffix + ".md5").write_text(
        f"{digest}\n",
        encoding="ascii",
    )

# Touch html-content.h so PlatformIO recompiles files that include it.
# This keeps __DATE__ fresh instead of stale from cached object files.
pathlib.Path(os.path.join(env.subst("$PROJECT_DIR"), "webserver", "html-content.h")).touch()

def after_build(source, target, env):
    environment_name = env.subst("$PIOENV")
    publish_artifacts = _project_option("custom_publish_artifacts")
    if publish_artifacts == "no":
        print(
            "Program has been built; publishable artifacts are disabled:",
            environment_name,
        )
        return
    if publish_artifacts != "yes":
        raise ValueError(
            "custom_publish_artifacts must be either 'yes' or 'no', "
            f"got {publish_artifacts!r}"
        )

    esp_target = _project_option("custom_esp_target")
    model = _project_option("custom_model")
    language = _project_option("custom_language")
    try:
        artifact_prefix = _ARTIFACT_PREFIXES[(esp_target, model)]
    except KeyError as exc:
        raise ValueError(
            f"Unsupported artifact target/model: {esp_target}/{model}"
        ) from exc

    channel = env.get("ALTRUIST_ARTIFACT_CHANNEL", "stable")
    if channel not in {"stable", "testing"}:
        raise ValueError(f"Unsupported artifact channel: {channel!r}")

    source_path = pathlib.Path(target[0].path)
    output_dir = pathlib.Path(env.subst("$PROJECT_BUILD_DIR"))
    output_dir.mkdir(parents=True, exist_ok=True)
    with source_path.open("rb") as source_file:
        digest = hashlib.file_digest(source_file, "md5").hexdigest()

    channel_suffix = "_testing" if channel == "testing" else ""
    artifact_name = f"{artifact_prefix}_{language}{channel_suffix}.bin"
    _write_artifact(source_path, output_dir, artifact_name, digest)

    compatibility_names = []
    if channel == "testing":
        compatibility_names.append(f"{artifact_prefix}_{language}_dev.bin")

    for compatibility_name in compatibility_names:
        _write_artifact(source_path, output_dir, compatibility_name, digest)

    print(
        "Published firmware artifact:",
        artifact_name,
        f"({channel}, {esp_target}/{model}, {language})",
    )

env.AddPostAction("$BUILD_DIR/firmware.bin", after_build)

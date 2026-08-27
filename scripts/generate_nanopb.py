#!/usr/bin/env python3
"""Generate nanopb C sources from vendored sensors.social proto files."""

from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
import urllib.request
import zipfile
from pathlib import Path

NANOPB_VERSION = "0.4.9.1"
NANOPB_ZIP = (
    f"https://github.com/nanopb/nanopb/archive/refs/tags/{NANOPB_VERSION}.zip"
)

ROOT = Path(__file__).resolve().parents[1]
PROTO_DIR = ROOT / "proto"
OUT_DIR = ROOT / "apis" / "helpers" / "proto" / "generated"
OPTIONS = PROTO_DIR / "sensors_social.options"

PROTO_FILES = [
    PROTO_DIR / "sensor" / "v1" / "measurement.proto",
    PROTO_DIR / "sensor" / "v1" / "sensor.proto",
    PROTO_DIR / "crypto" / "v1" / "encrypted.proto",
    PROTO_DIR / "crypto" / "v1" / "envelope.proto",
    PROTO_DIR / "device" / "v1" / "urban.proto",
    PROTO_DIR / "device" / "v1" / "insight.proto",
    PROTO_DIR / "core" / "v1" / "message.proto",
]


def find_generator() -> Path | None:
    env = shutil.which("nanopb_generator")
    if env:
        return Path(env)
    local = ROOT / ".tools" / "nanopb" / "generator" / "nanopb_generator.py"
    if local.exists():
        return local
    return None


def install_nanopb(dest: Path) -> Path:
    dest.mkdir(parents=True, exist_ok=True)
    zip_path = dest / "nanopb.zip"
    print(f"Downloading nanopb {NANOPB_VERSION}…")
    urllib.request.urlretrieve(NANOPB_ZIP, zip_path)
    with zipfile.ZipFile(zip_path) as zf:
        zf.extractall(dest)
    extracted = dest / f"nanopb-{NANOPB_VERSION}"
    generator = extracted / "generator" / "nanopb_generator.py"
    if not generator.exists():
        raise FileNotFoundError(f"nanopb_generator.py not found in {extracted}")
    return generator


def main() -> int:
    generator = find_generator()
    tmp: tempfile.TemporaryDirectory[str] | None = None
    if generator is None:
        tools = ROOT / ".tools"
        tools.mkdir(exist_ok=True)
        cached = tools / f"nanopb-{NANOPB_VERSION}" / "generator" / "nanopb_generator.py"
        if cached.exists():
            generator = cached
        else:
            generator = install_nanopb(tools)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for stale in OUT_DIR.glob("*"):
        if stale.is_file():
            stale.unlink()

    cmd = [
        sys.executable,
        str(generator),
        f"-I{PROTO_DIR}",
        f"-D{OUT_DIR}",
        f"-f{OPTIONS}",
        *[str(p.relative_to(PROTO_DIR)) for p in PROTO_FILES],
    ]
    print(" ".join(cmd))
    completed = subprocess.run(cmd, cwd=PROTO_DIR, check=False)
    if completed.returncode != 0:
        return completed.returncode

    # Protobuf field names `public` / `private` are C++ keywords.
    # Rename the generated C identifiers; wire tags stay 1/2.
    replacements = (
        (" public[12]", " public_items[12]"),
        (" private[2]", " private_items[2]"),
        (" public_count;", " public_items_count;"),
        (" private_count;", " private_items_count;"),
        ("MESSAGE,  public,", "MESSAGE,  public_items,"),
        ("MESSAGE,  private,", "MESSAGE,  private_items,"),
        ("_public_MSGTYPE", "_public_items_MSGTYPE"),
        ("_private_MSGTYPE", "_private_items_MSGTYPE"),
    )
    for header in (
        OUT_DIR / "device" / "v1" / "urban.pb.h",
        OUT_DIR / "device" / "v1" / "insight.pb.h",
    ):
        text = header.read_text(encoding="utf-8")
        for old, new in replacements:
            text = text.replace(old, new)
        header.write_text(text, encoding="utf-8")

    (OUT_DIR / "GENERATED.md").write_text(
        f"Generated with nanopb {NANOPB_VERSION}. Do not edit by hand.\n"
        "Refresh: python3 scripts/generate_nanopb.py\n"
        "\n"
        "These `.pb.c` / `.pb.h` files are C structs for the schemas in repo `proto/`\n"
        "(same messages as the buf docs). The firmware encoder is `proto_codec.cpp`.\n"
        "\n"
        "C++ keywords `public` / `private` are renamed in headers to `public_items` /\n"
        "`private_items`. Protobuf field numbers stay 1 and 2.\n",
        encoding="utf-8",
    )
    print(f"Wrote {OUT_DIR}")
    if tmp:
        tmp.cleanup()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

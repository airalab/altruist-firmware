#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${TMPDIR:-/tmp}/altruist-meshtastic-frame-test"

c++ -std=c++17 -Wall -Wextra -Werror \
	-I "${ROOT}/tests/meshtastic" \
	-I "${ROOT}/apis/helpers" \
	"${ROOT}/tests/meshtastic/mbedtls/md.cpp" \
	"${ROOT}/apis/helpers/meshtastic_frame.cpp" \
	"${ROOT}/tests/meshtastic/test_frame.cpp" \
	-o "${OUT}"

"${OUT}"

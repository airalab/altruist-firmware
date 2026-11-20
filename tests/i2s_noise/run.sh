#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${TMPDIR:-/tmp}/altruist-i2s-noise-valid-test"

c++ -std=c++17 -Wall -Wextra -Werror \
	-I "${ROOT}/sensors/drivers/i2s_noise" \
	"${ROOT}/tests/i2s_noise/test_read_valid.cpp" \
	-o "${OUT}"

"${OUT}"

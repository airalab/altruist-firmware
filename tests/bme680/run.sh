#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${TMPDIR:-/tmp}/altruist-bme680-gas-flags-test"

c++ -std=c++17 -Wall -Wextra -Werror \
	-I "${ROOT}/sensors/drivers/bme680" \
	"${ROOT}/tests/bme680/test_gas_flags.cpp" \
	-o "${OUT}"

"${OUT}"

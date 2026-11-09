#!/usr/bin/env bash
# Update the Altruist STABLE firmware served by the OTA update server
# (upd.sensors.robonomics.network) from the webflasher checkout.
#
# Replicates the manual procedure from ~/upd.sh on the server:
#   1. sync ~/htdocs/webflasher to origin/main
#   2. create a dated snapshot dir inside each channel dir
#   3. copy the stable artifacts from webflasher/firmware/Altruist
#   4. repoint the `latest` symlink
#   5. verify the served checksums against webflasher
#
# Designed to run ON the OTA server (user robonomics-upd-sensors), executed as:
#   ssh <user>@<host> 'bash -s' < scripts/ota-update-server.sh
#
# Rollback: in the affected channel dir run `ln -sfn <previous_dir>/ latest`.

set -euo pipefail

HTDOCS="$HOME/htdocs"
WEBFLASHER_DIR="$HTDOCS/webflasher"
WEBFLASHER_FW="$WEBFLASHER_DIR/firmware/Altruist"
UPD_ROOT="$HTDOCS/upd.sensors.robonomics.network"

# Snapshot naming convention used on the server: <date>_09-00, Moscow time.
STAMP="$(TZ=Europe/Moscow date +%Y%m%d)_09-00"

# channel dir (relative to UPD_ROOT) -> stable artifacts to copy
CHANNELS=(
	"insight/esp32c6 latest32c6ins_en latest32c6ins_ru"
	"urban/esp32c3 latest32c3_en latest32c3_ru"
	"urban/esp32c6 latest32c6urb_en latest32c6urb_ru"
)

echo "==> Syncing webflasher checkout to origin/main"
git -C "$WEBFLASHER_DIR" fetch origin main
git -C "$WEBFLASHER_DIR" reset --hard origin/main
webflasher_head="$(git -C "$WEBFLASHER_DIR" rev-parse --short HEAD)"
echo "    webflasher HEAD: $webflasher_head"

for entry in "${CHANNELS[@]}"; do
	read -r dir artifacts <<< "$entry"
	dest="$UPD_ROOT/$dir/$STAMP"
	echo "==> $dir -> $dest"
	mkdir -p "$dest"
	for artifact in $artifacts; do
		# copies .bin, .bin.md5 and .version when present
		cp -f "$WEBFLASHER_FW/$artifact".* "$dest/"
	done
	ln -sfn "$STAMP/" "$UPD_ROOT/$dir/latest"
done

echo "==> Ensuring root-level symlinks (what sensors actually download)"
for entry in "${CHANNELS[@]}"; do
	read -r dir artifacts <<< "$entry"
	for artifact in $artifacts; do
		ln -sfn "./$dir/latest/${artifact}.bin" "$UPD_ROOT/${artifact}.bin"
		ln -sfn "./$dir/latest/${artifact}.bin.md5" "$UPD_ROOT/${artifact}.bin.md5"
		ln -sfn "./$dir/latest/${artifact}.version" "$UPD_ROOT/${artifact}.version"
	done
done

echo "==> Verifying served files against webflasher checksums"
failed=0
for artifact in latest32c6ins_en latest32c6ins_ru \
	latest32c3_en latest32c3_ru \
	latest32c6urb_en latest32c6urb_ru; do
	# firmware downloads <artifact>.bin and checks <artifact>.bin.md5
	served="$UPD_ROOT/${artifact}.bin"
	expected="$(tr -d '[:space:]' < "$WEBFLASHER_FW/$artifact.bin.md5")"
	actual="$(md5sum "$served" | cut -d' ' -f1)"
	if [[ "$expected" == "$actual" ]]; then
		echo "    OK   $artifact.bin ($actual)"
	else
		echo "    FAIL $artifact.bin: expected $expected, got $actual" >&2
		failed=1
	fi
	# firmware reads <artifact>.version for the OTA version check
	if [[ ! -e "$UPD_ROOT/${artifact}.version" ]]; then
		echo "    FAIL $artifact.version: not served at web root" >&2
		failed=1
	else
		served_version="$(tr -d '[:space:]' < "$UPD_ROOT/${artifact}.version")"
		expected_version="$(tr -d '[:space:]' < "$WEBFLASHER_FW/$artifact.version")"
		if [[ "$served_version" == "$expected_version" ]]; then
			echo "    OK   $artifact.version ($served_version)"
		else
			echo "    FAIL $artifact.version: expected $expected_version, got $served_version" >&2
			failed=1
		fi
	fi
done

if [[ "$failed" -ne 0 ]]; then
	echo "Verification FAILED; roll back with: ln -sfn <previous_snapshot>/ latest" >&2
	exit 1
fi

echo "==> Done: OTA channels now serve $STAMP (webflasher $webflasher_head)"

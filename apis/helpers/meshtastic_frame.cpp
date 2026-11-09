#include "meshtastic_frame.h"

#include <mbedtls/md.h>
#include <string.h>

uint8_t meshtasticFragmentCount(size_t payload_len)
{
	if (payload_len == 0 || payload_len > MESHTASTIC_MAX_PAYLOAD_BYTES) {
		return 0;
	}
	if (payload_len <= MESHTASTIC_SINGLE_MAX_BYTES) {
		return 1;
	}
	const size_t count =
	    (payload_len + MESHTASTIC_FRAGMENT_BODY_BYTES - 1) / MESHTASTIC_FRAGMENT_BODY_BYTES;
	if (count < 2 || count > MESHTASTIC_MAX_FRAGMENTS) {
		return 0;
	}
	return static_cast<uint8_t>(count);
}

bool meshtasticMessageId(const uint8_t *payload, size_t payload_len, uint8_t id_out[MESHTASTIC_MESSAGE_ID_LEN])
{
	if (!payload || payload_len == 0 || !id_out) {
		return false;
	}
	uint8_t digest[32];
	const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	if (!info || mbedtls_md(info, payload, payload_len, digest) != 0) {
		return false;
	}
	memcpy(id_out, digest, MESHTASTIC_MESSAGE_ID_LEN);
	return true;
}

size_t meshtasticEncodeSingle(const uint8_t *payload, size_t payload_len, uint8_t *out, size_t out_cap)
{
	if (!payload || !out || payload_len < 1 || payload_len > MESHTASTIC_SINGLE_MAX_BYTES) {
		return 0;
	}
	const size_t need = 1 + payload_len;
	if (out_cap < need) {
		return 0;
	}
	out[0] = MESHTASTIC_CTRL_SINGLE;
	memcpy(out + 1, payload, payload_len);
	return need;
}

size_t meshtasticEncodeFragment(const uint8_t *payload, size_t payload_len, const uint8_t id[MESHTASTIC_MESSAGE_ID_LEN],
				uint8_t index, uint8_t count, uint8_t *out, size_t out_cap)
{
	if (!payload || !id || !out || count < 2 || count > MESHTASTIC_MAX_FRAGMENTS || index >= count) {
		return 0;
	}
	const size_t offset = static_cast<size_t>(index) * MESHTASTIC_FRAGMENT_BODY_BYTES;
	if (offset >= payload_len) {
		return 0;
	}
	size_t body = payload_len - offset;
	if (body > MESHTASTIC_FRAGMENT_BODY_BYTES) {
		body = MESHTASTIC_FRAGMENT_BODY_BYTES;
	}
	const bool last = (index + 1u) == count;
	if (!last && body != MESHTASTIC_FRAGMENT_BODY_BYTES) {
		return 0;
	}
	if (last && (body < 1 || body > MESHTASTIC_FRAGMENT_BODY_BYTES)) {
		return 0;
	}
	const size_t need = MESHTASTIC_FRAGMENT_HEADER_LEN + body;
	if (out_cap < need) {
		return 0;
	}
	out[0] = MESHTASTIC_CTRL_FRAGMENT;
	memcpy(out + 1, id, MESHTASTIC_MESSAGE_ID_LEN);
	out[7] = static_cast<uint8_t>(((count - 1u) << 4) | (index & 0x0fu));
	memcpy(out + 8, payload + offset, body);
	return need;
}

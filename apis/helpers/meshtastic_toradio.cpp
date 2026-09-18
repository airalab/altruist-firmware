#include "meshtastic_toradio.h"

#include "meshtastic_frame.h"

#include <string.h>

namespace {

size_t pbPutVarint(uint8_t *out, size_t cap, size_t off, uint32_t value)
{
	do {
		if (off >= cap) {
			return 0;
		}
		uint8_t byte = static_cast<uint8_t>(value & 0x7fu);
		value >>= 7;
		if (value) {
			byte |= 0x80u;
		}
		out[off++] = byte;
	} while (value);
	return off;
}

size_t pbPutTag(uint8_t *out, size_t cap, size_t off, uint32_t field, uint32_t wire)
{
	return pbPutVarint(out, cap, off, (field << 3) | wire);
}

size_t pbPutBytes(uint8_t *out, size_t cap, size_t off, uint32_t field, const uint8_t *data, size_t len)
{
	off = pbPutTag(out, cap, off, field, 2);
	if (!off) {
		return 0;
	}
	off = pbPutVarint(out, cap, off, static_cast<uint32_t>(len));
	if (!off || off + len > cap) {
		return 0;
	}
	memcpy(out + off, data, len);
	return off + len;
}

size_t wrapSerial(const uint8_t *inner, size_t inner_len, uint8_t *out, size_t out_cap)
{
	if (!inner || !out || inner_len == 0 || inner_len > 0xffffu || out_cap < 4 + inner_len) {
		return 0;
	}
	out[0] = MESHTASTIC_SERIAL_START1;
	out[1] = MESHTASTIC_SERIAL_START2;
	out[2] = static_cast<uint8_t>((inner_len >> 8) & 0xffu);
	out[3] = static_cast<uint8_t>(inner_len & 0xffu);
	memcpy(out + 4, inner, inner_len);
	return 4 + inner_len;
}

bool pbGetVarint(const uint8_t *in, size_t len, size_t *off, uint32_t *value)
{
	uint32_t result = 0;
	unsigned shift = 0;
	while (*off < len && shift <= 28) {
		const uint8_t byte = in[(*off)++];
		result |= static_cast<uint32_t>(byte & 0x7fu) << shift;
		if ((byte & 0x80u) == 0) {
			*value = result;
			return true;
		}
		shift += 7;
	}
	return false;
}

} // namespace

size_t meshtasticEncodeToRadioUnicast(uint32_t dest_node, uint32_t portnum, uint32_t packet_id, const uint8_t *frame,
				      size_t frame_len, uint8_t *out, size_t out_cap)
{
	if (!frame || !out || frame_len == 0 || frame_len > MESHTASTIC_TRANSPORT_MTU || packet_id == 0 || dest_node == 0 ||
	    dest_node == MESHTASTIC_BROADCAST_NODE) {
		return 0;
	}

	uint8_t data_msg[16 + MESHTASTIC_TRANSPORT_MTU];
	size_t data_len = 0;
	data_len = pbPutTag(data_msg, sizeof(data_msg), data_len, 1, 0);
	if (!data_len) {
		return 0;
	}
	data_len = pbPutVarint(data_msg, sizeof(data_msg), data_len, portnum);
	data_len = pbPutBytes(data_msg, sizeof(data_msg), data_len, 2, frame, frame_len);
	if (!data_len) {
		return 0;
	}

	uint8_t packet[96 + sizeof(data_msg)];
	size_t packet_len = 0;
	packet_len = pbPutTag(packet, sizeof(packet), packet_len, 2, 5);
	if (!packet_len || packet_len + 4 > sizeof(packet)) {
		return 0;
	}
	packet[packet_len++] = static_cast<uint8_t>(dest_node);
	packet[packet_len++] = static_cast<uint8_t>(dest_node >> 8);
	packet[packet_len++] = static_cast<uint8_t>(dest_node >> 16);
	packet[packet_len++] = static_cast<uint8_t>(dest_node >> 24);
	packet_len = pbPutBytes(packet, sizeof(packet), packet_len, 4, data_msg, data_len);
	if (!packet_len) {
		return 0;
	}
	packet_len = pbPutTag(packet, sizeof(packet), packet_len, 6, 5);
	if (!packet_len || packet_len + 4 > sizeof(packet)) {
		return 0;
	}
	packet[packet_len++] = static_cast<uint8_t>(packet_id);
	packet[packet_len++] = static_cast<uint8_t>(packet_id >> 8);
	packet[packet_len++] = static_cast<uint8_t>(packet_id >> 16);
	packet[packet_len++] = static_cast<uint8_t>(packet_id >> 24);
	packet_len = pbPutTag(packet, sizeof(packet), packet_len, 10, 0);
	packet_len = pbPutVarint(packet, sizeof(packet), packet_len, 1);
	packet_len = pbPutTag(packet, sizeof(packet), packet_len, 17, 0);
	packet_len = pbPutVarint(packet, sizeof(packet), packet_len, 1);
	if (!packet_len) {
		return 0;
	}

	uint8_t toradio[16 + sizeof(packet)];
	const size_t inner = pbPutBytes(toradio, sizeof(toradio), 0, 1, packet, packet_len);
	if (!inner) {
		return 0;
	}
	return wrapSerial(toradio, inner, out, out_cap);
}

size_t meshtasticEncodeWantConfig(uint32_t config_id, uint8_t *out, size_t out_cap)
{
	if (config_id == 0 || !out) {
		return 0;
	}
	uint8_t inner[16];
	size_t off = pbPutTag(inner, sizeof(inner), 0, 3, 0);
	off = pbPutVarint(inner, sizeof(inner), off, config_id);
	if (!off) {
		return 0;
	}
	return wrapSerial(inner, off, out, out_cap);
}

size_t meshtasticEncodeHeartbeat(uint32_t nonce, uint8_t *out, size_t out_cap)
{
	if (!out) {
		return 0;
	}
	uint8_t hb[8];
	size_t hb_len = pbPutTag(hb, sizeof(hb), 0, 1, 0);
	hb_len = pbPutVarint(hb, sizeof(hb), hb_len, nonce);
	if (!hb_len) {
		return 0;
	}
	uint8_t inner[16];
	const size_t off = pbPutBytes(inner, sizeof(inner), 0, 7, hb, hb_len);
	if (!off) {
		return 0;
	}
	return wrapSerial(inner, off, out, out_cap);
}

bool meshtasticFromRadioConfigComplete(const uint8_t *pb, size_t pb_len, uint32_t *config_id)
{
	if (!pb || !config_id || pb_len == 0) {
		return false;
	}
	size_t off = 0;
	while (off < pb_len) {
		uint32_t tag = 0;
		if (!pbGetVarint(pb, pb_len, &off, &tag)) {
			return false;
		}
		const uint32_t field = tag >> 3;
		const uint32_t wire = tag & 7u;
		if (field == 7 && wire == 0) {
			return pbGetVarint(pb, pb_len, &off, config_id);
		}
		if (wire == 0) {
			uint32_t skip = 0;
			if (!pbGetVarint(pb, pb_len, &off, &skip)) {
				return false;
			}
		} else if (wire == 1) {
			if (off + 8 > pb_len) {
				return false;
			}
			off += 8;
		} else if (wire == 2) {
			uint32_t len = 0;
			if (!pbGetVarint(pb, pb_len, &off, &len) || off + len > pb_len) {
				return false;
			}
			off += len;
		} else if (wire == 5) {
			if (off + 4 > pb_len) {
				return false;
			}
			off += 4;
		} else {
			return false;
		}
	}
	return false;
}

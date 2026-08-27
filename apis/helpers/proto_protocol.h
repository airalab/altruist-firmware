#ifndef __PROTO_PROTOCOL_H__
#define __PROTO_PROTOCOL_H__

/**
 * Shared constants for the sensors.social protobuf path.
 * Schema lives in repo `proto/` (same as buf docs). Firmware contract:
 * docs/new-protocol-firmware.md
 *
 * C6 Urban/Insight (release + debug) set `-DALTRUIST_PROTO_PROTOCOL`.
 * C3 stays CSV-only. Map dual-send is cfg::map_send_csv / map_send_proto.
 * On-chain datalog stays CSV on release (proto datalog only in debug).
 */

#include <stddef.h>
#include <stdint.h>

/** Compile-time gate: C6 builds set `-DALTRUIST_PROTO_PROTOCOL`. */
inline bool protoProtocolEnabled() {
#if defined(ALTRUIST_PROTO_PROTOCOL)
	return true;
#else
	return false;
#endif
}

/** Proto datalog would replace CSV on-chain; keep that debug-only for now. */
inline bool protoDatalogEnabled() {
#if defined(ALTRUIST_PROTO_PROTOCOL) && defined(ALTRUIST_BUILD_DEBUG)
	return true;
#else
	return false;
#endif
}

enum ProtoBuildStatus {
	PROTO_BUILD_OK = 0,
	PROTO_BUILD_DISABLED,
	PROTO_BUILD_EMPTY,
	PROTO_BUILD_KEY_FAILED,
	PROTO_BUILD_ENCRYPT_FAILED,
	PROTO_BUILD_ENCODE_FAILED,
	PROTO_BUILD_SIGN_FAILED,
	PROTO_BUILD_BUFFER_TOO_SMALL,
};

constexpr size_t PROTO_ENVELOPE_BUF_BYTES = 1536; /* SignedEnvelope encode buffer */
constexpr size_t PROTO_MESSAGE_BUF_BYTES = 1280;  /* serialized core.v1.Message */
constexpr size_t PROTO_ENVELOPE_NONCE_LEN = 16;   /* schema allows 16–32 */
constexpr size_t DATALOG_CHAIN_MAX_BYTES = 512;   /* parachain record hard cap */
constexpr size_t DATALOG_CHAIN_SAFE_BYTES = 480;  /* send proto datalog only if ≤ this */

inline const char *protoBuildStatusReason(ProtoBuildStatus status) {
	switch (status) {
	case PROTO_BUILD_OK:
		return "ok";
	case PROTO_BUILD_DISABLED:
		return "disabled";
	case PROTO_BUILD_EMPTY:
		return "payload_empty";
	case PROTO_BUILD_KEY_FAILED:
		return "key_failed";
	case PROTO_BUILD_ENCRYPT_FAILED:
		return "encryption_failed";
	case PROTO_BUILD_ENCODE_FAILED:
		return "encode_failed";
	case PROTO_BUILD_SIGN_FAILED:
		return "sign_failed";
	case PROTO_BUILD_BUFFER_TOO_SMALL:
		return "buffer_too_small";
	default:
		return "unknown";
	}
}

#endif

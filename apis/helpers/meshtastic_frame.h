#ifndef ALTRUIST_MESHTASTIC_FRAME_H
#define ALTRUIST_MESHTASTIC_FRAME_H

/*
 * Connectivity Protocol Meshtastic Transport v1 (sender).
 * Spec: connectivity-protocol/transport/meshtastic/v1.md
 * Opaque payload for the prototype is serialized core.v1.Message (not SignedEnvelope).
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MESHTASTIC_CTRL_SINGLE 0x01
#define MESHTASTIC_CTRL_FRAGMENT 0x41
#define MESHTASTIC_SINGLE_MAX_BYTES 220u
#define MESHTASTIC_FRAGMENT_BODY_BYTES 213u
#define MESHTASTIC_MAX_FRAGMENTS 16u
#define MESHTASTIC_MAX_PAYLOAD_BYTES (MESHTASTIC_MAX_FRAGMENTS * MESHTASTIC_FRAGMENT_BODY_BYTES)
#define MESHTASTIC_TRANSPORT_MTU 221u
#define MESHTASTIC_MESSAGE_ID_LEN 6u
#define MESHTASTIC_FRAGMENT_HEADER_LEN 8u
#define MESHTASTIC_PORTNUM_PRIVATE_APP 256u

/* 0 = too large / empty. 1 = use SINGLE. 2..16 = FRAGMENT count. */
uint8_t meshtasticFragmentCount(size_t payload_len);

bool meshtasticMessageId(const uint8_t *payload, size_t payload_len, uint8_t id_out[MESHTASTIC_MESSAGE_ID_LEN]);

size_t meshtasticEncodeSingle(const uint8_t *payload, size_t payload_len, uint8_t *out, size_t out_cap);

size_t meshtasticEncodeFragment(const uint8_t *payload, size_t payload_len, const uint8_t id[MESHTASTIC_MESSAGE_ID_LEN],
				uint8_t index, uint8_t count, uint8_t *out, size_t out_cap);

#ifdef __cplusplus
}
#endif

#endif

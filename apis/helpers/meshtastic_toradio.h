#ifndef ALTRUIST_MESHTASTIC_TORADIO_H
#define ALTRUIST_MESHTASTIC_TORADIO_H

/*
 * Minimal Meshtastic Phone API / Serial Module PROTO encoder.
 * ToRadio { packet: MeshPacket { to, decoded: Data { portnum, payload }, want_ack, pki_encrypted } }
 * wrapped in 0x94 0xC3 + uint16 BE length.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MESHTASTIC_SERIAL_START1 0x94
#define MESHTASTIC_SERIAL_START2 0xC3
#define MESHTASTIC_BROADCAST_NODE 0xFFFFFFFFu

size_t meshtasticEncodeToRadioUnicast(uint32_t dest_node, uint32_t portnum, uint32_t packet_id, const uint8_t *frame,
				      size_t frame_len, uint8_t *out, size_t out_cap);
size_t meshtasticEncodeWantConfig(uint32_t config_id, uint8_t *out, size_t out_cap);
size_t meshtasticEncodeHeartbeat(uint32_t nonce, uint8_t *out, size_t out_cap);
/** True when FromRadio contains config_complete_id; writes the id. */
bool meshtasticFromRadioConfigComplete(const uint8_t *pb, size_t pb_len, uint32_t *config_id);

#ifdef __cplusplus
}
#endif

#endif

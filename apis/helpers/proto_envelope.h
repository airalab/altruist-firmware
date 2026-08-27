#ifndef __PROTO_ENVELOPE_H__
#define __PROTO_ENVELOPE_H__

/**
 * Firmware adapter around proto_codec.
 * Reads sensor JSON + cfg::share_/encrypt_, resolves keys, then encodes
 * a crypto.v1.SignedEnvelope. Used by Robonomics HTTP (map) and datalog.
 */

#include "proto_protocol.h"

#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>

class Robonomics;

ProtoBuildStatus protoBuildSignedEnvelope(JsonDocument &data, Robonomics *robonomics, uint8_t *out, size_t out_cap,
					  size_t *out_len);

#endif

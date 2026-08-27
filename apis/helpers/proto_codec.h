#ifndef __PROTO_CODEC_H__
#define __PROTO_CODEC_H__

/**
 * Compact nanopb encoder: ProtoSample → core.v1.Message → crypto.v1.SignedEnvelope.
 * No Arduino/JSON here; the adapter (proto_envelope) fills ProtoSample.
 *
 * C field names public_items/private_items map to proto fields `public`/`private`
 * (tags 1/2). Wire format is unchanged.
 */

#include "proto_protocol.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* share without encrypt → PUBLIC; encrypt_* → PRIVATE; neither → SKIP. */
enum ProtoDest {
	PROTO_DEST_SKIP = 0,
	PROTO_DEST_PUBLIC = 1,
	PROTO_DEST_PRIVATE = 2,
};

struct ProtoSample {
	uint8_t owner[32]; /* Meta.owner: 32-byte pubkey, not SS58 */
	int insight;       /* 1 = device.v1.Insight, 0 = device.v1.Urban */

	int has_gps;
	double lat;
	double lon;

	int has_temperature;
	int has_humidity;
	int has_pressure;
	int has_pm25;
	int has_pm10;
	int has_noise_max;
	int has_noise_avg;
	int has_co2;
	int temp_from_scd41; /* Insight: T/H from SCD41 after warmup, else BME680 */
	int pressure_hpa;    /* Urban BME280 JSON is hPa → multiply by 100 for Pa */

	double temperature;
	double humidity;
	double pressure;
	double pm25;
	double pm10;
	double noise_max;
	double noise_avg;
	double co2;

	enum ProtoDest dest_temperature;
	enum ProtoDest dest_humidity;
	enum ProtoDest dest_pressure;
	enum ProtoDest dest_pm;
	enum ProtoDest dest_noise;
	enum ProtoDest dest_co2;
};

/* AES-GCM for crypto.v1.Encrypted; cipher_out is malloc'd, caller (codec) frees. */
typedef int (*ProtoAeadFn)(const uint8_t *plain, size_t plain_len, uint8_t from_pk[32], uint8_t nonce[12],
			   uint8_t **cipher, size_t *cipher_len);

/* Ed25519 over preimage: sensor_id || le64(timestamp_ms) || nonce || message. */
typedef void (*ProtoSignFn)(void *ctx, const uint8_t *msg, size_t len, uint8_t sig[64]);

ProtoBuildStatus proto_encode_message(const struct ProtoSample *sample, ProtoAeadFn aead, uint8_t *out, size_t out_cap,
				      size_t *out_len);

ProtoBuildStatus proto_encode_envelope(const uint8_t *message, size_t message_len, const uint8_t sensor_id[32],
				       uint64_t timestamp_ms, const uint8_t *nonce, size_t nonce_len, ProtoSignFn sign,
				       void *sign_ctx, uint8_t *out, size_t out_cap, size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif

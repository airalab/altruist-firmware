#include "proto_codec.h"

/*
 * Nanopb encode path for core.v1.Message and crypto.v1.SignedEnvelope.
 * Without ALTRUIST_PROTO_PROTOCOL the functions below are stubs.
 */

#include <string.h>

#if !defined(ALTRUIST_PROTO_PROTOCOL)

ProtoBuildStatus proto_encode_message(const struct ProtoSample *sample, ProtoAeadFn aead, uint8_t *out, size_t out_cap,
				      size_t *out_len) {
	(void)sample;
	(void)aead;
	(void)out;
	(void)out_cap;
	if (out_len) {
		*out_len = 0;
	}
	return PROTO_BUILD_DISABLED;
}

ProtoBuildStatus proto_encode_envelope(const uint8_t *message, size_t message_len, const uint8_t sensor_id[32],
				       uint64_t timestamp_ms, const uint8_t *nonce, size_t nonce_len, ProtoSignFn sign,
				       void *sign_ctx, uint8_t *out, size_t out_cap, size_t *out_len) {
	(void)message;
	(void)message_len;
	(void)sensor_id;
	(void)timestamp_ms;
	(void)nonce;
	(void)nonce_len;
	(void)sign;
	(void)sign_ctx;
	(void)out;
	(void)out_cap;
	if (out_len) {
		*out_len = 0;
	}
	return PROTO_BUILD_DISABLED;
}

#else

#include <pb_encode.h>
#include <stdlib.h>

#include "core/v1/message.pb.h"
#include "crypto/v1/envelope.pb.h"
#include "device/v1/insight.pb.h"
#include "device/v1/urban.pb.h"

static int encode_pb(const pb_msgdesc_t *fields, const void *src, uint8_t *out, size_t out_cap, size_t *out_len) {
	pb_ostream_t stream;
	if (!fields || !src || !out || !out_len || out_cap == 0) {
		return 0;
	}
	stream = pb_ostream_from_buffer(out, out_cap);
	if (!pb_encode(&stream, fields, src)) {
		return 0;
	}
	*out_len = stream.bytes_written;
	return 1;
}

static int push_item(void *arr, pb_size_t *count, pb_size_t max, size_t elem_size, const void *item) {
	if (!arr || !count || !item || *count >= max) {
		return 0;
	}
	memcpy((uint8_t *)arr + (*count) * elem_size, item, elem_size);
	(*count)++;
	return 1;
}

static int route_item(void *pub, pb_size_t *pub_n, pb_size_t pub_max, void *priv, pb_size_t *priv_n, pb_size_t priv_max,
		      size_t elem_size, enum ProtoDest dest, const void *item) {
	if (dest == PROTO_DEST_PUBLIC) {
		return push_item(pub, pub_n, pub_max, elem_size, item);
	}
	if (dest == PROTO_DEST_PRIVATE) {
		return push_item(priv, priv_n, priv_max, elem_size, item);
	}
	return 1;
}

/* Serialize EncryptedUrban/EncryptedInsight, AES-GCM, fill crypto.v1.Encrypted. */
static int seal_private(crypto_v1_Encrypted *slot, const pb_msgdesc_t *fields, const void *enc_msg, ProtoAeadFn aead) {
	uint8_t plain[device_v1_EncryptedUrban_size];
	size_t plain_len = 0;
	uint8_t from_pk[32];
	uint8_t nonce[12];
	uint8_t *cipher = NULL;
	size_t cipher_len = 0;

	if (!slot || !aead) {
		return 0;
	}
	if (!encode_pb(fields, enc_msg, plain, sizeof(plain), &plain_len)) {
		return 0;
	}
	if (aead(plain, plain_len, from_pk, nonce, &cipher, &cipher_len) != 0 || !cipher) {
		return 0;
	}
	if (cipher_len > sizeof(slot->ciphertext.bytes)) {
		free(cipher);
		return 0;
	}
	*slot = crypto_v1_Encrypted_init_zero;
	slot->version = 1;
	strncpy(slot->algorithm, "aesgcm256", sizeof(slot->algorithm));
	slot->algorithm[sizeof(slot->algorithm) - 1] = '\0';
	memcpy(slot->from, from_pk, 32);
	slot->nonce.size = 12;
	memcpy(slot->nonce.bytes, nonce, 12);
	slot->ciphertext.size = (pb_size_t)cipher_len;
	memcpy(slot->ciphertext.bytes, cipher, cipher_len);
	free(cipher);
	return 1;
}

#define ARR_MAX(field) ((pb_size_t)(sizeof((field)) / sizeof((field)[0])))

static int add_urban(device_v1_Urban *urban, device_v1_EncryptedUrban *enc, enum ProtoDest dest,
		     const device_v1_UrbanSensor *item) {
	return route_item(urban->public_items, &urban->public_items_count, ARR_MAX(urban->public_items), enc->sensors,
			  &enc->sensors_count, ARR_MAX(enc->sensors), sizeof(*item), dest, item);
}

static int add_insight(device_v1_Insight *insight, device_v1_EncryptedInsight *enc, enum ProtoDest dest,
		       const device_v1_InsightSensor *item) {
	return route_item(insight->public_items, &insight->public_items_count, ARR_MAX(insight->public_items),
			  enc->sensors, &enc->sensors_count, ARR_MAX(enc->sensors), sizeof(*item), dest, item);
}

static device_v1_UrbanSensor urban_gps(double lat, double lon) {
	device_v1_UrbanSensor item = device_v1_UrbanSensor_init_zero;
	item.which_sensor = device_v1_UrbanSensor_gps_tag;
	item.sensor.gps.lat = lat;
	item.sensor.gps.lon = lon;
	return item;
}

static device_v1_UrbanSensor urban_bme(pb_size_t tag, double value, int pressure_hpa) {
	device_v1_UrbanSensor item = device_v1_UrbanSensor_init_zero;
	item.which_sensor = device_v1_UrbanSensor_bme280_tag;
	item.sensor.bme280.which_measurement = tag;
	if (tag == sensor_v1_BME280_temperature_tag) {
		item.sensor.bme280.measurement.temperature.celsius = value;
	} else if (tag == sensor_v1_BME280_humidity_tag) {
		item.sensor.bme280.measurement.humidity.percent = value;
	} else {
		item.sensor.bme280.measurement.pressure.pascal = pressure_hpa ? value * 100.0 : value;
	}
	return item;
}

static device_v1_UrbanSensor urban_sds(pb_size_t tag, double value) {
	device_v1_UrbanSensor item = device_v1_UrbanSensor_init_zero;
	item.which_sensor = device_v1_UrbanSensor_sds011_tag;
	item.sensor.sds011.which_measurement = tag;
	if (tag == sensor_v1_SDS011_pm25_tag) {
		item.sensor.sds011.measurement.pm25.ug_m3 = value;
	} else {
		item.sensor.sds011.measurement.pm10.ug_m3 = value;
	}
	return item;
}

static device_v1_UrbanSensor urban_noise(pb_size_t tag, double value) {
	device_v1_UrbanSensor item = device_v1_UrbanSensor_init_zero;
	item.which_sensor = device_v1_UrbanSensor_ics43434_tag;
	item.sensor.ics43434.which_measurement = tag;
	if (tag == sensor_v1_ICS43434_noise_max_tag) {
		item.sensor.ics43434.measurement.noise_max.db = value;
	} else {
		item.sensor.ics43434.measurement.noise_avg.db = value;
	}
	return item;
}

static device_v1_InsightSensor insight_gps(double lat, double lon) {
	device_v1_InsightSensor item = device_v1_InsightSensor_init_zero;
	item.which_sensor = device_v1_InsightSensor_gps_tag;
	item.sensor.gps.lat = lat;
	item.sensor.gps.lon = lon;
	return item;
}

static device_v1_InsightSensor insight_bme(pb_size_t tag, double value) {
	device_v1_InsightSensor item = device_v1_InsightSensor_init_zero;
	item.which_sensor = device_v1_InsightSensor_bme680_tag;
	item.sensor.bme680.which_measurement = tag;
	if (tag == sensor_v1_BME680_temperature_tag) {
		item.sensor.bme680.measurement.temperature.celsius = value;
	} else if (tag == sensor_v1_BME680_humidity_tag) {
		item.sensor.bme680.measurement.humidity.percent = value;
	} else {
		item.sensor.bme680.measurement.pressure.pascal = value;
	}
	return item;
}

static device_v1_InsightSensor insight_scd(pb_size_t tag, double value) {
	device_v1_InsightSensor item = device_v1_InsightSensor_init_zero;
	item.which_sensor = device_v1_InsightSensor_scd41_tag;
	item.sensor.scd41.which_measurement = tag;
	if (tag == sensor_v1_SCD41_co2_tag) {
		item.sensor.scd41.measurement.co2.ppm = value;
	} else if (tag == sensor_v1_SCD41_temperature_tag) {
		item.sensor.scd41.measurement.temperature.celsius = value;
	} else {
		item.sensor.scd41.measurement.humidity.percent = value;
	}
	return item;
}

static ProtoBuildStatus fill_urban(const struct ProtoSample *s, core_v1_Message *msg, ProtoAeadFn aead) {
	device_v1_EncryptedUrban enc = device_v1_EncryptedUrban_init_zero;
	device_v1_Urban *urban = &msg->payload.urban;
	device_v1_UrbanSensor item;

	*urban = device_v1_Urban_init_zero;
	msg->which_payload = core_v1_Message_urban_tag;

	if (s->has_gps) {
		/* GPS is always public when coordinates exist. */
		item = urban_gps(s->lat, s->lon);
		if (!add_urban(urban, &enc, PROTO_DEST_PUBLIC, &item)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
	}
	if (s->has_temperature && s->dest_temperature != PROTO_DEST_SKIP) {
		item = urban_bme(sensor_v1_BME280_temperature_tag, s->temperature, 0);
		if (!add_urban(urban, &enc, s->dest_temperature, &item)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
	}
	if (s->has_humidity && s->dest_humidity != PROTO_DEST_SKIP) {
		item = urban_bme(sensor_v1_BME280_humidity_tag, s->humidity, 0);
		if (!add_urban(urban, &enc, s->dest_humidity, &item)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
	}
	if (s->has_pressure && s->dest_pressure != PROTO_DEST_SKIP) {
		item = urban_bme(sensor_v1_BME280_pressure_tag, s->pressure, s->pressure_hpa);
		if (!add_urban(urban, &enc, s->dest_pressure, &item)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
	}
	if (s->has_pm25 && s->dest_pm != PROTO_DEST_SKIP) {
		item = urban_sds(sensor_v1_SDS011_pm25_tag, s->pm25);
		if (!add_urban(urban, &enc, s->dest_pm, &item)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
	}
	if (s->has_pm10 && s->dest_pm != PROTO_DEST_SKIP) {
		item = urban_sds(sensor_v1_SDS011_pm10_tag, s->pm10);
		if (!add_urban(urban, &enc, s->dest_pm, &item)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
	}
	if (s->has_noise_max && s->dest_noise != PROTO_DEST_SKIP) {
		item = urban_noise(sensor_v1_ICS43434_noise_max_tag, s->noise_max);
		if (!add_urban(urban, &enc, s->dest_noise, &item)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
	}
	if (s->has_noise_avg && s->dest_noise != PROTO_DEST_SKIP) {
		item = urban_noise(sensor_v1_ICS43434_noise_avg_tag, s->noise_avg);
		if (!add_urban(urban, &enc, s->dest_noise, &item)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
	}

	if (enc.sensors_count > 0) {
		if (urban->private_items_count >= ARR_MAX(urban->private_items)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
		if (!seal_private(&urban->private_items[urban->private_items_count], device_v1_EncryptedUrban_fields,
				  &enc, aead)) {
			return PROTO_BUILD_ENCRYPT_FAILED;
		}
		urban->private_items_count++;
	}
	if (urban->public_items_count == 0 && urban->private_items_count == 0) {
		return PROTO_BUILD_EMPTY;
	}
	return PROTO_BUILD_OK;
}

static ProtoBuildStatus fill_insight(const struct ProtoSample *s, core_v1_Message *msg, ProtoAeadFn aead) {
	device_v1_EncryptedInsight enc = device_v1_EncryptedInsight_init_zero;
	device_v1_Insight *insight = &msg->payload.insight;
	device_v1_InsightSensor item;

	*insight = device_v1_Insight_init_zero;
	msg->which_payload = core_v1_Message_insight_tag;

	if (s->has_gps) {
		item = insight_gps(s->lat, s->lon);
		if (!add_insight(insight, &enc, PROTO_DEST_PUBLIC, &item)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
	}
	if (s->has_pressure && s->dest_pressure != PROTO_DEST_SKIP) {
		item = insight_bme(sensor_v1_BME680_pressure_tag, s->pressure);
		if (!add_insight(insight, &enc, s->dest_pressure, &item)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
	}
	if (s->has_temperature && s->dest_temperature != PROTO_DEST_SKIP) {
		item = s->temp_from_scd41 ? insight_scd(sensor_v1_SCD41_temperature_tag, s->temperature)
					  : insight_bme(sensor_v1_BME680_temperature_tag, s->temperature);
		if (!add_insight(insight, &enc, s->dest_temperature, &item)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
	}
	if (s->has_humidity && s->dest_humidity != PROTO_DEST_SKIP) {
		item = s->temp_from_scd41 ? insight_scd(sensor_v1_SCD41_humidity_tag, s->humidity)
					  : insight_bme(sensor_v1_BME680_humidity_tag, s->humidity);
		if (!add_insight(insight, &enc, s->dest_humidity, &item)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
	}
	if (s->has_co2 && s->dest_co2 != PROTO_DEST_SKIP) {
		item = insight_scd(sensor_v1_SCD41_co2_tag, s->co2);
		if (!add_insight(insight, &enc, s->dest_co2, &item)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
	}

	if (enc.sensors_count > 0) {
		if (insight->private_items_count >= ARR_MAX(insight->private_items)) {
			return PROTO_BUILD_ENCODE_FAILED;
		}
		if (!seal_private(&insight->private_items[insight->private_items_count],
				  device_v1_EncryptedInsight_fields, &enc, aead)) {
			return PROTO_BUILD_ENCRYPT_FAILED;
		}
		insight->private_items_count++;
	}
	if (insight->public_items_count == 0 && insight->private_items_count == 0) {
		return PROTO_BUILD_EMPTY;
	}
	return PROTO_BUILD_OK;
}

ProtoBuildStatus proto_encode_message(const struct ProtoSample *sample, ProtoAeadFn aead, uint8_t *out, size_t out_cap,
				      size_t *out_len) {
	static core_v1_Message msg;
	ProtoBuildStatus st;

	if (out_len) {
		*out_len = 0;
	}
	if (!sample || !out || !out_len || out_cap == 0) {
		return PROTO_BUILD_BUFFER_TOO_SMALL;
	}

	msg = core_v1_Message_init_zero;
	msg.has_metadata = true;
	memcpy(msg.metadata.owner, sample->owner, 32);

	st = sample->insight ? fill_insight(sample, &msg, aead) : fill_urban(sample, &msg, aead);
	if (st != PROTO_BUILD_OK) {
		return st;
	}
	if (!encode_pb(core_v1_Message_fields, &msg, out, out_cap, out_len)) {
		return PROTO_BUILD_ENCODE_FAILED;
	}
	return PROTO_BUILD_OK;
}

static void write_u64_le(uint8_t out[8], uint64_t value) {
	int i;
	for (i = 0; i < 8; i++) {
		out[i] = (uint8_t)(value >> (8 * i));
	}
}

/*
 * Sign preimage = sensor_id (32) || timestamp as little-endian uint64 || nonce || message.
 * Confirm LE64 with connectivity before #20. Envelope timestamp field is still uint64 unix ms.
 */
ProtoBuildStatus proto_encode_envelope(const uint8_t *message, size_t message_len, const uint8_t sensor_id[32],
				       uint64_t timestamp_ms, const uint8_t *nonce, size_t nonce_len, ProtoSignFn sign,
				       void *sign_ctx, uint8_t *out, size_t out_cap, size_t *out_len) {
	static uint8_t preimage[32 + 8 + 32 + PROTO_MESSAGE_BUF_BYTES];
	crypto_v1_SignedEnvelope envelope;
	uint8_t ts_le[8];
	uint8_t signature[64];
	size_t preimage_len;
	pb_ostream_t stream;

	if (out_len) {
		*out_len = 0;
	}
	if (!message || !sensor_id || !nonce || !sign || !out || !out_len) {
		return PROTO_BUILD_BUFFER_TOO_SMALL;
	}
	if (nonce_len < 16 || nonce_len > 32 || message_len > PROTO_MESSAGE_BUF_BYTES) {
		return PROTO_BUILD_BUFFER_TOO_SMALL;
	}

	write_u64_le(ts_le, timestamp_ms);
	preimage_len = 32 + 8 + nonce_len + message_len;
	if (preimage_len > sizeof(preimage)) {
		return PROTO_BUILD_BUFFER_TOO_SMALL;
	}
	memcpy(preimage, sensor_id, 32);
	memcpy(preimage + 32, ts_le, 8);
	memcpy(preimage + 40, nonce, nonce_len);
	memcpy(preimage + 40 + nonce_len, message, message_len);
	sign(sign_ctx, preimage, preimage_len, signature);

	envelope = crypto_v1_SignedEnvelope_init_zero;
	memcpy(envelope.sensor_id, sensor_id, 32);
	envelope.timestamp = timestamp_ms;
	envelope.nonce.size = (pb_size_t)nonce_len;
	memcpy(envelope.nonce.bytes, nonce, nonce_len);
	envelope.message.size = (pb_size_t)message_len;
	memcpy(envelope.message.bytes, message, message_len);
	memcpy(envelope.signature, signature, 64);

	stream = pb_ostream_from_buffer(out, out_cap);
	if (!pb_encode(&stream, crypto_v1_SignedEnvelope_fields, &envelope)) {
		return PROTO_BUILD_ENCODE_FAILED;
	}
	*out_len = stream.bytes_written;
	return PROTO_BUILD_OK;
}

#endif

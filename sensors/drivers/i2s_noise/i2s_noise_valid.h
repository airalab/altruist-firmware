#ifndef I2S_NOISE_VALID_H
#define I2S_NOISE_VALID_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** ICS-43434 Urban path (#195). Keep in sync with SoundSensor SAMPLES / SAMPLE_FREQ. */
#ifndef I2S_NOISE_SAMPLES
#define I2S_NOISE_SAMPLES 2048
#endif
#ifndef I2S_NOISE_SAMPLE_FREQ
#define I2S_NOISE_SAMPLE_FREQ 22627
#endif
#ifndef I2S_NOISE_BYTES_PER_SAMPLE
#define I2S_NOISE_BYTES_PER_SAMPLE 4u
#endif

#define I2S_NOISE_BLOCK_BYTES \
	((uint32_t)I2S_NOISE_SAMPLES * (uint32_t)I2S_NOISE_BYTES_PER_SAMPLE)

/** One 2048-sample block at 22627 Hz is ~91 ms. */
static inline uint32_t i2sNoiseBlockDurationMs(void)
{
	return ((uint32_t)I2S_NOISE_SAMPLES * 1000u + (uint32_t)I2S_NOISE_SAMPLE_FREQ - 1u) /
	       (uint32_t)I2S_NOISE_SAMPLE_FREQ;
}

/** Finite i2s_read wait: 3× the block (DMA / scheduling). Never portMAX_DELAY. */
static inline uint32_t i2sNoiseReadTimeoutMs(void)
{
	return i2sNoiseBlockDurationMs() * 3u;
}

/** ESP_OK is 0. Short/zero reads and any error are not a measurement. */
static inline bool i2sNoiseReadBlockValid(int err, size_t bytes_read, uint32_t expected_bytes)
{
	return (err == 0) && (expected_bytes > 0u) && (bytes_read == (size_t)expected_bytes);
}

static inline bool i2sNoiseShouldUpdateAggregates(bool sds_running, bool acq_ok)
{
	return !sds_running && acq_ok;
}

#endif

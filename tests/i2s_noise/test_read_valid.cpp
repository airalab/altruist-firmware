#include "i2s_noise_valid.h"

#include <stdio.h>

static int g_failed = 0;

static void expect(bool ok, const char *name)
{
	if (ok) {
		printf("ok  %s\n", name);
		return;
	}
	printf("FAIL  %s\n", name);
	g_failed = 1;
}

struct NoiseAgg {
	uint32_t sum;
	uint32_t count;
	uint8_t maxv;
	uint32_t publishes;
};

static void applyAcq(NoiseAgg *a, bool sds, bool acq_ok, uint8_t mx, uint32_t mean)
{
	if (!i2sNoiseShouldUpdateAggregates(sds, acq_ok)) {
		return;
	}
	if (mx > a->maxv) {
		a->maxv = mx;
	}
	a->sum += mean;
	a->count++;
	a->publishes++;
}

int main(void)
{
	const uint32_t expected = I2S_NOISE_BLOCK_BYTES;
	expect(expected == 8192u, "block is 2048 * 4 bytes");
	expect(i2sNoiseBlockDurationMs() == 91u, "block duration 91 ms");
	expect(i2sNoiseReadTimeoutMs() == 273u, "read timeout 3x block");

	expect(i2sNoiseReadBlockValid(0, expected, expected), "ESP_OK full block");
	expect(!i2sNoiseReadBlockValid(1, expected, expected), "i2s_read error");
	expect(!i2sNoiseReadBlockValid(-1, expected, expected), "negative err");
	expect(!i2sNoiseReadBlockValid(0, 0, expected), "ESP_OK zero bytes");
	expect(!i2sNoiseReadBlockValid(0, expected / 2u, expected), "ESP_OK short block");
	expect(!i2sNoiseReadBlockValid(0, expected, 0), "expected 0");
	expect(!i2sNoiseReadBlockValid(0x107, 0, expected), "timeout-like err");

	expect(!i2sNoiseShouldUpdateAggregates(true, true), "SDS pause is not a read error");
	expect(!i2sNoiseShouldUpdateAggregates(true, false), "SDS pause + fail");
	expect(!i2sNoiseShouldUpdateAggregates(false, false), "fail does not update");
	expect(i2sNoiseShouldUpdateAggregates(false, true), "success updates");

	NoiseAgg a = {};
	applyAcq(&a, false, false, 40, 36); // driver/start/read fail
	expect(a.count == 0 && a.sum == 0 && a.maxv == 0 && a.publishes == 0,
	       "failed acq does not touch aggregates");
	applyAcq(&a, true, true, 55, 40); // SDS pause
	expect(a.count == 0 && a.publishes == 0, "SDS pause does not update");
	applyAcq(&a, false, true, 42, 38); // recovery
	expect(a.count == 1 && a.sum == 38 && a.maxv == 42 && a.publishes == 1,
	       "later complete block updates once");
	applyAcq(&a, false, true, 50, 40);
	expect(a.count == 2 && a.sum == 78 && a.maxv == 50 && a.publishes == 2,
	       "second success accumulates");

	return g_failed ? 1 : 0;
}

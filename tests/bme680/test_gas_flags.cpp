#include "bme68x_gas_valid.h"

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

int main(void)
{
	const uint8_t heat = BME68X_HEAT_STAB_MSK;
	const uint8_t gas = BME68X_GASM_VALID_MSK;
	const uint32_t r = 50000;

	expect(!bme68xGasResultValid(0, r), "0 0 -> invalid");
	expect(!bme68xGasResultValid(gas, r), "0 1 -> invalid");
	expect(!bme68xGasResultValid(heat, r), "1 0 -> invalid");
	expect(bme68xGasResultValid((uint8_t)(heat | gas), r), "1 1 -> valid");
	expect(!bme68xGasResultValid((uint8_t)(heat | gas), 0), "1 1 R=0 -> invalid");

	return g_failed ? 1 : 0;
}

#ifndef BME68X_GAS_VALID_H
#define BME68X_GAS_VALID_H

#include "bme68x_defs.h"

#include <stdbool.h>
#include <stdint.h>

/** Bosch: gas result needs heater-stable AND conversion-valid (#194). */
static inline bool bme68xGasResultValid(uint8_t status, uint32_t gas_resistance)
{
	const uint8_t required = (uint8_t)(BME68X_HEAT_STAB_MSK | BME68X_GASM_VALID_MSK);
	return ((status & required) == required) && (gas_resistance > 0u);
}

#endif

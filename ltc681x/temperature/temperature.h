/*
 * temperature.h
 *
 *  Created on: 31 Oca 2024
 *      Author: mehmet.dincer
 */

#include "stdint.h"
#include "math.h"

#ifndef LTC681X_TEMPERATURE_TEMPERATURE_H_
#define LTC681X_TEMPERATURE_TEMPERATURE_H_

#define NTC_R_SERIES                    (10000.0f)
#define NTC_R_NOMINAL                   (10000.0f)
#define NTC_TEMP_NOMINAL                (25.0f)
#define NTC_ADC_MAX                     (65535) //  16bit
#define NTC_BETA                        (3950)

#define IC_MAX_VOLTAGE                  (5.0f)
#define PULL_UP                         (0x01)
#define PULL_DOWN                       (0x02)

double AE_calculateTemp(float adcValue, float refVolt, uint8_t PULL_);


#endif /* LTC681X_TEMPERATURE_TEMPERATURE_H_ */

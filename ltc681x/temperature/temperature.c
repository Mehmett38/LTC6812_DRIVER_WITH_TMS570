/*
 * temperature.c
 *
 *  Created on: 31 Oca 2024
 *      Author: mehmet.dincer
 */


#include "temperature.h"

/**
 * @brief calculate the temperature with NTC
 * @param[in] ADC voltage value
 * @param[in] refVolt value
 * @param[in] system type for parameter search @refgroup PULL_
 * @return temperature in celcius
 * @structure
 *      refVolt--------/\/\/\/\----|----/\/\/\/\--------GND
 *                    resistance   |      NTC
 *                                 |
 *                                 |
 *                                ADC
 */
double AE_calculateTemp(float adcValue, float refVolt, uint8_t PULL_)
{
    uint16_t adcVal;

    if(PULL_ == PULL_DOWN)
    {
        adcValue = refVolt - adcValue;             // take the resistance voltage
    }

    adcVal = (adcValue / IC_MAX_VOLTAGE) * NTC_ADC_MAX;

    float rntc = (float)NTC_R_SERIES / (((float)NTC_ADC_MAX / (float)adcVal ) - 1.0f);
    float temp;
    temp = rntc / (float)NTC_R_NOMINAL;
    temp = logf(temp);
    temp /= (float)NTC_BETA;
    temp += 1.0f / ((float)NTC_TEMP_NOMINAL + 273.15f);
    temp = 1.0f / temp;
    temp -= 273.15f;
    return temp;
}

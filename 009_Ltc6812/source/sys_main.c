/** @file sys_main.c 
*   @brief Application main file
*   @date 11-Dec-2018
*   @version 04.07.01
*
*   This file contains an empty main function,
*   which can be used for the application.
*/

/* 
* Copyright (C) 2009-2018 Texas Instruments Incorporated - www.ti.com 
* 
* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*    Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the 
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


/* USER CODE BEGIN (0) */
/* USER CODE END */

/* Include Files */

#include "sys_common.h"

/* USER CODE BEGIN (1) */

#include "ltc6812.h"
#include "het.h"
#include "sys_core.h"
#include "gio.h"

/* USER CODE END */

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

/* USER CODE BEGIN (2) */
Ltc682x ltcBat = {0};

void ltcInit(spiBASE_t * spiReg);

#define UNUSED(x)   ((void)x)
uint8_t toggle = 1;
uint8_t returnVarningClose;

/* USER CODE END */

int main(void)
{
/* USER CODE BEGIN (3) */
    ltcInit(spiREG1);

    gioInit();


#if 0       // code testing V1.1    working successfully
    uint16_t isoTxData[12];
    uint16_t isoRxData[8] = {0};
    uint16_t eceTx;

    uint16_t isoData[6] = {0x7C, 0x77, 0x44, 0x22, 0x11, 0x55};

    eceTx = AE_pec15((uint8_t*)isoData, 6);

    isoTxData[0] = cmdWRCFGA_pu16[0];
    isoTxData[1] = cmdWRCFGA_pu16[1];
    isoTxData[2] = cmdWRCFGA_pu16[2];
    isoTxData[3] = cmdWRCFGA_pu16[3];
    isoTxData[4] = isoData[0];
    isoTxData[5] = isoData[1];
    isoTxData[6] = isoData[2];
    isoTxData[7] = isoData[3];
    isoTxData[8] = isoData[4];
    isoTxData[9] = isoData[5];
    isoTxData[10] = (eceTx >> 8) & 0xFF;
    isoTxData[11] = (eceTx >> 0) & 0xFF;
    spiDAT1_t spiDat_s =            // spi configuration parameters
    {
         .CSNR = 0,
         .CS_HOLD = 1,
         .DFSEL = SPI_FMT_0,
         .WDEL = 0
    };
#endif

    LTC_status status;


    AE_delayMs(2);  // Twakeup time
    AE_ltcSetPwm(&ltcBat, S_PIN_ALL, PWM_DUTY_LEVEL_14);

    while(1)
    {
#if 0   // read the cell voltage
        AE_ltcStartCellAdc(&ltcBat, MODE_7KHZ, true, CELL_ALL);
        //!< check adcMeasure duration is completed
        while(!AE_ltcAdcMeasureState());
        AE_ltcReadCellVoltage(&ltcBat);
#endif

#if 0   // GPIO voltage Measure
        AE_ltcStartGpioAdc(&ltcBat, MODE_7KHZ, GPIO_ALL);
        while(!AE_ltcAdcM   easureState());
        AE_ltcReadGPIOVoltage(&ltcBat);
#endif

#if 0   //read status registerA
        AE_ltcStartStatusAdc(&ltcBat, MODE_7KHZ, CHST_ALL);
        //!< check adcMeasure duration is completed
        while(!AE_ltcAdcMeasureState());
        AE_ltcReadStatusRegA(&ltcBat);
#endif

#if 0   //read status registerB
        AE_ltcStartStatusAdc(&ltcBat, MODE_7KHZ, CHST_ALL);
        //!< check adcMeasure duration is completed
        while(!AE_ltcAdcMeasureState());
        AE_ltcReadStatusRegB(&ltcBat);
#endif

#if 0
        AE_ltcStartStatusAdc(&ltcBat, MODE_7KHZ, CHST_ALL);
        while(!AE_ltcAdcMeasureState());
        AE_ltcReadStatusRegA(&ltcBat);
        AE_ltcReadStatusRegB(&ltcBat);

#endif

#if 0   //pwm duty setting
        AE_ltcSetPwm(&ltcBat, S_PIN_1 | S_PIN_2| S_PIN_3, PWM_DUTY_LEVEL_8);
        AE_ltcSetPwm(&ltcBat, S_PIN_4 | S_PIN_5| S_PIN_6, PWM_DUTY_LEVEL_8);
#endif

#if 0    //LTC6812-1 has 3 clear ADC commands: CLRCELL, CLRAUX and CLRSTAT
        status = AE_ltcClearCellAdc(&ltcBat);
        status = AE_ltcClearGpioAdc(&ltcBat);
        status = AE_ltcClearStatusAdc(&ltcBat);
#endif

        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<BEFORE WORKING LTC>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        #if 0   // code testing V1.1    working successfully

        AE_ltcWakeUpSleep();

        AE_LTC_CS_ON();

        spiTransmitData(spiREG1, &spiDat_s, 12, isoTxData);

        AE_LTC_CS_OFF();

        AE_delayTenUs(1);   //!< 10us delay

        AE_ltcWakeUpIdle();
        AE_delayTenUs(1);   //!< 10us delay


        AE_LTC_CS_ON();

//        spiTransmitAndReceiveData(spiREG1, &__spiDat_s, 12, cmdRDCFGA_pu16, isoRxData);

        spiTransmitData(spiREG1, &spiDat_s, 4, cmdRDCFGA_pu16);
        spiReceiveData(spiREG1, &spiDat_s, 8, isoRxData);

        AE_LTC_CS_OFF();

        AE_delayMs(10000);
        #endif

        #if 0   // code testing V1.0

        //!< configuration A setting
        AE_ltcWriteConfiguration(&cellInf);

        #endif


        #if 0   //!< ltc6820 data transfer

        AE_LTC_CS_ON();

        spiTransmitData(spiREG1, &spiDat_s, 4, txDat);

        AE_LTC_CS_OFF();

        #endif

        #if 0

            AE_ltcWakeUpSleep();
            LTC_status ltcStatus = AE_ltcRDCFG(&cellInf, RDCFGA);

        #endif


        #if 0      // timing test

        gioSetBit(gioPORTA, 2, 1);
        AE_delayMs(10);
        gioSetBit(gioPORTA, 2, 0);
        AE_delayMs(10);

        #endif

        AE_delayMs(100);

        if(status == LTC_OK);                   //!< close the status warning
        if(returnVarningClose == 10)    break;  //!< close return 0 warning
    }


/* USER CODE END */

    return 0;
}


/* USER CODE BEGIN (4) */

void ltcInit(spiBASE_t * spiReg)
{
    ltcBat.batConf.minCellVolt = 3.76;
    ltcBat.batConf.maxCellVolt = 4.2;

    ltcBat.batConf.adcopt = false;
    ltcBat.batConf.refon = true;

    ltcBat.batConf.gioAPullOffPin = GPIO_5 | GPIO_4 | GPIO_3;
    ltcBat.batConf.gioBPullOffPin = GPIO_8 | GPIO_7 | GPIO_6;

    ltcBat.batConf.numberOfSerialCell = 15;
    ltcBat.batConf.numberOfSlave = 1;

    ltcBat.batConf.dischargeCellCfgA4 = DCC_1 | DCC_2 | DCC_3 | DCC_4 | DCC_5 | DCC_6 | DCC_7 | DCC_8;    //if enabld discharge cell
    ltcBat.batConf.dischargeCellCfgA5 = DCC_9 | DCC_10| DCC_10| DCC_11 | DCC_12;
    ltcBat.batConf.dischargeCellCfgB0 = DCC_13 | DCC_14 | DCC_15;
    ltcBat.batConf.dischargeCellCfgB1 = DCC_0;

    ltcBat.batConf.dischargeTime = DIS_60_MIN;

    ltcBat.batConf.dischargeTimeMonitor = true;

    AE_ltcInit(spiReg, &ltcBat);
}


/* USER CODE END */
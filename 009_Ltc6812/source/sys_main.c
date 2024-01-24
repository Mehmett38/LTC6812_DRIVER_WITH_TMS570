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

#define UNUSED(x)   ((void)x)
uint8_t toggle = 1;
uint8_t returnVarningClose;

bool gpioAStatus[5] = {false, false, true, true, true};
bool gpioBStatus[4] = {false, false, false, false};
bool dccAStatus[12] = {false};
bool dccBStatus[4] = {false};

/* USER CODE END */

int main(void)
{
/* USER CODE BEGIN (3) */
    CellInf cellInf;
    _enable_interrupt_();

    gioInit();
    hetInit();
    pwmEnableNotification(hetREG1, pwm0, pwmEND_OF_PERIOD);
    pwmStart(hetRAM1, pwm0);

    AE_ltcInit(spiREG1, 1, &cellInf);

    AE_ltcSetcfgraRefOn(&cellInf, true);
    AE_ltcSetcfgraAdcOpt(&cellInf, false);
    AE_ltcSetcfgraGpio(&cellInf, gpioAStatus);
    AE_ltcSetcfgraDcc(&cellInf, dccAStatus);
    AE_ltcSetcfgraDct0(&cellInf, DIS_4_MIN);
    AE_ltcSetcfgraUnderVoltage(&cellInf, 3);
    AE_ltcSetcfgraOverVoltage(&cellInf, 4.2);
    Ae_ltcSetcfgrbFdrf(&cellInf, false);
    Ae_ltcSetcfgrbDTMEN(&cellInf, true);
    Ae_ltcSetcfgrbPs(&cellInf, PATH_ADC_ALL);
    AE_ltcSetcfgrbGpio(&cellInf, gpioBStatus);
    AE_ltcSetcfgrbDcc(&cellInf, dccBStatus);


    uint16_t isoTxData[8];
    uint16_t isoRxData[8] = {0};
    uint16_t eceTx;

    uint16_t isoData[6] = {0x7C, 0x77, 0x44, 0x22, 0x11, 0x55};

    eceTx = pec15((uint8_t*)isoData, 6);

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

    while(1)
    {
        #if 1   // code testing V1.0

        AE_ltcWakeUpSleep();

        AE_LTC_CS_ON();

        spiTransmitData(spiREG1, &__spiDat_s, 12, isoTxData);

        AE_LTC_CS_OFF();

        AE_delayTenUs(1);   //!< 10us delay

        AE_ltcWakeUpIdle();
        AE_delayTenUs(1);   //!< 10us delay


        AE_LTC_CS_ON();

//        spiTransmitAndReceiveData(spiREG1, &__spiDat_s, 12, cmdRDCFGA_pu16, isoRxData);

        spiTransmitData(spiREG1, &__spiDat_s, 4, cmdRDCFGA_pu16);
        spiReceiveData(spiREG1, &__spiDat_s, 8, isoRxData);

        AE_LTC_CS_OFF();

        AE_delayMs(1000);
        #endif

        #if 0   // code testing V1.1

        //!< configuration A setting
        AE_ltcWriteConfiguration(&cellInf);

        #endif


        #if 0   //!< ltc6820 data transfer

        AE_LTC_CS_ON();

        spiTransmitData(spiREG1, &__spiDat_s, 4, txDat);

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

        if(returnVarningClose == 10)    break;  //!< close return 0 warning
    }


/* USER CODE END */

    return 0;
}


/* USER CODE BEGIN (4) */




/* USER CODE END */

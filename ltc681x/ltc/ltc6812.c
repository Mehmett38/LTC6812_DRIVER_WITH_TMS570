/*
 * ltc6812.c
 *
 *  Created on: 19 Oca 2024
 *      Author: mehmet.dincer
 */



#include "ltc6812.h"
#include "crc15.h"

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<-MACROS->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<-GLOBAL VARIABLES->>>>>>>>>>>>>>>>>>>>>>>>>>>>>
volatile uint32_t uwTick;
static spiBASE_t * __ltcSpi_ps;     // spi base address
static uint8_t __slaveNumber_u8;    // number of slave bms device
static uint16_t __i;                // counter
uint16_t dummy_u16 = 0xFF;          // dummy variable
spiDAT1_t __spiDat_s =       // spi configuration parameters
{
     .CSNR = 0,
     .CS_HOLD = 1,
     .DFSEL = SPI_FMT_0,
     .WDEL = 0
};


//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<-FUNCTIONS->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
/**
 * @brief initialize spi line and set the global spiReg and slave number
 * @param[in] spiBase address
 * @param[in] slave number 0-255
 * @param[in] cell number, register conf number, ...
 * @return none
 */
void AE_ltcInit(spiBASE_t * ltcSpi_ps, uint8_t slaveNumber_u8, CellInf * cellinf_s)
{
    spiInit();
    init_PEC15_Table();

    __ltcSpi_ps = ltcSpi_ps;
    __slaveNumber_u8 = slaveNumber_u8;

    for(__i = 0; __i<__slaveNumber_u8; __i++)
    {
        //!NOTE degerler rasgele yazildi
        cellinf_s[__i].icReg.cellChannels_u8 = 13;
        cellinf_s[__i].icReg.statChannels_u8 = 7;
        cellinf_s[__i].icReg.auxChannels_u8 = 9;
        cellinf_s[__i].icReg.numCvReg_u8 = 5;
        cellinf_s[__i].icReg.numGpioReg_u8 = 4;
        cellinf_s[__i].icReg.statChannels_u8 = 2;
    }
}

/**
 * @brief write the configuration register
 * @param[in] BMS global structure
 * @param[in] WRCFGA or WRCFGB
 * @return none
 */
void AE_ltcWRCFG(CellInf *cellInf, WRCFG wrcfgX_e)
{
    uint16_t cmd[4];
    uint16_t * data_pu16;

    if(wrcfgX_e == WRCFGA)  // set the register addres and address crc
    {
        data_pu16 = cellInf->configA.txData_u8;

        for(__i = 0; __i < 4; __i++)
            cmd[__i] = cmdWRCFGA_pu16[__i];
    }
    else if(WRCFGB)
    {
        data_pu16 = cellInf->configB.txData_u8;

        for(__i = 0; __i < 4; __i++)
            cmd[__i] = cmdWRCFGB_pu16[__i];

    }
    AE_ltcWrite681x(cmd, data_pu16);
}

/**
 * @brief Combines cmd and data message by adding crc values and sends
 * @param[in] cmd[4] cmd[0,1] register address, cmd[2,3] address crc value
 * @param[in] tx-register values datas
 * @return none
 */
void AE_ltcWrite681x(uint16_t *txCmd_pu8, uint16_t * data_pu16)
{
    uint8_t currenIc_u8;        // current slave number
    uint8_t currenByte_u8;      // register current byte number
    uint16_t dataPec_u16;       // crc15 value
    uint8_t cmdIndex_u8 = 4;    /* x.th index of the data to write the cmd buffer
                                 first 4 index command + crc*/
    uint8_t cmdLen_u8 = 4 + (8 * __slaveNumber_u8); // 8 = 6 byte cmd + 2 byte crc
    uint16_t cmd[256];

    for(currenIc_u8 = 0; currenIc_u8 < __slaveNumber_u8; currenIc_u8++)
    {
        for(currenByte_u8 = 0; currenByte_u8 < WRITE_BYTES_IN_REG; currenByte_u8++)
        {
            cmd[cmdIndex_u8++] = data_pu16[(currenIc_u8 * WRITE_BYTES_IN_REG) + currenByte_u8];
        }

        dataPec_u16 = pec15((uint8_t *)&data_pu16[(currenIc_u8 * 6) + currenByte_u8], WRITE_BYTES_IN_REG);
        cmd[cmdIndex_u8++] = (dataPec_u16 >> 8) & 0xFF;
        cmd[cmdIndex_u8++] = (dataPec_u16 >> 0) & 0xFF;
    }

    AE_LTC_CS_ON();

    spiTransmitData(__ltcSpi_ps, &__spiDat_s, cmdLen_u8, cmd);

    AE_LTC_CS_OFF();
}

/**
 * @brief Read configuration registers
 * @param[in] BMS global structure
 * @param[in] RDCFGA or RDCFGB
 * @return success : fail ? LTC_OK : LTC_WRONG_CRC
 */
LTC_status AE_ltcRDCFG(CellInf *cellInf, RDCFG rdcfg_e)
{
    uint16_t cmdReg[4];
    uint16_t *ltcRxDatas_pu16;

    if(rdcfg_e == RDCFGA)
    {
        ltcRxDatas_pu16 = cellInf->configA.rxData_u8;

        for(__i = 0; __i < 4; __i++)
        {
            cmdReg[__i] = cmdRDCFGA_pu16[__i];
        }
    }
    else
    {
        ltcRxDatas_pu16 = cellInf->configB.rxData_u8;

        for(__i = 0; __i < 4; __i++)
        {
            cmdReg[__i] = cmdRDCFGB_pu16[__i];
        }
    }

    return AE_ltcRead681x(cmdReg, ltcRxDatas_pu16);
}

/**
 * @brief read the register compare the readed and calculated crc then return compare result
 * @param[in] cmd[4] cmd[0,1] register address, cmd[2,3] address crc value
 * @param[in] rx-register values datas
 * @return success : fail ? LTC_OK : LTC_WRONG_CRC
 */
LTC_status AE_ltcRead681x(uint16_t *txCmd_pu8, uint16_t * data_pu16)
{
    uint16_t crcVal_u16;
    uint16_t crcRead_u16;   //readed crc value
    LTC_status ltcStatus_s;

    AE_LTC_CS_ON();

    spiTransmitData(__ltcSpi_ps, &__spiDat_s, 4, txCmd_pu8);    //cmd command write

    spiReceiveData(__ltcSpi_ps, &__spiDat_s, (READ_BYTES_IN_REG * __slaveNumber_u8), &data_pu16[__i]);

    AE_LTC_CS_OFF();

    for(__i = 0; __i < __slaveNumber_u8; __i++)
    {
        //! 6th index crc pec0, 7th index crc pec1 value
        crcVal_u16 = (data_pu16[ __i * WRITE_BYTES_IN_REG + 6] << 8) | (data_pu16[ __i * WRITE_BYTES_IN_REG + 7] << 0);
        crcRead_u16 = pec15((uint8_t*)&data_pu16[ __i * WRITE_BYTES_IN_REG], 6);

        ltcStatus_s = (crcVal_u16 != crcRead_u16) ? LTC_WRONG_CRC : LTC_OK;
    }

    return ltcStatus_s;
}

/**
 *@brief 1->References Remain Powered Up Until Watchdog Timeout
 *       0->References Shut Down After Conversions (Default)
 *@param[in] BMS global variable, refOn on or off
 *@param[in] refOn 1->on else off
 *@return none
 */
void AE_ltcSetcfgraRefOn(CellInf *cellInf_ps, bool refOn_b)
{
    if(refOn_b)
        cellInf_ps->configA.txData_u8[0] |= 0x04;
    else
        cellInf_ps->configA.txData_u8[0] &= ~(0x04);
}

/**
 * @brief 0 -> Selects Modes 27kHz, 7kHz, 422Hz or 26Hz with MD[1:0] Bits in ADC Conversion Commands (Default)
          1 -> Selects Modes 14kHz, 3kHz, 1kHz or 2kHz with MD[1:0] Bits in ADC Conversion Commands
 *@param[in] BMS globa variable
 *@param[in] adcOpt 1->on else off
 */
void AE_ltcSetcfgraAdcOpt(CellInf *cellInf_ps, bool adcOpt_b)
{
    if(adcOpt_b)
        cellInf_ps->configA.txData_u8[0] |= 0x01;
    else
        cellInf_ps->configA.txData_u8[0] &= ~(0x01);
}

/**
 * @brief Write: 0 -> GPIOx Pin Pull-Down ON; 1 -> GPIOx Pin Pull-Down OFF (Default)
          Read: 0 -> GPIOx Pin at Logic 0; 1 -> GPIOx Pin at Logic 1
 * @param[in] BMS global variable
 * @param[in] gpio global variable
 * @return none
 */
void AE_ltcSetcfgraGpio(CellInf *cellInf_ps, bool gpio[5])
{
    for(__i = 0; __i < 5; __i++)
    {
        if(gpio[__i])
            cellInf_ps->configA.txData_u8[0] |= (1<<(3+__i));
        else
            cellInf_ps->configA.txData_u8[0] &= ~(1<< (3+__i));
    }
}

/**
 * @brief x = 1 to 15: 1 -> Turn ON Shorting Switch for Cell x
 *                     0 -> Turn OFF Shorting Switch for Cell x (Default)
 *        x = 0:       1 -> Turn ON GPIO9 Pull-Down
 *                     0 -> Turn OFF GPIO9 Pull-Down (Default)
 * @param[in] BMS global variable
 * @param[in] discharge cell x is on or off
 * @return none
 */
void AE_ltcSetcfgraDcc(CellInf *cellInf_ps, bool dcc[12])
{
    for(__i = 0; __i < 8; __i++)    //CFGAR4
    {
        if(dcc[__i])
            cellInf_ps->configA.txData_u8[4] |= (1 << __i);
        else
            cellInf_ps->configA.txData_u8[4] &= ~(1 << __i);
    }

    for(__i = 0; __i < 4; __i++)    //CFGAR5
    {
        if(dcc[__i + 8])
            cellInf_ps->configA.txData_u8[5] |= (1 << __i);
        else
            cellInf_ps->configA.txData_u8[5] &= ~(1 << __i);
    }
}

/**
 * @brief Discharge Time Out Value
 * param[in] bms global variable
 * @param[in] disabled time durations
 * @return none
 */
void AE_ltcSetcfgraDct0(CellInf*cellInf_ps, DischargeTime dct0)
{
    cellInf_ps->configA.txData_u8[5] &= ~(0xF0);    //clear register
    cellInf_ps->configA.txData_u8[5] |= dct0;
}

/**
 * @brief Undervoltage Comparison Voltage
 * @param[in] BMS gloval variable
 * @param[in] under voltage value
 * @return none
 */
void AE_ltcSetcfgraUnderVoltage(CellInf*cellInf_ps, float underVolt)
{
    //!< voltage = (UV + 1) * 16 * 100uV, UV = 625 * voltage - 1

    uint16_t uv_u16 = 625 * underVolt - 1;

    cellInf_ps->configA.txData_u8[1] &= 0xFF;   //clear the register
    cellInf_ps->configA.txData_u8[1] |= uv_u16 & 0xFF;

    cellInf_ps->configA.txData_u8[2] &= ~(0x0F);   //clear the register
    cellInf_ps->configA.txData_u8[2] |= (uv_u16 >> 8) & 0x0F;
}

/**
 * @brief Overvoltage Comparison voltage
 * @param[in] BMS global variable
 * @param[in] overVoltage value
 */
void AE_ltcSetcfgraOverVoltage(CellInf*cellInf_ps, float overVolt)
{
    //!< voltage = OV * 16 * 100uV, OV = 625 * voltage

    uint16_t ov_u16 = 625 * overVolt;

    cellInf_ps->configA.txData_u8[2] &= ~(0xF0);   //clear the register
    cellInf_ps->configA.txData_u8[2] |= (ov_u16 << 4) & 0xF0;

    cellInf_ps->configA.txData_u8[3] &= 0x00;       //clear the register
    cellInf_ps->configA.txData_u8[3] |= (ov_u16 >> 4) & 0xFF;       //clear the register
}

/**
 * @brief Force Digital Redundancy Failure
 * @param[in] BMS global variable
 * @param[in] on or off
 */
void Ae_ltcSetcfgrbFdrf(CellInf *cellInf_ps, bool fdrf)
{
    if(fdrf)
        cellInf_ps->configB.txData_u8[1] |= (1<<6);
    else
        cellInf_ps->configB.txData_u8[1] &= ~(1<<6);
}

/**
 * @brief  Enable Discharge Timer Monitor
 * @param[in] BMS gloval variable
 * @param[in] on or off
 * @return none
 */
void Ae_ltcSetcfgrbDTMEN(CellInf *cellInf_ps, bool dtmen)
{
    if(dtmen)
        cellInf_ps->configB.txData_u8[1] |= (1<<3);
    else
        cellInf_ps->configB.txData_u8[1] &= ~(1<<3);
}

/**
 * @brief Digital Redundancy Path Selection
 * @param[in] BMS global variable
 * @param[in] adc path selection
 */
void Ae_ltcSetcfgrbPs(CellInf *cellInf_ps, PathSelection ps)
{
    cellInf_ps->configB.txData_u8[1] &= ~(0x30);
    cellInf_ps->configB.txData_u8[1] |= (ps << 4);
}

/**
 * @brief Write: 0 -> GPIOx Pin Pull-Down ON; 1 -> GPIOx Pin Pull-Down OFF (Default)
 *         Read: 0 -> GPIOx Pin at Logic 0; 1 -> GPIOx Pin at Logic 1
 * @param[in] BMS global variable
 * @param[in] gpi pin that set pull down or not
 */
void AE_ltcSetcfgrbGpio(CellInf *cellInf_ps, bool gpio[4])
{
    for(__i = 0; __i < 4; __i++)
    {
        if(gpio[__i])
            cellInf_ps->configB.txData_u8[0] |= (1<<(__i));
        else
            cellInf_ps->configB.txData_u8[0] &= ~(1<< (__i));
    }
}

/**
 * @brief x = 1 to 15: 1 -> Turn ON Shorting Switch for Cell x
 *                     0 -> Turn OFF Shorting Switch for Cell x (Default)
 *        x = 0:       1 -> Turn ON GPIO9 Pull-Down
 *                     0 -> Turn OFF GPIO9 Pull-Down (Default)
 * @param[in] BMS global variable
 * @param[in] discharge cell x is on or off
 * @return none
 */
void AE_ltcSetcfgrbDcc(CellInf *cellInf_ps, bool dcc[4])
{
    cellInf_ps->configB.txData_u8[1] &= ~(1 << 2);
    cellInf_ps->configB.txData_u8[1] |= (dcc[0] << 2);

    cellInf_ps->configB.txData_u8[0] &= ~(0x70);
    cellInf_ps->configB.txData_u8[0] |= (dcc[1] << 4) | (dcc[2] << 5) | (dcc[3] << 6);
}

/**
 * @brief Write configuration register
 */
LTC_status AE_ltcWriteConfiguration(CellInf *cellInf)
{
    LTC_status ltcStatus;

    AE_ltcWakeUpSleep();    //!< pass from sleep to Standby mode

    AE_ltcWRCFG(cellInf, WRCFGA);   //!< write to configuration register A
    AE_ltcWRCFG(cellInf, WRCFGB);   //!< write to configuration register B

    AE_ltcWakeUpIdle();             //!< enable the tranmission and receive data

    ltcStatus = AE_ltcRDCFG(cellInf, RDCFGA);
    if(ltcStatus != LTC_OK)
        return LTC_WRONG_CRC;

    return AE_ltcRDCFG(cellInf, RDCFGB);
}

/**
 * @brief ltc6812 enter standby mode
 * @return none
 */
void AE_ltcWakeUpSleep()
{
    for(__i = 0; __i < __slaveNumber_u8; __i++)
    {
        AE_LTC_CS_ON();

        spiTransmitData(__ltcSpi_ps, &__spiDat_s, 1, &dummy_u16);

        AE_LTC_CS_OFF();
    }

    AE_delayTenUs(10);  //!< 300us delay
}

/**
 * @brief Wake isoSPI up from IDlE state and enters the READY state
 * @return none
 */
void AE_ltcWakeUpIdle()
{
    for(__i = 0; __i < __slaveNumber_u8; __i++)
    {
        AE_LTC_CS_ON();

        spiTransmitData(__ltcSpi_ps, &__spiDat_s, 1, &dummy_u16);

        AE_LTC_CS_OFF();

    }
}

/**
 * @brief pwm period complete callback
 */
void pwmNotification(hetBASE_t * hetREG,uint32 pwm, uint32 notification)
{
    uwTick++;
}

/**
 * @brief return the tick sum
 * @return total tick
 */
uint32_t getUsTick()
{
    return uwTick;
}

/**
 * @brief delay time milisecond
 * @param[in] milisecond delay time
 */
void AE_delayMs(uint32_t delay_u32)
{
    delay_u32 *= 100;

    uint32_t tickStart = getUsTick();

    while((getUsTick() - tickStart) < delay_u32);
}

/**
 * @brief delay 10us
 * @note delay tick is 10us so total delay = 10 * delay_u32
 * @param[in] delay time
 * @return none
 */
void AE_delayTenUs(uint32_t delay_u32)
{
    uint32_t tickStart = getUsTick();

    while(((getUsTick() - tickStart)) < delay_u32);
}







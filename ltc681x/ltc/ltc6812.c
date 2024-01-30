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
uint16_t txBuffer[256];
uint16_t rxBuffer[256];
volatile uint32_t uwTick;
static spiBASE_t * ltcSpi_ps;     // spi base address
uint8_t slaveNumber;              // bms total connection
static uint16_t i;                // counter
uint16_t dummy_u16 = 0xFF;        // dummy variable
spiDAT1_t spiDat_s =              // spi configuration parameters
{
     .CSNR = 0,
     .CS_HOLD = 1,
     .DFSEL = SPI_FMT_0,
     .WDEL = 0
};


//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<-FUNCTIONS->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
/**
 * @brief initialize spi line and set the VO, VUV
 * @param[in] spiBase address
 * @param[in] cell number, over voltage and under voltage informatin included
 * @return none
 */
void AE_ltcInit(spiBASE_t * spi, Ltc682x * ltcBat)
{
    spiInit();
    init_PEC15_Table(); //!< Crc start table

    hetInit();
    _enable_interrupt_();
    pwmEnableNotification(hetREG1, pwm0, pwmEND_OF_PERIOD);
    pwmStart(hetRAM1, pwm0);

    ltcSpi_ps = spi;
    slaveNumber = ltcBat->batConf.numberOfSlave;

    memset(&ltcBat->cfgAr, 0, sizeof(CFGAR));
    memset(&ltcBat->cfgBr, 0, sizeof(CFGBR));

    //!< configuration register A
    ltcBat->cfgAr.CFGAR0.ADCOPT |= ltcBat->batConf.adcopt;
    ltcBat->cfgAr.CFGAR0.REFON |= ltcBat->batConf.refon;
    ltcBat->cfgAr.CFGAR0.cfg |= ltcBat->batConf.gioAPullOffPin;

    //!< configuration register B
    ltcBat->cfgBr.CFGBR0.cfg |= (ltcBat->batConf.gioBPullOffPin) & 0x000F;

    ltcBat->cfgBr.CFGBR1.DTMEN |= ltcBat->batConf.dischargeTimeMonitor;

    AE_ltcWrite((uint16_t*)&ltcBat->cfgAr, cmdWRCFGA_pu16);
    AE_ltcWrite((uint16_t*)&ltcBat->cfgBr, cmdWRCFGB_pu16);
}

/**
 * @brief It fills the given values into the buffer by adding crc to the end and then sends them.
 * @param[in] transmitted datas address
 * @param[in] cmd command + 2byte crc
 * @return none
 */
void AE_ltcWrite(uint16_t * txData, uint16_t cmd[4])
{
    uint8_t bufferLen = 4 + slaveNumber * 8;    // 4 = crc, 8 = 6 byte data + 2 byte crc

    for(i = 0; i < 4; i++)
    {
        txBuffer[i] = cmd[i];
    }

    uint8_t j;
    uint16_t pec;

    for(i = 0; i < slaveNumber; i++)
    {
        for(j = 0; j < TRANSMIT_LEN; j++)
        {
            txBuffer[4 + 8 * i + j] = txData[i * TRANSMIT_LEN + j];
        }

        pec = AE_pec15((uint8_t*)&txData[i * TRANSMIT_LEN], 6);
        txBuffer[4 + 8 * i + 6] = (pec >> 8) & 0xFF;
        txBuffer[4 + 8 * i + 7] = (pec >> 0) & 0xFF;
    }

    AE_ltcWakeUpSleep();

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, bufferLen, txBuffer);

    AE_LTC_CS_OFF();

    AE_delayTenUs(1);   //!< 10us delay
}

/**
 * @brief Read the cmd register and assign to rxData address
 * @param[in] return data address
 * @param[in] cmd + byte crc
 * @return if crc is suitable with message return OK else CRC_ERROR
 */
LTC_status AE_ltcRead(uint16_t * rxData, uint16_t cmd[4])
{
    AE_ltcWakeUpSleep();
    AE_ltcWakeUpIdle();

    AE_delayTenUs(1);   //!< 10us delay

    AE_LTC_CS_ON();

    spiTransmitData(spiREG1, &spiDat_s, 4, cmd);
    spiReceiveData(spiREG1, &spiDat_s, (slaveNumber * RECEIVE_LEN), rxBuffer);

    AE_LTC_CS_OFF();

    uint8_t j;
    for(i = 0; i < slaveNumber; i++)
    {
        uint16_t pec = AE_pec15((uint8_t*)&rxBuffer[RECEIVE_LEN * i], 6);

        uint16_t readPec = rxBuffer[RECEIVE_LEN * i + 6] << 8 | rxBuffer[RECEIVE_LEN * i + 7];

        if(pec != readPec) return LTC_WRONG_CRC;

        for(j = 0; j < RECEIVE_LEN; j++)
        {
            rxData[j] = rxBuffer[RECEIVE_LEN * i + j];
        }
    }

    return LTC_OK;
}

/**
 * @brief ltc6812 enter standby mode
 * @return none
 */
void AE_ltcWakeUpSleep()
{
    for(i = 0; i < slaveNumber ; i++)
    {
        AE_LTC_CS_ON();

        spiTransmitData(ltcSpi_ps, &spiDat_s, 1, &dummy_u16);

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
    for(i = 0; i < slaveNumber; i++)
    {
        AE_LTC_CS_ON();

        spiTransmitData(ltcSpi_ps, &spiDat_s, 1, &dummy_u16);

        AE_LTC_CS_OFF();
    }
}

/**
 * @brief Read the cell voltage
 * @param[in] ltcBat global variable
 * @return if crc is suitable with message retur ok else crcError
 */
LTC_status AE_ltcReadCellVoltage(Ltc682x * ltcBat)
{
    LTC_status status;
    float * fptr = (float*)&ltcBat->volt.cell1;

    //!< voltage register Group A
    status = AE_ltcRead(rxBuffer, cmdRDCVA_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < 3; i++)
    {   //offset + 12
        fptr[i] = ((rxBuffer[i*2] << 0) | (rxBuffer[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
    }

    //!< voltage register Group B
    status = AE_ltcRead(rxBuffer, cmdRDCVB_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < 3; i++)
    {   //offset + 12
        fptr[i + 3] = ((rxBuffer[i*2] << 0) | (rxBuffer[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
    }

    //!< voltage register Group C
    status = AE_ltcRead(rxBuffer, cmdRDCVC_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < 3; i++)
    {   //offset + 12
        fptr[i + 6] = ((rxBuffer[i*2] << 0) | (rxBuffer[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
    }

    //!< voltage register Group D
    status = AE_ltcRead(rxBuffer, cmdRDCVD_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < 3; i++)
    {   //offset + 12
        fptr[i + 9] = ((rxBuffer[i*2] << 0) | (rxBuffer[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
    }

    //!< voltage register Group E
    status = AE_ltcRead(rxBuffer, cmdRDCVE_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < 3; i++)
    {   //offset + 12
        fptr[i + 12] = ((rxBuffer[i*2] << 0) | (rxBuffer[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
    }

    return status;
}

/**
 * @brief Start Cell Voltage ADC Conversion and Poll Status
 * @param[in] ltcBat global variable
 * @param[in] 0 Discharge Not Permitted 1 Discharge Permitted
 * @param[in] cell selection for adc conversion for parameter search @refgroup CH
 * @return if crc is suitable with message retur ok else crcError
 */
void AE_ltcStartCellAdc(Ltc682x * ltcBat, AdcMode adcMode, uint8_t dischargePermit, uint8_t CELL)
{
    uint16_t adcvReg = 0x0260;   //!< base value of ADCV register

    if(adcMode & 0x10)
    {
        if(!ltcBat->cfgAr.CFGAR0.ADCOPT)
        {
            ltcBat->cfgAr.CFGAR0.ADCOPT = 1;
            AE_ltcWrite((uint16_t*)&ltcBat->cfgAr, cmdWRCFGA_pu16);
        }
    }

    adcvReg |= (adcMode << 7);
    adcvReg |= (dischargePermit << 4);
    adcvReg |= CELL;

    uint16_t cmd[4] = {0};
    uint16_t pec;

    cmd[0] = (adcvReg >> 8) & 0x00FF;
    cmd[1] = (adcvReg >> 0) & 0x00FF;
    pec = AE_pec15((uint8_t*)cmd, 2);
    cmd[2] = (pec >> 8) & 0x00FF;
    cmd[3] = (pec >> 0) & 0x00FF;

    AE_ltcWakeUpSleep();

    AE_LTC_CS_ON();
    spiTransmitData(spiREG1, &spiDat_s, 4, cmd);
    AE_LTC_CS_OFF();
}

/**
 * @brief after starting to cell adc measure, we can control process is finish or not
 * @return if measure is completed return 1 else 0
 */
uint8_t AE_ltcAdcMeasureState()
{
    uint16_t cmd[4] = {0x07, 0x14, 0xF3, 0x6C};
    uint16_t returnVal;

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, 4, cmd);
    spiReceiveData(ltcSpi_ps, &spiDat_s, 1, &returnVal);

    AE_LTC_CS_OFF();

    return (uint8_t)returnVal;
}

/**
 * @brief Start the adc conversion for the GPIO pins
 * @param[in] bms global variable
 * @param[in] adc speed selection
 * @param[in] gio pin that want to read, to parameter search @refgroup CHG
 * @return none
 */
void AE_ltcStartGpioAdc(Ltc682x * ltcBat, AdcMode adcMode, uint8_t GPIO_)
{
    uint16_t adax = 0x0460;     //!< adax register default value
    uint16_t cmd[4];
    uint16_t pec;

    if(adcMode & 0x10)
    {
        if(!ltcBat->cfgAr.CFGAR0.ADCOPT)
        {
            ltcBat->cfgAr.CFGAR0.ADCOPT = 1;
            AE_ltcWrite((uint16_t*)&ltcBat->cfgAr.CFGAR0, cmdWRCFGA_pu16);
        }
    }

    adax |= (adcMode & 0x0F) << 7;
    adax |= GPIO_;

    cmd[0] = (adax & 0xFF00) >> 8;
    cmd[1] = (adax & 0x00FF) >> 0;

    pec = AE_pec15((uint8_t*)cmd, 2);
    cmd[2] = (pec & 0xFF00) >> 8;
    cmd[3] = (pec & 0x00FF) >> 0;

    AE_ltcWakeUpSleep();

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, 4, cmd);

    AE_LTC_CS_OFF();
}

/**
 * @brief Read the GPIO1-9 pins voltage
 * @param[in] assign to ltcBatGpio
 * @return if pec is true OK else
 */
LTC_status AE_ltcReadGPIOVoltage(Ltc682x * ltcBat)
{
    LTC_status status;
    float * fptr = (float*)&ltcBat->gpio.gpio1;

    //!< voltage register Group A
    status = AE_ltcRead(rxBuffer, cmdRDAUXA_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < 3; i++)
    {   //offset + 12
        fptr[i] = ((rxBuffer[i*2] << 0) | (rxBuffer[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
    }

    //!< voltage register Group B
    status = AE_ltcRead(rxBuffer, cmdRDAUXB_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < 3; i++)
    {   //offset + 12
        fptr[i + 3] = ((rxBuffer[i*2] << 0) | (rxBuffer[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
    }

    //!< voltage register Group C
    status = AE_ltcRead(rxBuffer, cmdRDAUXC_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < 3; i++)
    {   //offset + 12
        fptr[i + 6] = ((rxBuffer[i*2] << 0) | (rxBuffer[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
    }

    //!< voltage register Group D
    status = AE_ltcRead(rxBuffer, cmdRDAUXD_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < 1; i++)
    {   //offset + 12
        fptr[i + 9] = ((rxBuffer[i*2] << 0) | (rxBuffer[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
    }

    return status;
}

/**
 * @brief before reading the status register, this function must be called
 * @param[in] bms global variable
 * @param[in] adc speed selection
 * @param[in] Status group selection, for parameter search @refgroup CHST
 */
void AE_ltcStartStatusAdc(Ltc682x * ltcBat, AdcMode adcMode, uint8_t CHST_)
{
    uint16_t adstat= 0x0468;    //!< ADSTAT base register
    uint16_t cmd[4];
    uint16_t pec;

    if(adcMode & 0x10)
    {
        if(!ltcBat->cfgAr.CFGAR0.ADCOPT)
        {
            ltcBat->cfgAr.CFGAR0.ADCOPT = 1;
            AE_ltcWrite((uint16_t*)&ltcBat->cfgAr.CFGAR0, cmdWRCFGA_pu16);
        }
    }

    adstat |= (adcMode & 0x0F) << 7;
    adstat |= CHST_;

    cmd[0] = (adstat & 0xFF00) >> 8;
    cmd[1] = (adstat & 0x00FF) >> 0;

    pec = AE_pec15((uint8_t*)cmd, 2);
    cmd[2] = (pec & 0xFF00) >> 8;
    cmd[3] = (pec & 0x00FF) >> 0;

    AE_ltcWakeUpSleep();

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, 4, cmd);

    AE_LTC_CS_OFF();
}

/**
 * @brief read the status register A
 * @param[in] bms global register
 * @return if pec is correct retur ok else crcError
 */
LTC_status AE_ltcReadStatusRegA(Ltc682x * ltcBat)
{
    LTC_status status;

    float *fptr = (float*)&ltcBat->statusRegA.sumOfCell;

    status = AE_ltcRead(rxBuffer, cmdRDSTATA_pu16);
    if(status == LTC_WRONG_CRC) return status;


    fptr[0] = rxBuffer[0] | rxBuffer[1] << 8;
    fptr[0] = fptr[0] / 10000.0 * 30.0;                 //Cells Voltage = SC • 100μV • 30

    fptr[1] = rxBuffer[2] | rxBuffer[3] << 8;
    fptr[1] = fptr[1] * 100.0 / 7.6 /1000.0 - 276;      //ITMP • 100μV/7.6mV/C – 276C

    fptr[2] = rxBuffer[4] | rxBuffer[5] << 8;
    fptr[2] = fptr[2] / 10000.0;                        //Analog Power Supply Voltage = VA • 100μV

    return status;
}

/**
 * @brief read the status register A
 * @param[in] bms global register
 * @return if pec is correct retur ok else crcError
 */
LTC_status AE_ltcReadStatusRegB(Ltc682x * ltcBat)
{
    LTC_status status;

    status = AE_ltcRead(rxBuffer, cmdRDSTATB_pu16);
    if(status == LTC_WRONG_CRC) return status;

    ltcBat->statusRegB.digitalPowerSupplyVolt = (rxBuffer[0] << 8 | rxBuffer[1])/10000.0;

    //!< if you think this function looks like too complex, yes you are right
    //!< if this function did not written like this, 24 line code is needed
    //!< rxBuffer[i/4 + 2] => (+2) is offset related to Status Register Group B
    //!< 0x02 << ((i % 4) * 2)) select the bit 1,3,5,7
    //!< 0x01 << ((i % 4) * 2)) select the bit 0,2,4,6
    //!< if that ↑ index is ( >0 ) assign this bit to (1 << i)
    for(i = 0; i < 12; i++)
    {
        ltcBat->statusRegB.CellOverFlag.flag |=
                ((rxBuffer[i / 4 + 2] & (0x02 << ((i % 4) * 2))) != 0) ? (1<<i) : 0;
        ltcBat->statusRegB.CellUnderFlag.flag |=
                ((rxBuffer[i / 4 + 2] & (0x01 << ((i % 4) * 2))) != 0) ? (1<<i) : 0;
    }

    ltcBat->statusRegB.thsd = rxBuffer[5] & 0x01;
    ltcBat->statusRegB.muxFail = (rxBuffer[5] >> 1) & 0x01;

    ltcBat->statusRegB.revionCode = (rxBuffer[5] >> 5) & 0x0F;

    return status;
}

/**
 * @brief stop the cell adc conversion
 * @param[in] bms global variable
 * @return if packet is true ok else crc error
 */
LTC_status AE_ltcClearCellAdc()
{
    Ltc682x ltcTemp;
    LTC_status status;

    AE_ltcWakeUpSleep();

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, 4, cmdCLRCELL_pu16);

    AE_LTC_CS_OFF();

    status = AE_ltcReadCellVoltage(&ltcTemp);
    if(status == LTC_WRONG_CRC) return status;

    if(ltcTemp.volt.cell1 > 6.5)        //!< if adc conversion is close read the pin 0xFF but this variable is
        return LTC_OK;                  // float, so value > 6.5 condition is consistent
    else
        return LTC_WRONG_CRC;
}

/**
 * @brief stop the gpio adc conversion
 * @param[in] bms global variable
 * @return if packet is true ok else crc error
 */
LTC_status AE_ltcClearGpioAdc(Ltc682x * ltcBat)
{
    LTC_status status;

    AE_ltcWakeUpSleep();

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, 4, cmdCLRAUX_pu16);

    AE_LTC_CS_OFF();

    status = AE_ltcReadGPIOVoltage(ltcBat);
    if(status == LTC_WRONG_CRC) return status;

    if(ltcBat->gpio.gpio1 > 6.5)        //!< //!< if adc conversion is close read the pin 0xFF but this variable is
        return LTC_OK;                  // float, so value > 6.5 condition is consistent
    else
        return LTC_WRONG_CRC;
}

/**
 * @brief stop the status adc conversion
 * @param[in] bms global variable
 * @return if packet is true ok else crc error
 */
LTC_status AE_ltcClearStatusAdc(Ltc682x * ltcBat)
{
    LTC_status status;

    AE_ltcWakeUpSleep();

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, 4, cmdCLRSTAT_pu16);

    AE_LTC_CS_OFF();

    status = AE_ltcReadStatusRegA(ltcBat);
    if(status == LTC_WRONG_CRC) return status;

    if(ltcBat->gpio.gpio1 > 6.5)        //!< //!< if adc conversion is close read the pin 0xFF but this variable is
        return LTC_OK;                  // float, so value > 6.5 condition is consistent
    else
        return LTC_WRONG_CRC;
}

/**
 * @brief set the sPin_x duty cycle
 * @param[in] bms global variable
 * @param[in] S output pin number, for parameter search @refgroup sPin
 * @param[in] pwm duty level level0-level15, offset 100/16 search @refgroup pwmDutyLevel
 * @return none
 */
void AE_ltcStartPwm(Ltc682x * ltcBat, uint16_t S_PIN_, uint8_t PWM_DUTY_LEVEL_)
{
    uint16_t pwmDuty[12] = {0};   //!< pwm blocks valid on PWM Register Group and PWM/S Control Register Group B
                            //!< so, send 12 bytes data
    uint8_t mode = 0;
    uint8_t index = 0;

    for(i = 0; i < 15; i++) //!< 15 == S pin number
    {
        if((S_PIN_ >> i) & 0x01)
        {
            index = i / 2;
            mode = i % 2;

            pwmDuty[index] |= (PWM_DUTY_LEVEL_ << (mode * 4));
        }
    }

    AE_ltcWrite(pwmDuty, cmdWRPWM_pu16);
    AE_ltcWrite(&pwmDuty[6], cmdWRPSB_pu16);
}

/**
 * @brief if pwm
 */
LTC_status AE_ltcReadStatusPwm(Ltc682x * ltcBat)
{
    LTC_status status = LTC_WRONG_CRC;
    return status;
}

/**
 * @brief if pwm is started pause the pw
 * @param[in] bms global variable
 * @return none
 */
void AE_ltcPausePwm(Ltc682x * ltcBat)
{
    AE_ltcWakeUpSleep();

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, 4, cmdMute_pu16);

    AE_LTC_CS_OFF();
}

/*
 * @brief if pwm is paused, restart pwm
 * @param[in] bms global variable
 * @return none
 */
void AE_ltcContinuePwm(Ltc682x * ltcBat)
{
    AE_ltcWakeUpSleep();

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, 4, cmdUnMute_pu16);

    AE_LTC_CS_OFF();
}

/**
 * @brief start the bms balance
 * @param[in] bms global variable
 * @param[in] discharge time
 * @param[in] discharge underVolt
 * @param[in] discharge overVolt
 * @param[in] pin that want to balance, for parameter search @refgroup dcc
 * @return none
 */
void AE_ltcSetBalance(Ltc682x * ltcBat, DischargeTime DIS_, float underVolt, float overVolt, uint16_t DCC_)
{
    uint16_t underVoltTemp;     // (VUV + 1) * 16 * 10uV
    uint16_t overVoltTemp;      // VOV * 16 * 10uV
    uint16_t maskedDCC;

    underVoltTemp = underVolt * 625.0 - 1;
    overVoltTemp = overVolt * 625;

    //DCC pins are scattered in several registers so we must mask them
    //Register Group A, CFGRA4 Mask = pin1-8    0x1FE
    maskedDCC = DCC_ & 0x1FE;
    maskedDCC >>= 1;           //pin1 start index 1 and dcc1 located 0th index in register
    ltcBat->cfgAr.CFGAR4.cfg |= maskedDCC;

    //Register Group A, CFGRA5 Mask = pin9-12    0x1E00
    maskedDCC = DCC_ & 0x1E00;
    maskedDCC >>= 9;            //pin9 start index 9 and dcc9 located 0th index in register
    ltcBat->cfgAr.CFGAR5.cfg |= maskedDCC;

    //Register Group B, CFGBR0 Mask = pin13-15    0xE000
    maskedDCC = DCC_ & 0xE000;
    maskedDCC >>= 9;            //pin13 start index 13 and dcc13 located 4th index in register (13 - 4)
    ltcBat->cfgBr.CFGBR0.cfg |= maskedDCC;

    //Register Group B, CFGBR1 Mask = pin0    0xE000
    maskedDCC = DCC_ & 0x1;
    maskedDCC <<= 2;            //pin0 start index 0 and dcc0 located 2th index in register (0 - 2)
    ltcBat->cfgBr.CFGBR1.cfg |= maskedDCC;


    //!< enable discharge monitoring and set under and over voltage
    ltcBat->cfgAr.CFGAR1.cfg |= underVoltTemp & 0x00FF;
    ltcBat->cfgAr.CFGAR2.cfg |= (underVoltTemp >> 8) & 0x000F;

    ltcBat->cfgAr.CFGAR2.cfg |= (overVoltTemp << 4) & 0x00F0;
    ltcBat->cfgAr.CFGAR3.cfg |= (overVoltTemp >> 4) & 0x00FF;

    //set discharge duration
    ltcBat->cfgAr.CFGAR5.cfg |= (DIS_ << 4) & 0x00F0;

    AE_ltcWrite((uint16_t*)&ltcBat->cfgAr, cmdWRCFGA_pu16);
    AE_ltcWrite((uint16_t*)&ltcBat->cfgBr, cmdWRCFGB_pu16);
}

/**
 * @brief return the minimum cell voltage
 * @param[in] bms global variable
 * @return minimum cell voltage
 */
float AE_ltcMinCellVolt(Ltc682x * ltcBat)
{
    float *fptr = (float *)&ltcBat->volt.cell1;
    float min;
    min = fptr[0];

    for(i = 1; i < 15; i++)
    {
        if(fptr[i] < min)
            min = fptr[i];
    }

    return min;
}

/**
 * @brief check for any open wires between the ADCs of the ltc681x
 * @param[in] bms global variable
 * @param[in] adc mode selection
 * @param[in] checked cell selection, for parameter search @refgroup CHG
 * @return if no error return OK else OPEN_WIRE
 */
LTC_status AE_ltcIsCellOpenWire(Ltc682x * ltcBat, AdcMode adcMode, uint8_t CELL_)
{
    Ltc682x bmsTest1;
    Ltc682x bmsTest2;
    LTC_status status;
    float * cellPu;
    float * cellPd;

    uint16_t adow= 0x0228;    //!< ADSTAT base register
    uint16_t cmd[4];
    uint16_t pec;

    if(adcMode & 0x10)
    {
        if(!ltcBat->cfgAr.CFGAR0.ADCOPT)
        {
            ltcBat->cfgAr.CFGAR0.ADCOPT = 1;
            AE_ltcWrite((uint16_t*)&ltcBat->cfgAr.CFGAR0, cmdWRCFGA_pu16);
        }
    }

    adow |= (1 << 6);       // 6.th index, PUL = 1 command register index
    adow |= (adcMode & 0x0F) << 7;
    adow |= CELL_;

    cmd[0] = (adow & 0xFF00) >> 8;
    cmd[1] = (adow & 0x00FF) >> 0;

    pec = AE_pec15((uint8_t*)cmd, 2);
    cmd[2] = (pec & 0xFF00) >> 8;
    cmd[3] = (pec & 0x00FF) >> 0;

    AE_ltcWakeUpSleep();

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, 4, cmd);

    AE_LTC_CS_OFF();

    while(!AE_ltcAdcMeasureState());
    status = AE_ltcReadCellVoltage(&bmsTest1);
    if(status == LTC_WRONG_CRC) return LTC_WRONG_CRC;
    cellPu = (float*)&bmsTest1.volt.cell1;

    adow &= ~(1 << 6);       // 6.th index, PUL = 0 command register index
    cmd[0] = (adow & 0xFF00) >> 8;
    cmd[1] = (adow & 0x00FF) >> 0;

    pec = AE_pec15((uint8_t*)cmd, 2);
    cmd[2] = (pec & 0xFF00) >> 8;
    cmd[3] = (pec & 0x00FF) >> 0;

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, 4, cmd);

    AE_LTC_CS_OFF();

    while(!AE_ltcAdcMeasureState());
    status = AE_ltcReadCellVoltage(&bmsTest2);
    if(status == LTC_WRONG_CRC) return LTC_WRONG_CRC;
    cellPd = (float*)&bmsTest2.volt.cell1;

    for(i = 1; i < 16; i++) //cell 2-15
    {
        if((cellPu[i] - cellPd[i]) < -0.400)    return LTC_OPEN_WIRE;
    }

    return LTC_OK;
}

/**
 * @brief check for any open wires between the ADCs of the ltc681x
 * @param[in] bms global variable
 * @param[in] adc mode selection
 * @param[in] checked cell selection, for parameter search @refgroup CHG
 * @return if no error return OK else OPEN_WIRE
 */
LTC_status AE_ltcIsGpioOpenWire(Ltc682x * ltcBat, AdcMode adcMode, uint8_t CELL_)
{
    Ltc682x bmsTest1;
    Ltc682x bmsTest2;
    LTC_status status;
    float * gioPU;
    float * gioPd;

    uint16_t axow= 0x0410;    //!< ADSTAT base register
    uint16_t cmd[4];
    uint16_t pec;

    if(adcMode & 0x10)
    {
        if(!ltcBat->cfgAr.CFGAR0.ADCOPT)
        {
            ltcBat->cfgAr.CFGAR0.ADCOPT = 1;
            AE_ltcWrite((uint16_t*)&ltcBat->cfgAr.CFGAR0, cmdWRCFGA_pu16);
        }
    }

    axow |= (1 << 6);       // 6.th index, PUL = 1 command register index
    axow |= (adcMode & 0x0F) << 7;
    axow |= CELL_;

    cmd[0] = (axow & 0xFF00) >> 8;
    cmd[1] = (axow & 0x00FF) >> 0;

    pec = AE_pec15((uint8_t*)cmd, 2);
    cmd[2] = (pec & 0xFF00) >> 8;
    cmd[3] = (pec & 0x00FF) >> 0;

    AE_ltcWakeUpSleep();

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, 4, cmd);

    AE_LTC_CS_OFF();

    while(!AE_ltcAdcMeasureState());
    status = AE_ltcReadCellVoltage(&bmsTest1);
    if(status == LTC_WRONG_CRC) return LTC_WRONG_CRC;
    gioPU = (float*)&bmsTest1.gpio.gpio1;

    axow &= ~(1 << 6);       // 6.th index, PUL = 0 command register index
    cmd[0] = (axow & 0xFF00) >> 8;
    cmd[1] = (axow & 0x00FF) >> 0;

    pec = AE_pec15((uint8_t*)cmd, 2);
    cmd[2] = (pec & 0xFF00) >> 8;
    cmd[3] = (pec & 0x00FF) >> 0;

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, 4, cmd);

    AE_LTC_CS_OFF();

    while(!AE_ltcAdcMeasureState());
    status = AE_ltcReadCellVoltage(&bmsTest2);
    if(status == LTC_WRONG_CRC) return LTC_WRONG_CRC;
    gioPd = (float*)&bmsTest2.gpio.gpio1;

    for(i = 1; i < 9; i++) //gio 1-9
    {
        if((gioPU[i] - gioPd[i]) < -0.400)    return LTC_OPEN_WIRE;
    }

    return LTC_OK;
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







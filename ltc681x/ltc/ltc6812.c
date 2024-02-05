/**
 * @brief ltc6812.c
 * @data 19 Oca 2024
 * @author: Mehmet Dinçer
 */


#include "ltc6812.h"

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<-GLOBAL VARIABLES->>>>>>>>>>>>>>>>>>>>>>>>>>>>>
static uint16_t txBuffer[256];
static uint16_t rxBuffer[256];
volatile uint32_t uwTick;
static spiBASE_t * ltcSpi_ps;                                           // spi base address
static uint8_t slaveNumber;                                                    // bms total connection
static uint16_t i;                                                      // counter
static uint16_t dummy_u16 = 0xFF;                                              // dummy variable
static LTC_status balanceStatus = LTC_BALANCE_COMPLETED;
static spiDAT1_t spiDat_s =                                                    // spi configuration parameters
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
    init_PEC15_Table();                                                 //!< Crc start table

    hetInit();                                                          // for pwm
    _enable_interrupt_();                                               // global interrupt enable
    pwmEnableNotification(hetREG1, pwm0, pwmEND_OF_PERIOD);             // to delay microsecond and microsecond used pwm period interrupt
    pwmStart(hetRAM1, pwm0);

    ltcSpi_ps = spi;
    slaveNumber = ltcBat->batConf.numberOfSlave;

    memset(&ltcBat->cfgAr, 0, sizeof(CFGAR));
    memset(&ltcBat->cfgBr, 0, sizeof(CFGBR));

    //!< configuration register A
    ltcBat->cfgAr.CFGAR0.ADCOPT |= ltcBat->batConf.adcopt;              // adc speed mode selection
    ltcBat->cfgAr.CFGAR0.REFON |= ltcBat->batConf.refon;                // power up untill watchdog
    ltcBat->cfgAr.CFGAR0.cfg |= ltcBat->batConf.gioAPullOffPin << 2;    // GPIO1 located 1st index and Register start location
                                                                        // at 3th index so we must shift 2 bit left

    //!< configuration register B
    ltcBat->cfgBr.CFGBR0.cfg |= ltcBat->batConf.gioBPullOffPin >> 6;    // GPIO6 located 6th index and Register start location
                                                                        // at 0th index so we must shift 6 bit right

    AE_ltcWrite((uint16_t*)&ltcBat->cfgAr, cmdWRCFGA_pu16);             // write to configuration register
    AE_ltcWrite((uint16_t*)&ltcBat->cfgBr, cmdWRCFGB_pu16);             // write to configuration register
}

/**
 * @brief It fills the given values into the buffer by adding crc to the end and then sends them.
 * @param[in] transmitted datas address
 * @param[in] cmd command + 2byte crc
 * @return none
 */
void AE_ltcWrite(uint16_t * txData, uint16_t cmd[4])
{
    uint8_t bufferLen = CMD_LEN + slaveNumber * TRANSMIT_LEN;           // 4 = cmd, 8 = 6 byte data + 2 byte crc
    uint8_t j;
    uint16_t pec;

    for(i = 0; i < CMD_LEN; i++)                                        // assign command and command pec
    {
        txBuffer[i] = cmd[i];
    }

    for(i = 0; i < slaveNumber; i++)
    {
        for(j = 0; j < REGISTER_LEN; j++)
        {
            txBuffer[CMD_LEN + TRANSMIT_LEN * i + j] = txData[i * REGISTER_LEN + j];
        }

        pec = AE_pec15((uint8_t*)&txData[i * TRANSMIT_LEN], 6);
        txBuffer[CMD_LEN + TRANSMIT_LEN * i + 6] = (pec >> 8) & 0xFF;   // +6 = pec0 index
        txBuffer[CMD_LEN + TRANSMIT_LEN * i + 7] = (pec >> 0) & 0xFF;   // +7 = pec1 index
    }

    AE_ltcWakeUpSleep();

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, bufferLen, txBuffer);

    AE_LTC_CS_OFF();

    AE_delayTenUs(1);   //!< 10us delay
}

/**
 * @brief write the cmd command
 * @param[in] cmd + pec
 * @return none
 */
void AE_ltcCmdWrite(uint16_t cmd[4])
{
    AE_ltcWakeUpSleep();

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, CMD_LEN, cmd);

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
        uint16_t pec = AE_pec15((uint8_t*)&rxBuffer[RECEIVE_LEN * i], REGISTER_LEN);        // calculate the crc of the first 6 index

        uint16_t readPec = (rxBuffer[RECEIVE_LEN * i + 6] << 8) | (rxBuffer[RECEIVE_LEN * i + 7]);

        if(pec != readPec) return LTC_WRONG_CRC;

        for(j = 0; j < RECEIVE_LEN; j++)
        {
            rxData[j] = rxBuffer[RECEIVE_LEN * i + j];  //!NOTE çoklu slave için rxData[i][j] eklenecek
        }
    }

    return LTC_OK;
}

/**
 * @brief read the some specific register
 * @param[in] cmd + pec
 * @return register value
 */
uint16_t AE_ltcCmdRead(uint16_t cmd[4])
{
    uint16_t returnVal;

    AE_LTC_CS_ON();

    spiTransmitData(ltcSpi_ps, &spiDat_s, 4, cmd);
    spiReceiveData(ltcSpi_ps, &spiDat_s, 1, &returnVal);

    AE_LTC_CS_OFF();

    return returnVal;
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

    AE_delayTenUs(10);                                          //!< 100us delay, t-wake time
}

/**
 * @brief isoSPI wake up from idle state and enters the READY state
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
    AE_delayTenUs(1);                                           //!< 10us delay, t-ready time
}


/**
 * @brief write to configuration register A
 * @param[in] bms global valiable
 * @return none
 */
void AE_ltcWriteConfRegA(Ltc682x * ltcBat)
{
    AE_ltcWrite((uint16_t*)&ltcBat->cfgAr, cmdWRCFGA_pu16);             // write to configuration register
}

/**
 * @brief write to configuration register B
 * @param[in] bms global variable
 * @return none
 */
void AE_ltcWriteConfRegB(Ltc682x * ltcBat)
{
    AE_ltcWrite((uint16_t*)&ltcBat->cfgBr, cmdWRCFGB_pu16);             // write to configuration register
}

/**
 * @brief read the configuration register A
 * @param[in] bms global variable
 * @return none
 */
LTC_status AE_ltcReadConfRegA(Ltc682x * ltcBat)
{
    return AE_ltcRead((uint16_t*)&ltcBat->cfgAr.CFGAR0.cfg, cmdRDCFGA_pu16);
}

/**
 * @brief read the configuration register B
 * @param[in] bms global variable
 * @return none
 */
LTC_status AE_ltcReadConfRegB(Ltc682x * ltcBat)
{
    return AE_ltcRead((uint16_t*)&ltcBat->cfgBr.CFGBR0.cfg, cmdRDCFGB_pu16);
}

/**
 * @brief Read the cell voltage
 * @param[in] ltcBat global variable
 * @return if crc is suitable with message retur ok else crcError
 */
LTC_status AE_ltcReadCellVoltage(Ltc682x * ltcBat)
{
    LTC_status status;
    float * fptr = (float*)&ltcBat->volt.cell1;                 // To access each index separately

    //!< voltage register Group A
    status = AE_ltcRead(rxBuffer, cmdRDCVA_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < 3; i++)
    {   //offset = 0
        fptr[i] = ((rxBuffer[i*2] << 0) | (rxBuffer[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
    }

    //!< voltage register Group B
    status = AE_ltcRead(rxBuffer, cmdRDCVB_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < 3; i++)
    {   //offset = 3
        fptr[i + 3] = ((rxBuffer[i*2] << 0) | (rxBuffer[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
    }

    //!< voltage register Group C
    status = AE_ltcRead(rxBuffer, cmdRDCVC_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < 3; i++)
    {   //offset = 6
        fptr[i + 6] = ((rxBuffer[i*2] << 0) | (rxBuffer[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
    }

    //!< voltage register Group D
    status = AE_ltcRead(rxBuffer, cmdRDCVD_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < 3; i++)
    {   //offset = 9
        fptr[i + 9] = ((rxBuffer[i*2] << 0) | (rxBuffer[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
    }

    //!< voltage register Group E
    status = AE_ltcRead(rxBuffer, cmdRDCVE_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < 3; i++)
    {   //offset = 12
        fptr[i + 12] = ((rxBuffer[i*2] << 0) | (rxBuffer[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
    }

    return status;
}

/**
 * @brief configure under and overvoltage values
 * @param[in] bms global variable
 * @param[in] undervoltage value
 * @param[in] overvoltage value
 * @return none
 */
void AE_ltcSetUnderOverVoltage(Ltc682x * ltcBat, float underVolt, float overVolt)
{
    uint16_t underVoltTemp = underVolt * 625.0 - 1;
    uint16_t overVoltTemp = overVolt * 625;

    // clear voltage set register
    ltcBat->cfgAr.CFGAR1.cfg = 0;
    ltcBat->cfgAr.CFGAR2.cfg = 0;
    ltcBat->cfgAr.CFGAR3.cfg = 0;

    // configure the voltage
    ltcBat->cfgAr.CFGAR0.DTEN = 1;
    ltcBat->cfgAr.CFGAR0.REFON = 1;

    ltcBat->cfgAr.CFGAR1.cfg |= underVoltTemp & 0x00FF;
    ltcBat->cfgAr.CFGAR2.cfg |= (underVoltTemp >> 8) & 0x000F;

    ltcBat->cfgAr.CFGAR2.cfg |= (overVoltTemp << 4) & 0x00F0;
    ltcBat->cfgAr.CFGAR3.cfg |= (overVoltTemp >> 4) & 0x00FF;

    AE_ltcWrite((uint16_t*)&ltcBat->cfgAr, cmdWRCFGA_pu16);
}

LTC_status AE_ltcUnderOverFlag(Ltc682x * ltcBat)
{
    LTC_status status;

    // Although all other functions work in a single call, Status register group B does not work
    // To work this command cell read 3 times, I find this by trying
    for(i = 0; i < 3; i++)
    {
        AE_ltcStartCellAdc(ltcBat, MODE_422HZ, true, CELL_ALL);
        //!< check adcMeasure duration is completed
        while(!AE_ltcAdcMeasureState());
        status = AE_ltcReadCellVoltage(ltcBat);

        if(status == LTC_WRONG_CRC) return LTC_WRONG_CRC;
    }

    //read status register B, flags 1-12 located here
    AE_ltcStartStatusAdc(ltcBat, MODE_7KHZ, CHST_ALL);
    while(!AE_ltcAdcMeasureState());
    status = AE_ltcReadStatusRegB(ltcBat);

    if(status == LTC_WRONG_CRC) return LTC_WRONG_CRC;

    //read auxilary register D , flags 13-15 located here
    AE_ltcStartGpioAdc(ltcBat, MODE_7KHZ, GPIO_ALL);
    while(!AE_ltcAdcMeasureState());
    status = AE_ltcRead(rxBuffer, cmdRDAUXD_pu16);
    if(status == LTC_WRONG_CRC) return LTC_WRONG_CRC;

    //assign flags that come from auxilary groupd flags to status register B structure
    ltcBat->statusRegB.CellUnderFlag.cell13 = (rxBuffer[4] >> 0) & 0x01;
    ltcBat->statusRegB.CellUnderFlag.cell14 = (rxBuffer[4] >> 2) & 0x01;
    ltcBat->statusRegB.CellUnderFlag.cell15 = (rxBuffer[4] >> 4) & 0x01;

    ltcBat->statusRegB.CellOverFlag.cell13 = (rxBuffer[4] >> 1) & 0x01;
    ltcBat->statusRegB.CellOverFlag.cell14 = (rxBuffer[4] >> 3) & 0x01;
    ltcBat->statusRegB.CellOverFlag.cell15 = (rxBuffer[4] >> 5) & 0x01;

    return LTC_OK;
}

/**
 * @brief Start Cell Voltage ADC Conversion and Poll Status
 * @param[in] ltcBat global variable
 * @param[in] 0 Discharge Not Permitted 1 Discharge Permitted
 * @param[in] cell selection for adc conversion for parameter search @refgroup CH
 * @return if crc is suitable with message retur ok else crcError
 */
void AE_ltcStartCellAdc(Ltc682x * ltcBat, AdcMode adcMode, uint8_t dischargePermit, uint8_t CELL_)
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
    adcvReg |= CELL_;

    uint16_t cmd[4] = {0};
    uint16_t pec;

    cmd[0] = (adcvReg >> 8) & 0x00FF;
    cmd[1] = (adcvReg >> 0) & 0x00FF;
    pec = AE_pec15((uint8_t*)cmd, 2);
    cmd[2] = (pec >> 8) & 0x00FF;
    cmd[3] = (pec >> 0) & 0x00FF;

    AE_ltcCmdWrite(cmd);
}

/**
 * @brief after starting to cell adc measure, we can control process is finish or not
 * @return if measure is completed return 1 else 0
 */
uint8_t AE_ltcAdcMeasureState()
{
    uint8_t status = (uint8_t)AE_ltcCmdRead(cmdPladc_pu16);
    return status;
}

/**
 * @brief Start the adc conversion for the GPIO pins
 * @param[in] bms global variable
 * @param[in] adc speed selection
 * @param[in] gio pin that want to read, to parameter search
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

    AE_ltcCmdWrite(cmd);
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

    AE_ltcCmdWrite(cmd);
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

    memset((void*)&ltcBat->statusRegB.digitalPowerSupplyVolt, 0, sizeof(StatusRegB));

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
LTC_status AE_ltcClearCellAdc(Ltc682x * ltcBat)
{
    Ltc682x ltcTemp;
    LTC_status status;

    AE_ltcCmdWrite(cmdCLRCELL_pu16);

    status = AE_ltcReadCellVoltage(&ltcTemp);
    if(status == LTC_WRONG_CRC) return status;

    if(ltcTemp.volt.cell1 > 6.5)                                    //!< if adc conversion is close read the pin 0xFF but this variable is
    {
        memset((void*)&ltcBat->volt.cell1, 0, sizeof(CellVolt));    // float, so value > 6.5 condition is consistent

        return LTC_OK;
    }
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

    AE_ltcCmdWrite(cmdCLRAUX_pu16);

    status = AE_ltcReadGPIOVoltage(ltcBat);
    if(status == LTC_WRONG_CRC) return status;

    if(ltcBat->gpio.gpio1 > 6.5)        //!< //!< if adc conversion is close read the pin 0xFF but this variable is
    {                                   // float, so value > 6.5 condition is consistent
        memset((void*)&ltcBat->gpio.gpio1, 0, sizeof(Gpio));
        return LTC_OK;
    }
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

    AE_ltcCmdWrite(cmdCLRSTAT_pu16);

    status = AE_ltcReadStatusRegA(ltcBat);
    if(status == LTC_WRONG_CRC) return status;

    if(ltcBat->gpio.gpio1 > 6.5)        //!< //!< if adc conversion is close read the pin 0xFF but this variable is
    {                                   // float, so value > 6.5 condition is consistent
        memset((void*)&ltcBat->statusRegA.sumOfCell, 0, sizeof(StatusRegA));
        return LTC_OK;
    }
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
    uint16_t pwmDuty[12] = {0};         //!< pwm blocks valid on PWM Register Group and PWM/S Control Register Group B
                                        //!< so, send 12 bytes data
    uint8_t mode = 0;
    uint8_t index = 0;

    for(i = 0; i < 15; i++)             //!< 15 == S pin number
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
 * @brief if pwm is started pause the pw
 * @param[in] bms global variable
 * @return none
 */
void AE_ltcPausePwm(Ltc682x * ltcBat)
{
    AE_ltcCmdWrite(cmdMute_pu16);
}

/*
 * @brief if pwm is paused, restart pwm
 * @param[in] bms global variable
 * @return none
 */
void AE_ltcContinuePwm(Ltc682x * ltcBat)
{
    AE_ltcCmdWrite(cmdUnMute_pu16);
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
void AE_ltcPreBalance(Ltc682x * ltcBat, DischargeTime DIS_, float underVolt, float overVolt, uint16_t DCC_)
{
    uint16_t maskedDCC;

    ltcBat->cfgBr.CFGBR1.DTMEN |= 1;            //The LTC6812-1 has the ability to periodically monitor
                                                //cell voltages while the discharge timer is active. The host
                                                //should write the DTMEN bit in Configuration Register
                                                //Group B to 1 to enable this feature.

    // clear related bits
    ltcBat->cfgAr.CFGAR4.cfg &= 0x00;
    ltcBat->cfgAr.CFGAR5.cfg &= 0x00;
    ltcBat->cfgBr.CFGBR0.cfg &= 0x8F;
    ltcBat->cfgBr.CFGBR1.cfg &= 0xFB;

    //DCC pins are scattered in several registers so we must mask them
    //Register Group A, CFGRA4 Mask = pin1-8    0x1FE
    maskedDCC = DCC_ & 0x1FE;
    maskedDCC >>= 1;           //pin1 start index 1 and dcc1 located 0th index in register
    ltcBat->cfgAr.CFGAR4.cfg |= maskedDCC;

    //Register Group A, CFGRA5 Mask = pin9-12    0x1E00
    maskedDCC = DCC_ & 0x1E00;
    maskedDCC >>= 9;            //pin9 start index 9 and dcc9 located 0th index in register
    ltcBat->cfgAr.CFGAR5.cfg |= maskedDCC;

    //set discharge duration
    ltcBat->cfgAr.CFGAR5.cfg |= (DIS_ << 4) & 0x00F0;

    //Register Group B, CFGBR0 Mask = pin13-15    0xE000
    maskedDCC = DCC_ & 0xE000;
    maskedDCC >>= 9;            //pin13 start index 13 and dcc13 located 4th index in register (13 - 4)
    ltcBat->cfgBr.CFGBR0.cfg |= maskedDCC;

    //Register Group B, CFGBR1 Mask = pin0    0xE000
    maskedDCC = DCC_ & 0x1;
    maskedDCC <<= 2;            //pin0 start index 0 and dcc0 located 2th index in register (0 - 2)
    ltcBat->cfgBr.CFGBR1.cfg |= maskedDCC;

    //!< enable discharge monitoring and set under and over voltage
    AE_ltcSetUnderOverVoltage(ltcBat, underVolt, overVolt);

    AE_ltcWrite((uint16_t*)&ltcBat->cfgAr, cmdWRCFGA_pu16);
    AE_ltcWrite((uint16_t*)&ltcBat->cfgBr, cmdWRCFGB_pu16);

    balanceStatus = LTC_IN_BALANCE;
}

/**
 * @brief balance the cell in polling mode by checking the under and overvoltage flags
 * @param[in] bms global variable
 * @param[in] slave's min cell voltage
 * @return none
 */
void AE_ltcBalance(Ltc682x * ltcBat, float minVoltage)
{
    uint16_t maskedDCC;

//    minVoltage += 0.003f;       //!< optimization

    //clear the configuration register-A
    memset((void*)&ltcBat->cfgAr.CFGAR0.cfg, 0, sizeof(CFGAR));

    //enable REFON bit
    ltcBat->cfgAr.CFGAR0.REFON |= 1;

    //clear the DCC13-15, DCC0 and discharge timer enable
    ltcBat->cfgBr.CFGBR0.cfg &= 0x8F;
    ltcBat->cfgBr.CFGBR1.DTMEN = 0;
    ltcBat->cfgBr.CFGBR1.DCC0 = 0;

    AE_ltcWrite((uint16_t*)&ltcBat->cfgAr, cmdWRCFGA_pu16);
    AE_ltcWrite((uint16_t*)&ltcBat->cfgBr, cmdWRCFGB_pu16);

    // set the under and overvoltage limit
    AE_ltcSetUnderOverVoltage(ltcBat, 3.0f, minVoltage);

    // take the under and overvoltage flags
    AE_ltcUnderOverFlag(ltcBat);

    maskedDCC = ltcBat->statusRegB.CellOverFlag.flag;

    //<<<<<<<<<<<<<<<-Over Voltage->>>>>>>>>>>>>>>
    //DCC pins are scattered in several registers so we must mask them
    ltcBat->cfgAr.CFGAR4.cfg |= maskedDCC & 0xFF;

    //Register Group A, CFGRA5 Mask = pin9-12    0x1E00
    ltcBat->cfgAr.CFGAR5.cfg |= (maskedDCC >> 8) & 0x0F;

    //Register Group B, CFGBR0 Mask = pin13-15    0xE000
    ltcBat->cfgBr.CFGBR0.cfg |= (maskedDCC >> 8) & 0xF0;

    ltcBat->cfgAr.CFGAR5.DCTO = 1;

    AE_ltcWrite((uint16_t*)&ltcBat->cfgAr, cmdWRCFGA_pu16);
    AE_ltcWrite((uint16_t*)&ltcBat->cfgBr, cmdWRCFGB_pu16);
}

/**
 * @brief check the balance status
 */
LTC_status AE_ltcIsBalanceComplete(Ltc682x * ltcBat)
{
    LTC_status status;
    uint16_t dccVal = 0;

    if(balanceStatus == LTC_IN_BALANCE)
    {
        status = AE_ltcRead((uint16_t*)&ltcBat->cfgAr.CFGAR0.cfg, cmdRDCFGA_pu16);
        status = AE_ltcRead((uint16_t*)&ltcBat->cfgBr.CFGBR0.cfg, cmdRDCFGB_pu16);

        if(status == LTC_WRONG_CRC) return LTC_WRONG_CRC;

        dccVal |= (ltcBat->cfgAr.CFGAR4.cfg & 0xFF) << 1;   // 1 - 0 (1 = DCCx's x, 0 = register start index)
        dccVal |= (ltcBat->cfgAr.CFGAR5.cfg & 0x0F) << 9;   // 9 - 0
        dccVal |= (ltcBat->cfgBr.CFGBR0.cfg & 0x70) << 9;   // 13 - 4 (look at datasheet 4 comes register start index)
        dccVal |= (ltcBat->cfgBr.CFGBR1.cfg & 0x04) >> 2;   // 0 - 2

        //when balance is completed dccVal will be 0 and
        if(!dccVal)
        {
            return LTC_BALANCE_COMPLETED;
        }
        else
        {
            return LTC_IN_BALANCE;
        }
    }
    else
    {
        return LTC_BALANCE_COMPLETED;
    }
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
 * @param[in] checked cell selection, for parameter search @refgroup CH
 * @return if no error return OK else OPEN_WIRE
 */
LTC_status AE_ltcIsCellOpenWire(Ltc682x * ltcBat, AdcMode adcMode, uint8_t CELL_)
{
    Ltc682x bmsTest1;
    Ltc682x bmsTest2;
    LTC_status status;
    float * cellPu;
    float * cellPd;

    uint16_t adow= 0x0228;    //!< adow base register
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

    AE_ltcClearCellAdc(ltcBat);

    AE_ltcCmdRead(cmd);
    AE_ltcCmdRead(cmd);


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

    AE_ltcClearCellAdc(ltcBat);

    AE_ltcCmdRead(cmd);
    AE_ltcCmdRead(cmd);

    while(!AE_ltcAdcMeasureState());
    status = AE_ltcReadCellVoltage(&bmsTest2);
    if(status == LTC_WRONG_CRC) return LTC_WRONG_CRC;
    cellPd = (float*)&bmsTest2.volt.cell1;

    for(i = 1; i < ltcBat->batConf.numberOfSerialCell; i++) //cell 2-15
    {   // -0.400 comes from data sheet
        if((cellPu[i] - cellPd[i]) < -0.400)    return LTC_OPEN_WIRE;
    }

    return LTC_OK;
}

/**
 * @brief check for any open wires between the ADCs of the ltc681x
 * @param[in] bms global variable
 * @param[in] adc mode selection
 * @param[in] checked cell selection, for parameter search @refgroup CH
 * @return if no error return OK else OPEN_WIRE
 */
LTC_status AE_ltcIsGpioOpenWire(Ltc682x * ltcBat, AdcMode adcMode, uint8_t CELL_)
{
    Ltc682x bmsTest1;
    Ltc682x bmsTest2;
    LTC_status status;
    float * gioPU;
    float * gioPd;

    uint16_t axow= 0x0410;    //!< axow base register
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
 * @brief calculate the GPIO3 NTC temperature
 * @param[in] bms global variable
 * @return temperature in celcius
 */
double AE_ltcTemperature(Ltc682x * ltcBat)
{
    AE_ltcStartGpioAdc(ltcBat, MODE_7KHZ, GPIO_ALL);
    while(!AE_ltcAdcMeasureState());
    AE_ltcReadGPIOVoltage(ltcBat);

    return AE_calculateTemp(ltcBat->gpio.gpio3, ltcBat->gpio.ref, PULL_DOWN);
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







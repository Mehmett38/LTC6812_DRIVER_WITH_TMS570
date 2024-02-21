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
spiBASE_t * ltcSpi_ps;                                                  // spi base address
static uint8_t slaveNumber;                                             // bms total connection
static int16_t i;                                                       // counter
static uint16_t dummy_u16 = 0xFF;                                       // dummy variable
static LTC_status balanceStatus = LTC_BALANCE_COMPLETED;
static spiDAT1_t spiDat_s =                                             // spi configuration parameters
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
    slaveNumber = ltcBat[0].batConf.numberOfSlave;

    for(i = 0; i < slaveNumber; i++)
    {
        memset(&ltcBat[i].cfgAr, 0, sizeof(CFGAR));
        memset(&ltcBat[i].cfgBr, 0, sizeof(CFGBR));

        //!< configuration register A
        ltcBat[i].cfgAr.CFGAR0.ADCOPT |= ltcBat[i].batConf.adcopt;              // adc speed mode selection
        ltcBat[i].cfgAr.CFGAR0.REFON |= ltcBat[i].batConf.refon;                // power up untill watchdog
        ltcBat[i].cfgAr.CFGAR0.cfg |= ltcBat[i].batConf.gioAPullOffPin << 2;    // GPIO1 located 1st index and Register start location
                                                                            // at 3th index so we must shift 2 bit left
        //!< configuration register B
        ltcBat[i].cfgBr.CFGBR0.cfg |= ltcBat[i].batConf.gioBPullOffPin >> 6;    // GPIO6 located 6th index and Register start location
                                                                            // at 0th index so we must shift 6 bit right
    }

    AE_ltcWrite((uint16_t*)&ltcBat[0].cfgAr, cmdWRCFGA_pu16);             // write to configuration register
    AE_ltcWrite((uint16_t*)&ltcBat[0].cfgBr, cmdWRCFGB_pu16);             // write to configuration register
}

/**
 * @brief It fills the given values into the buffer by adding crc to the end and then sends them.
 * @param[in] transmitted datas address
 * @param[in] cmd command + 2byte crc
 * @return none
 */
void AE_ltcWrite(uint16_t * txData, uint16_t cmd[4])
{
    uint8_t bufferLen = slaveNumber * TRANSMIT_LEN + CMD_LEN;           // 4 = cmd, 8 = 6 byte data + 2 byte crc
    uint8_t j;
    uint16_t pec;

    for(j = 0; j < CMD_LEN; j++)                                    // assign command pec
    {
        txBuffer[j] = cmd[j];
    }

    for(i = (slaveNumber - 1); i >= 0; i--)
    {
        for(j = 0; j < REGISTER_LEN; j++)
        {   /*When the slaveNumber changes, slave offset must be added to the txData pointer
            to ensure that it is transferred to the other slave.*/
            txBuffer[CMD_LEN + TRANSMIT_LEN * (slaveNumber - i - 1) + j] = txData[(i * sizeof(Ltc682x) / sizeof(uint16_t)) + j];
        }

        pec = AE_pec15((uint8_t*)& txData[(i * sizeof(Ltc682x) / sizeof(uint16_t))], 6);
        txBuffer[CMD_LEN + TRANSMIT_LEN * (slaveNumber - i - 1) + 6] = (pec >> 8) & 0xFF;   // +6 = pec0 index
        txBuffer[CMD_LEN + TRANSMIT_LEN * (slaveNumber - i - 1) + 7] = (pec >> 0) & 0xFF;   // +7 = pec1 index
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


    spiTransmitData(ltcSpi_ps, &spiDat_s, 4, cmd);
    spiReceiveData(ltcSpi_ps, &spiDat_s, (slaveNumber * RECEIVE_LEN), rxBuffer);

    AE_LTC_CS_OFF();

    uint8_t j;
    for(i = 0; i < slaveNumber; i++)
    {
        uint16_t pec = AE_pec15((uint8_t*)&rxBuffer[RECEIVE_LEN * i], REGISTER_LEN);        // calculate the crc of the first 6 index

        uint16_t readPec = (rxBuffer[RECEIVE_LEN * i + 6] << 8) | (rxBuffer[RECEIVE_LEN * i + 7]);

        if(pec != readPec) return LTC_WRONG_CRC;

        for(j = 0; j < RECEIVE_LEN; j++)
        {
            rxData[j + i * RECEIVE_LEN] = rxBuffer[RECEIVE_LEN * i + j];  //!NOTE çoklu slave için rxData[i][j] eklenecek
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
    return AE_ltcRead((uint16_t*)&ltcBat[0].cfgAr.CFGAR0.cfg, cmdRDCFGA_pu16);
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
    float * fLtcPtr = NULL;                 // To access each index separately
    uint16_t * fRxPtr = NULL;                  // to access the rx buffer's 8k index
    uint8_t j;                              // counter

    for(j = 0; j < slaveNumber; j++)
    {
        //!< voltage register Group A
        status = AE_ltcRead(rxBuffer, cmdRDCVA_pu16);
        if(status == LTC_WRONG_CRC) return status;

        fLtcPtr = &ltcBat[j].volt.cell1;    // to access the voltage register index by index
        fRxPtr = &rxBuffer[j * RECEIVE_LEN];

        for(i = 0; i < 3; i++)
        {   //offset = 0
            fLtcPtr[i] = ((fRxPtr[i*2] << 0) | (fRxPtr[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
        }

        //!< voltage register Group B
        status = AE_ltcRead(rxBuffer, cmdRDCVB_pu16);
        if(status == LTC_WRONG_CRC) return status;
        for(i = 0; i < 3; i++)
        {   //offset = 3
            fLtcPtr[i + 3] = ((fRxPtr[i*2] << 0) | (fRxPtr[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
        }

        //!< voltage register Group C
        status = AE_ltcRead(rxBuffer, cmdRDCVC_pu16);
        if(status == LTC_WRONG_CRC) return status;
        for(i = 0; i < 3; i++)
        {   //offset = 6
            fLtcPtr[i + 6] = ((fRxPtr[i*2] << 0) | (fRxPtr[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
        }

        //!< voltage register Group D
        status = AE_ltcRead(rxBuffer, cmdRDCVD_pu16);
        if(status == LTC_WRONG_CRC) return status;
        for(i = 0; i < 3; i++)
        {   //offset = 9
            fLtcPtr[i + 9] = ((fRxPtr[i*2] << 0) | (fRxPtr[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
        }

        //!< voltage register Group E
        status = AE_ltcRead(rxBuffer, cmdRDCVE_pu16);
        if(status == LTC_WRONG_CRC) return status;
        for(i = 0; i < 3; i++)
        {   //offset = 12
            fLtcPtr[i + 12] = ((fRxPtr[i*2] << 0) | (fRxPtr[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
        }
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
void AE_ltcSetUnderOverVoltage(Ltc682x * ltcBat, float * underVolt, float * overVolt)
{
    float underVoltTemp;
    float overVoltTemp;

    for(i = 0; i < slaveNumber; i++)
    {
        ltcBat[i].cfgAr.CFGAR0.REFON = 1;
    }
    AE_ltcWrite((uint16_t*)&ltcBat[0].cfgAr, cmdWRCFGA_pu16);

    for(i = 0; i < slaveNumber; i++)
    {
        underVoltTemp = underVolt[i] * 625.0 - 1;
        overVoltTemp = overVolt[i] * 625;

        // clear voltage set register
        ltcBat[i].cfgAr.CFGAR1.cfg = 0;
        ltcBat[i].cfgAr.CFGAR2.cfg = 0;
        ltcBat[i].cfgAr.CFGAR3.cfg = 0;

        // configure the voltage
        ltcBat[i].cfgAr.CFGAR0.REFON = 1;

        ltcBat[i].cfgAr.CFGAR1.cfg |= (uint16_t)underVoltTemp & 0x00FF;
        ltcBat[i].cfgAr.CFGAR2.cfg |= ((uint16_t)underVoltTemp >> 8) & 0x000F;

        ltcBat[i].cfgAr.CFGAR2.cfg |= ((uint16_t)overVoltTemp << 4) & 0x00F0;
        ltcBat[i].cfgAr.CFGAR3.cfg |= ((uint16_t)overVoltTemp >> 4) & 0x00FF;
    }

    AE_ltcWrite((uint16_t*)&ltcBat[0].cfgAr, cmdWRCFGA_pu16);
}

LTC_status AE_ltcUnderOverFlag(Ltc682x * ltcBat)
{
    LTC_status status;

    // Although all other functions work in a single call, Status register group B does not work
    // To work this command cell read 3 times, I find this by trying

    AE_ltcStartCellAdc(ltcBat, MODE_7KHZ, true, CELL_ALL);
    //!< check adcMeasure duration is completed
    while(!AE_ltcAdcMeasureState());
    status = AE_ltcReadCellVoltage(ltcBat);
    if(status == LTC_WRONG_CRC) return LTC_WRONG_CRC;

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

    for(i = 0; i < slaveNumber; i++)
    {
        //assign flags that come from auxilary groupd flags to status register B structure
        ltcBat[i].statusRegB.CellUnderFlag.cell13 = (rxBuffer[4 + i * RECEIVE_LEN] >> 0) & 0x01;
        ltcBat[i].statusRegB.CellUnderFlag.cell14 = (rxBuffer[4 + i * RECEIVE_LEN] >> 2) & 0x01;
        ltcBat[i].statusRegB.CellUnderFlag.cell15 = (rxBuffer[4 + i * RECEIVE_LEN] >> 4) & 0x01;

        ltcBat[i].statusRegB.CellOverFlag.cell13 = (rxBuffer[4 + i * RECEIVE_LEN] >> 1) & 0x01;
        ltcBat[i].statusRegB.CellOverFlag.cell14 = (rxBuffer[4 + i * RECEIVE_LEN] >> 3) & 0x01;
        ltcBat[i].statusRegB.CellOverFlag.cell15 = (rxBuffer[4 + i * RECEIVE_LEN] >> 5) & 0x01;
    }

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
        for(i = 0; i < slaveNumber; i++)
        {
            if(!ltcBat[i].cfgAr.CFGAR0.ADCOPT)
            {
                ltcBat[i].cfgAr.CFGAR0.ADCOPT = 1;
            }
        }
        AE_ltcWrite((uint16_t*)&ltcBat[0].cfgAr, cmdWRCFGA_pu16);
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
        for(i = 0; i < slaveNumber; i++)
        {
            if(!ltcBat[i].cfgAr.CFGAR0.ADCOPT)
            {
                ltcBat[i].cfgAr.CFGAR0.ADCOPT = 1;
            }
        }
        AE_ltcWrite((uint16_t*)&ltcBat[0].cfgAr, cmdWRCFGA_pu16);
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
LTC_status AE_ltcReadGpioVoltage(Ltc682x * ltcBat)
{
    LTC_status status;
    float * fLtcPtr = NULL;                 // To access each index separately
    uint16_t * fRxPtr = NULL;               // to access the rx buffer's 8k index
    uint8_t j;                              // counter

    for(j = 0; j < slaveNumber; j++)
    {
        //!< voltage register Group A
        status = AE_ltcRead(rxBuffer, cmdRDAUXA_pu16);
        if(status == LTC_WRONG_CRC) return status;

        fLtcPtr = &ltcBat[j].gpio.gpio1;    // to access the voltage register index by index
        fRxPtr = &rxBuffer[j * RECEIVE_LEN];

        for(i = 0; i < 3; i++)
        {   //offset + 12
            fLtcPtr[i] = ((fRxPtr[i*2] << 0) | (fRxPtr[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
        }

        //!< voltage register Group B
        status = AE_ltcRead(rxBuffer, cmdRDAUXB_pu16);
        if(status == LTC_WRONG_CRC) return status;

        for(i = 0; i < 3; i++)
        {   //offset + 12
            fLtcPtr[i + 3] = ((fRxPtr[i*2] << 0) | (fRxPtr[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
        }

        //!< voltage register Group C
        status = AE_ltcRead(rxBuffer, cmdRDAUXC_pu16);
        if(status == LTC_WRONG_CRC) return status;

        for(i = 0; i < 3; i++)
        {   //offset + 12
            fLtcPtr[i + 6] = ((fRxPtr[i*2] << 0) | (fRxPtr[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
        }

        //!< voltage register Group D
        status = AE_ltcRead(rxBuffer, cmdRDAUXD_pu16);
        if(status == LTC_WRONG_CRC) return status;

        for(i = 0; i < 1; i++)
        {   //offset + 12
            fLtcPtr[i + 9] = ((fRxPtr[i*2] << 0) | (fRxPtr[i*2 + 1] << 8)) / 10000.0;  //!< 10000 comes from datasheet
        }
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
        for(i = 0; i < slaveNumber; i++)
        {
            if(!ltcBat[i].cfgAr.CFGAR0.ADCOPT)
            {
                ltcBat[i].cfgAr.CFGAR0.ADCOPT = 1;
            }
        }
        AE_ltcWrite((uint16_t*)&ltcBat[0].cfgAr, cmdWRCFGA_pu16);
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

    float * fLtcPtr = NULL;                 // To access each index separately
    uint16_t * fRxPtr = NULL;                  // to access the rx buffer's 8k index

    status = AE_ltcRead(rxBuffer, cmdRDSTATA_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < slaveNumber; i++)
    {
        fLtcPtr = (float *)&ltcBat[i].statusRegA.sumOfCell;
        fRxPtr = &rxBuffer[i * RECEIVE_LEN];

        fLtcPtr[0] = fRxPtr[0] | fRxPtr[1] << 8;
        fLtcPtr[0] = fLtcPtr[0] / 10000.0 * 30.0;                 //Cells Voltage = SC • 100μV • 30

        fLtcPtr[1] = fRxPtr[2] | fRxPtr[3] << 8;
        fLtcPtr[1] = fLtcPtr[1] * 100.0 / 7.6 /1000.0 - 276;      //ITMP • 100μV/7.6mV/C – 276C

        fLtcPtr[2] = fRxPtr[4] | fRxPtr[5] << 8;
        fLtcPtr[2] = fLtcPtr[2] / 10000.0;                        //Analog Power Supply Voltage = VA • 100μV
    }

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

    uint16_t * fRxPtr = NULL;                  // to access the rx buffer's 8k index

    status = AE_ltcRead(rxBuffer, cmdRDSTATB_pu16);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < slaveNumber; i++)
    {
        fRxPtr = (uint16_t*)&rxBuffer[i * RECEIVE_LEN];

        memset((void*)&ltcBat[i].statusRegB.digitalPowerSupplyVolt, 0, sizeof(StatusRegB));

        ltcBat[i].statusRegB.digitalPowerSupplyVolt = (fRxPtr[0] << 8 | fRxPtr[1])/10000.0;

        ltcBat[i].statusRegB.CellUnderFlag.cell1 = ((fRxPtr[2] >> 0 ) & 0x01);
        ltcBat[i].statusRegB.CellOverFlag.cell1  = ((fRxPtr[2] >> 1 ) & 0x01);
        ltcBat[i].statusRegB.CellUnderFlag.cell2 = ((fRxPtr[2] >> 2 ) & 0x01);
        ltcBat[i].statusRegB.CellOverFlag.cell2  = ((fRxPtr[2] >> 3 ) & 0x01);
        ltcBat[i].statusRegB.CellUnderFlag.cell3 = ((fRxPtr[2] >> 4 ) & 0x01);
        ltcBat[i].statusRegB.CellOverFlag.cell3  = ((fRxPtr[2] >> 5 ) & 0x01);
        ltcBat[i].statusRegB.CellUnderFlag.cell4 = ((fRxPtr[2] >> 6 ) & 0x01);
        ltcBat[i].statusRegB.CellOverFlag.cell4  = ((fRxPtr[2] >> 7 ) & 0x01);

        ltcBat[i].statusRegB.CellUnderFlag.cell5 = ((fRxPtr[3] >> 0 ) & 0x01);
        ltcBat[i].statusRegB.CellOverFlag.cell5  = ((fRxPtr[3] >> 1 ) & 0x01);
        ltcBat[i].statusRegB.CellUnderFlag.cell6 = ((fRxPtr[3] >> 2 ) & 0x01);
        ltcBat[i].statusRegB.CellOverFlag.cell6  = ((fRxPtr[3] >> 3 ) & 0x01);
        ltcBat[i].statusRegB.CellUnderFlag.cell7 = ((fRxPtr[3] >> 4 ) & 0x01);
        ltcBat[i].statusRegB.CellOverFlag.cell7  = ((fRxPtr[3] >> 5 ) & 0x01);
        ltcBat[i].statusRegB.CellUnderFlag.cell8 = ((fRxPtr[3] >> 6 ) & 0x01);
        ltcBat[i].statusRegB.CellOverFlag.cell8  = ((fRxPtr[3] >> 7 ) & 0x01);

        ltcBat[i].statusRegB.CellUnderFlag.cell9  = ((fRxPtr[4] >> 0 ) & 0x01);
        ltcBat[i].statusRegB.CellOverFlag.cell9   = ((fRxPtr[4] >> 1 ) & 0x01);
        ltcBat[i].statusRegB.CellUnderFlag.cell10 = ((fRxPtr[4] >> 2 ) & 0x01);
        ltcBat[i].statusRegB.CellOverFlag.cell10  = ((fRxPtr[4] >> 3 ) & 0x01);
        ltcBat[i].statusRegB.CellUnderFlag.cell11 = ((fRxPtr[4] >> 4 ) & 0x01);
        ltcBat[i].statusRegB.CellOverFlag.cell11  = ((fRxPtr[4] >> 5 ) & 0x01);
        ltcBat[i].statusRegB.CellUnderFlag.cell12 = ((fRxPtr[4] >> 6 ) & 0x01);
        ltcBat[i].statusRegB.CellOverFlag.cell12  = ((fRxPtr[4] >> 7 ) & 0x01);

        ltcBat[i].statusRegB.thsd = fRxPtr[5] & 0x01;
        ltcBat[i].statusRegB.muxFail = (fRxPtr[5] >> 1) & 0x01;

        ltcBat[i].statusRegB.revionCode = (fRxPtr[5] >> 5) & 0x0F;
    }

    return status;
}

/**
 * @brief stop the cell adc conversion
 * @param[in] bms global variable
 * @return if packet is true ok else crc error
 */
LTC_status AE_ltcClearCellAdc(Ltc682x * ltcBat)
{
    LTC_status status;

    AE_ltcCmdWrite(cmdCLRCELL_pu16);

    status = AE_ltcReadCellVoltage(ltcBat);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < slaveNumber; i++)
    {
        if(ltcBat[i].volt.cell1 > 6.5f)                                    //!< if adc conversion is closed, read the pin 0xFF but this variable is
        {                                                               // float, so value > 6.5 condition is consistent
            memset((void*)&ltcBat[i].volt.cell1, 0, sizeof(CellVolt));
        }
        else
            return LTC_WRONG_CRC;
    }


    return LTC_OK;
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

    status = AE_ltcReadGpioVoltage(ltcBat);
    if(status == LTC_WRONG_CRC) return status;

    for(i = 0; i < slaveNumber; i++)
    {
        if(ltcBat[i].gpio.gpio1 > 6.5f)        //!< //!< if adc conversion is close read the pin 0xFF but this variable is
        {                                   // float, so value > 6.5 condition is consistent
            memset((void*)&ltcBat[i].gpio.gpio1, 0, sizeof(Gpio));
        }
        else
            return LTC_WRONG_CRC;
    }

    return LTC_OK;
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

    for(i = 0; i < slaveNumber; i++)
    {
        for(i = 0; i < slaveNumber; i++)
        {
            if(ltcBat[i].statusRegA.analogPowerSupplyVolt > 6.5f)        //!< //!< if adc conversion is close read the pin 0xFF but this variable is
            {                                   // float, so value > 6.5 condition is consistent
                memset((void*)&ltcBat[i].statusRegA.sumOfCell, 0, sizeof(StatusRegA));
            }
            else
                return LTC_WRONG_CRC;
        }
    }

    return LTC_OK;
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
    uint8_t mode = 0;
    uint8_t index = 0;
    uint8_t j;

    for(j = 0; j < slaveNumber; j++)
    {
        for(i = 0; i < 15; i++)             //!< 15 == S pin number
        {
            if((S_PIN_ >> i) & 0x01)
            {
                index = i / 2;
                mode = i % 2;

                ltcBat[j].pwmDuty[index] &= ~(0xFF << (mode * 4));
                ltcBat[j].pwmDuty[index] |= (PWM_DUTY_LEVEL_ << (mode * 4));
            }
        }
    }

    AE_ltcWrite(&ltcBat[0].pwmDuty[0], cmdWRPWM_pu16);  // PWM1-12
    AE_ltcWrite(&ltcBat[0].pwmDuty[6], cmdWRPSB_pu16);  // PWM13-15
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
void AE_ltcPreBalance(Ltc682x * ltcBat, DischargeTime DIS_, float * underVolt, float * overVolt, uint16_t DCC_)
{
    uint16_t maskedDCC;

    for(i = 0; i < slaveNumber; i++)
    {
        ltcBat[i].cfgBr.CFGBR1.DTMEN |= 1;          //The LTC6812-1 has the ability to periodically monitor
                                                    //cell voltages while the discharge timer is active. The host
                                                    //should write the DTMEN bit in Configuration Register
                                                    //Group B to 1 to enable this feature.

        // clear related bits
        ltcBat[i].cfgAr.CFGAR4.cfg &= 0x00;
        ltcBat[i].cfgAr.CFGAR5.cfg &= 0x00;
        ltcBat[i].cfgBr.CFGBR0.cfg &= 0x8F;
        ltcBat[i].cfgBr.CFGBR1.cfg &= 0xFB;

        DCC_ &= ~(0xFFFF << (ltcBat[i].batConf.numberOfCell + 1));  //+1 comes from index shifting

        //DCC pins are scattered in several registers so we must mask them
        //Register Group A, CFGRA4 Mask = pin1-8    0x1FE
        maskedDCC = DCC_ & 0x1FE;
        maskedDCC >>= 1;           //pin1 start index 1 and dcc1 located 0th index in register
        ltcBat[i].cfgAr.CFGAR4.cfg |= maskedDCC;

        //Register Group A, CFGRA5 Mask = pin9-12    0x1E00
        maskedDCC = DCC_ & 0x1E00;
        maskedDCC >>= 9;            //pin9 start index 9 and dcc9 located 0th index in register
        ltcBat[i].cfgAr.CFGAR5.cfg |= maskedDCC;

        //set discharge duration
        ltcBat[i].cfgAr.CFGAR5.cfg |= (DIS_ << 4) & 0x00F0;

        //Register Group B, CFGBR0 Mask = pin13-15    0xE000
        maskedDCC = DCC_ & 0xE000;
        maskedDCC >>= 9;            //pin13 start index 13 and dcc13 located 4th index in register (13 - 4)
        ltcBat[i].cfgBr.CFGBR0.cfg |= maskedDCC;

        //Register Group B, CFGBR1 Mask = pin0    0xE000
        maskedDCC = DCC_ & 0x1;
        maskedDCC <<= 2;            //pin0 start index 0 and dcc0 located 2th index in register (0 - 2)
        ltcBat[i].cfgBr.CFGBR1.cfg |= maskedDCC;

        //!< enable discharge monitoring and set under and over voltage
    }

    AE_ltcSetUnderOverVoltage(ltcBat, underVolt, overVolt);       //float pointer olarak yaz 20/02/2024
    AE_ltcWrite((uint16_t*)&ltcBat->cfgAr, cmdWRCFGA_pu16);
    AE_ltcWrite((uint16_t*)&ltcBat->cfgBr, cmdWRCFGB_pu16);

    balanceStatus = LTC_IN_BALANCE;
}

/**
 * @brief balance the cell in polling mode by checking the under and overvoltage flags
 * @param[in] bms global variable
 * @param[in] slave's min cell voltage
 * @param[in] cell minimum discharge voltages
 * @return none
 */
void AE_ltcBalance(Ltc682x * ltcBat, float *minCellVoltages, float * minBalanceVoltages)
{
    uint16_t maskedDCC;

    for(i = 0; i < slaveNumber; i++)
    {
        //clear the configuration register-A
        memset((void*)&ltcBat[i].cfgAr.CFGAR0.cfg, 0, sizeof(CFGAR));

        //enable REFON bit
        ltcBat[i].cfgAr.CFGAR0.REFON = 1;

        //clear the DCC13-15, DCC0 and discharge timer enable
        ltcBat[i].cfgBr.CFGBR0.cfg &= 0x8F;
        ltcBat[i].cfgBr.CFGBR1.DTMEN = 0;
        ltcBat[i].cfgBr.CFGBR1.DCC0 = 0;
    }

    AE_ltcWrite((uint16_t*)&ltcBat[0].cfgAr, cmdWRCFGA_pu16);
    AE_ltcWrite((uint16_t*)&ltcBat[0].cfgBr, cmdWRCFGB_pu16);

    // set the under and overvoltage limit
    AE_ltcSetUnderOverVoltage(ltcBat, minBalanceVoltages, minCellVoltages);

    // take the under and overvoltage flags
    AE_ltcUnderOverFlag(ltcBat);

    for(i = 0; i < slaveNumber; i++)
    {
        maskedDCC = ltcBat[i].statusRegB.CellOverFlag.flag & ~(0xFFFF << ltcBat[i].batConf.numberOfCell);

        //<<<<<<<<<<<<<<<-Over Voltage->>>>>>>>>>>>>>>
        //DCC pins are scattered in several registers so we must mask them
        ltcBat[i].cfgAr.CFGAR4.cfg |= maskedDCC & 0xFF;

        //Register Group A, CFGRA5 Mask = pin9-12    0x1E00
        ltcBat[i].cfgAr.CFGAR5.cfg |= (maskedDCC >> 8) & 0x0F;

        //Register Group B, CFGBR0 Mask = pin13-15    0xE000
        ltcBat[i].cfgBr.CFGBR0.cfg |= (maskedDCC >> 8) & 0xF0;

        ltcBat[i].cfgAr.CFGAR5.DCTO = 1;
    }

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
void AE_ltcMinCellVolt(Ltc682x * ltcBat)
{
    float *fptr = NULL;
    float min;
    uint8_t j;

    for(i = 0; i < slaveNumber; i++)
    {
        fptr = (float *)&ltcBat[i].volt.cell1;
        min = fptr[0];

        for(j = 1; j < ltcBat[i].batConf.numberOfCell; j++)
        {
            if(fptr[j] < min)
                min = fptr[j];
        }

        ltcBat[i].minCellVolt = min;
    }
}

/**
 * @brief check for any open wires between the ADCs of the ltc681x
 * @param[in] bms global variable
 * @param[in] adc mode selection
 * @param[in] checked cell selection, for parameter search @refgroup CH
 * @return if no error return OK else OPEN_WIRE
 */
LTC_status AE_ltcIsCellOpenWire(Ltc682x * ltcBat, AdcMode adcMode, uint8_t CELL_, OpenWire *openWire)
{
    LTC_status status;
    float* cellPuPtr;
    float* cellPdPtr;
    float cellPu[CELL_NUMBER];
    float cellPd[CELL_NUMBER];
    uint8_t i;  // i is change inside of loop because i is declared global so some function change it so defined again
    uint8_t j;

    uint16_t adow= 0x0228;    //!< adow base register
    uint16_t cmd[4];
    uint16_t pec;

    if(adcMode & 0x10)
    {
        for(i = 0; i < slaveNumber; i++)
        {
            if(!ltcBat[i].cfgAr.CFGAR0.ADCOPT)
            {
                ltcBat[i].cfgAr.CFGAR0.ADCOPT = 1;
            }
        }
        AE_ltcWrite((uint16_t*)&ltcBat[0].cfgAr, cmdWRCFGA_pu16);
    }

    for(i = 0; i < slaveNumber; i++)
    {
        adow |= (1 << 6);       // 6.th index, PUL = 1 command register index
        adow |= (adcMode & 0x0F) << 7;
        adow |= CELL_;

        cmd[0] = (adow & 0xFF00) >> 8;
        cmd[1] = (adow & 0x00FF) >> 0;

        pec = AE_pec15((uint8_t*)cmd, 2);
        cmd[2] = (pec & 0xFF00) >> 8;
        cmd[3] = (pec & 0x00FF) >> 0;

        AE_ltcClearCellAdc(ltcBat);

        AE_ltcCmdWrite(cmd);
        AE_ltcCmdWrite(cmd);

        AE_ltcStartCellAdc(ltcBat, MODE_7KHZ, true, CELL_ALL);
        //!< check adcMeasure duration is completed
        while(!AE_ltcAdcMeasureState());
        status = AE_ltcReadCellVoltage(ltcBat);
        if(status == LTC_WRONG_CRC) return LTC_WRONG_CRC;

        cellPuPtr = (float*)&ltcBat[i].volt.cell1;
        for(j = 0; j < CELL_NUMBER; j++)
        {
            cellPu[j] = cellPuPtr[j];
        }

        adow &= ~(1 << 6);       // 6.th index, PUL = 0 command register index
        cmd[0] = (adow & 0xFF00) >> 8;
        cmd[1] = (adow & 0x00FF) >> 0;

        pec = AE_pec15((uint8_t*)cmd, 2);
        cmd[2] = (pec & 0xFF00) >> 8;
        cmd[3] = (pec & 0x00FF) >> 0;

        AE_ltcClearCellAdc(ltcBat);

        AE_ltcCmdWrite(cmd);
        AE_ltcCmdWrite(cmd);

        AE_ltcStartCellAdc(ltcBat, MODE_7KHZ, true, CELL_ALL);
        //!< check adcMeasure duration is completed
        while(!AE_ltcAdcMeasureState());
        status = AE_ltcReadCellVoltage(ltcBat);
        if(status == LTC_WRONG_CRC) return LTC_WRONG_CRC;

        cellPdPtr = (float *)&ltcBat[i].volt.cell1;
        for(j = 0; j < CELL_NUMBER; j++)
        {
            cellPd[j] = cellPdPtr[j];
        }

        for(j = 0; j < ltcBat[i].batConf.numberOfCell; j++)
        {
            if((cellPu[j] - cellPd[j]) < OPEN_WIRE_VOLTAGE)
            {
                openWire->slaveNumber = i + 1;
                openWire->cellNumber = j + 1;
                return LTC_OPEN_WIRE;
            }
        }
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
LTC_status AE_ltcIsGpioOpenWire(Ltc682x * ltcBat, AdcMode adcMode, uint8_t CELL_, OpenWire *openWire)
{
    LTC_status status;
    float* gioPuPtr;
    float* gioPdPtr;
    float gioPu[GPIO_NUMBER];
    float gioPd[GPIO_NUMBER];
    uint8_t i = 0;
    uint8_t j = 0;

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

    for(i = 0; i < slaveNumber; i++)
    {
        axow |= (1 << 6);       // 6.th index, PUL = 1 command register index
        axow |= (adcMode & 0x0F) << 7;
        axow |= CELL_;

        cmd[0] = (axow & 0xFF00) >> 8;
        cmd[1] = (axow & 0x00FF) >> 0;

        pec = AE_pec15((uint8_t*)cmd, 2);
        cmd[2] = (pec & 0xFF00) >> 8;
        cmd[3] = (pec & 0x00FF) >> 0;

        AE_ltcCmdWrite(cmd);
        AE_ltcCmdWrite(cmd);

        AE_ltcStartGpioAdc(ltcBat, MODE_7KHZ, GPIO_ALL);
        while(!AE_ltcAdcMeasureState());
        status = AE_ltcReadGpioVoltage(ltcBat);

        gioPuPtr = (float*)&ltcBat[i].gpio.gpio1;
        for(j = 0; j < GPIO_NUMBER; j++)
        {
            gioPu[j] = gioPuPtr[j];
        }

        axow &= ~(1 << 6);       // 6.th index, PUL = 0 command register index
        cmd[0] = (axow & 0xFF00) >> 8;
        cmd[1] = (axow & 0x00FF) >> 0;

        pec = AE_pec15((uint8_t*)cmd, 2);
        cmd[2] = (pec & 0xFF00) >> 8;
        cmd[3] = (pec & 0x00FF) >> 0;

        AE_ltcCmdWrite(cmd);
        AE_ltcCmdWrite(cmd);

        AE_ltcStartGpioAdc(ltcBat, MODE_7KHZ, GPIO_ALL);
        while(!AE_ltcAdcMeasureState());
        status = AE_ltcReadGpioVoltage(ltcBat);

        gioPdPtr = (float*)&ltcBat[i].gpio.gpio1;
        for(j = 0; j < GPIO_NUMBER; j++)
        {
            gioPd[j] = gioPdPtr[j];
        }

        for(i = 1; j < GPIO_NUMBER; j++) //gio 1-9
        {   //-0.400f comes from datasheet
            if((gioPu[j] - gioPd[j]) < OPEN_WIRE_VOLTAGE)
            {
                openWire->slaveNumber = i;
                openWire->gioNumber = j;
                return LTC_OPEN_WIRE;
            }
        }
    }

    return LTC_OK;
}

/**
 * @brief calculate the GPIO3 NTC temperature
 * @param[in] bms global variable
 * @return temperature in celcius
 */
void AE_ltcTemperature(Ltc682x * ltcBat, float * temperature)
{
    AE_ltcStartGpioAdc(ltcBat, MODE_7KHZ, GPIO_ALL);
    while(!AE_ltcAdcMeasureState());
    AE_ltcReadGpioVoltage(ltcBat);

    for(i = 0; i < slaveNumber; i++)
    {
        temperature[i] = AE_calculateTemp(ltcBat[i].gpio.gpio3, ltcBat[i].gpio.ref, PULL_DOWN);
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







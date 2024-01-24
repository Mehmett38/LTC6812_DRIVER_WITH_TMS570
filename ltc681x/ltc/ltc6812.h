/**
 * ltc6812.h
 *
 *  @date 19 Oca 2024
 *  @author: mehmet.dincer
 */

#ifndef LTC681X_LTC6812_H_
#define LTC681X_LTC6812_H_


#include "stdint.h"
#include "stdlib.h"
#include "spi.h"
#include "het.h"
#include "sys_core.h"
#include "gio.h"
#include "crc15.h"
#include "stdbool.h"


//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<-MACROS->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#define WRITE_BYTES_IN_REG              (6)
#define READ_BYTES_IN_REG               (8)
#define CMD_LEN                         (4)


#define AE_LTC_CS_ON()              (spiREG1->PC3 &= ~(1 << 0)) //drive low the cs0 pin
#define AE_LTC_CS_OFF()             (spiREG1->PC3 |= 1 << 0)    //drive hight the cs0 pins

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<-ENUMS->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
typedef enum{
    LTC_ERROR,
    LTC_WRONG_CRC,
    LTC_OK,
    LTC_CRC_OK
}LTC_status;

typedef enum{
    WRCFGA,     //Write Configuration Register Group A
    WRCFGB      //Write Configuration Register Group B
}WRCFG;

typedef enum{
    RDCFGA,
    RDCFGB
}RDCFG;

typedef enum{
    DIS_DISABLED,
    DIS_05_MIN,
    DIS_1_MIN,
    DIS_2_MIN,
    DIS_3_MIN,
    DIS_4_MIN,
    DIS_5_MIN,
    DIS_10_MIN,
    DIS_15_MIN,
    DIS_20_MIN,
    DIS_30_MIN,
    DIS_40_MIN,
    DIS_60_MIN,
    DIS_75_MIN,
    DIS_90_MIN,
    DIS_120_MIN
}DischargeTime;

typedef enum{
    PATH_ADC_1 = 0b01,
    PATH_ADC_2 = 0b10,
    PATH_ADC_3 = 0b11,
    PATH_ADC_ALL = 0b00
}PathSelection;


//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<-STRUCTURES->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
typedef struct
{
  uint16_t txData_u8[6];  //!< Stores data to be transmitted
  //! TMS570 spi line take at least 16bit
  uint16_t rxData_u8[8];  //!< Stores received data
}IcRegister;

typedef struct
{
  uint8_t cellChannels_u8; //!< Number of Cell channels
  uint8_t statChannels_u8; //!< Number of Stat channels
  uint8_t auxChannels_u8;  //!< Number of Aux channels
  uint8_t numCvReg_u8;    //!< Number of Cell voltage register
  uint8_t numGpioReg_u8;  //!< Number of Aux register
  uint8_t numStatReg_u8;  //!< Number of  Status register
}RegisterCfg;

typedef struct{
    IcRegister configA;
    IcRegister configB;
    RegisterCfg icReg;

}CellInf;

extern spiDAT1_t __spiDat_s;
extern volatile uint32_t uwTick;

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<-GLOBAL VARIABLES->>>>>>>>>>>>>>>>>>>>>>>>>>>>>
static uint16_t cmdWRCFGA_pu16[4]   = {0x00, 0x01, 0x3D, 0x6E}; // last 2 index CRC
static uint16_t cmdWRCFGB_pu16[4]   = {0x00, 0x24, 0xB1, 0x9E};
static uint16_t cmdRDCFGA_pu16[4]   = {0x00, 0x02, 0x2B, 0x0A};
static uint16_t cmdRDCFGB_pu16[4]   = {0x00, 0x26, 0x2C, 0xC8}; // !note
static uint16_t cmdRDCVA_pu16[4]    = {0x00, 0x04, 0x07, 0xC2};
static uint16_t cmdRDCVB_pu16[4]    = {0x00, 0x06, 0x9A, 0x94};
static uint8_t cmdRDCVC_pu16[4]    = {0x00, 0x08, 0x5E, 0x52};
static uint8_t cmdRDCVD_pu16[4]    = {0x00, 0x0A, 0xC3, 0x04};
static uint8_t cmdRDCVE_pu16[4]    = {0x00, 0x09, 0xD5, 0x60};
static uint8_t cmdRDAUXA_pu16[4]   = {0x00, 0x0C, 0xEF, 0xCC};
static uint8_t cmdRDAUXB_pu16[4]   = {0x00, 0x0E, 0x72, 0x9A};
static uint8_t cmdRDAUXC_pu16[4]   = {0x00, 0x0D, 0x64, 0xFE};
static uint8_t cmdRDAUXD_pu16[4]   = {0x00, 0x0F, 0xF9, 0xA8};
static uint8_t cmdRDSTATA_pu16[4]  = {0x01, 0x00, 0xED, 0x72}; // !note
static uint8_t cmdRDSTATB_pu16[4]  = {0x00, 0x12, 0x70, 0x24}; // !note
static uint8_t cmdWRSCTRL_pu16[4]  = {0x00, 0x14, 0x5C, 0xEC}; // !note
static uint8_t cmdWRPWM_pu16[4]    = {0x00, 0x20, 0x00, 0x00}; // !note
static uint8_t cmdWRPSB_pu16[4]    = {0x00, 0x1C, 0xB4, 0xE2}; // !note
static uint8_t cmdRDSCTRL_pu16[4]  = {0x00, 0x16, 0xC1, 0xBA}; // !note
static uint8_t cmdRDPWM_pu16[4]    = {0x00, 0x22, 0x9D, 0x56}; // !note
static uint8_t cmdRDPSB_pu16[4]    = {0x00, 0x1E, 0x29, 0xB4}; // !note
static uint8_t cmdSTSCTRL_pu16[4]  = {0x00, 0x19, 0x8E, 0x4E}; // !note
static uint8_t cmdCLRSCTRL_pu16[4] = {0x00, 0x18, 0x05, 0x7C}; // !note


//<<<<<<<<<<<<<<<<<<<<<<<<<-FUNCTION PROTOTYPES->>>>>>>>>>>>>>>>>>>>>>>>>>
void AE_ltcInit(spiBASE_t * ltcSpi_ps, uint8_t slaveNumber_u8, CellInf * cellinf_s);
void AE_ltcWRCFG(CellInf *cellInf, WRCFG wrcfgX_e);
void AE_ltcWrite681x(uint16_t *txCmd_pu8, uint16_t * data_pu8);
LTC_status AE_ltcRDCFG(CellInf *cellInf, RDCFG rdcfg_e);
LTC_status AE_ltcRead681x(uint16_t *txCmd_pu8, uint16_t * data_pu8);
void AE_ltcSetcfgraRefOn(CellInf *cellInf_ps, bool refOn_b);
void AE_ltcSetcfgraAdcOpt(CellInf *cellInf_ps, bool adcOpt_b);
void AE_ltcSetcfgraGpio(CellInf *cellInf_ps, bool gpio[5]);
void AE_ltcSetcfgraDcc(CellInf *cellInf_ps, bool dcc[12]);
void AE_ltcSetcfgraDct0(CellInf*cellInf_ps, DischargeTime dct0);
void AE_ltcSetcfgraUnderVoltage(CellInf*cellInf_ps, float underVolt);
void AE_ltcSetcfgraOverVoltage(CellInf*cellInf_ps, float overVolt);
void Ae_ltcSetcfgrbFdrf(CellInf *cellInf_ps, bool fdrf);
void Ae_ltcSetcfgrbDTMEN(CellInf *cellInf_ps, bool dtmen);
void Ae_ltcSetcfgrbPs(CellInf *cellInf_ps, PathSelection ps);
void AE_ltcSetcfgrbGpio(CellInf *cellInf_ps, bool gpio[4]);
void AE_ltcSetcfgrbDcc(CellInf *cellInf_ps, bool dcc[4]);
LTC_status AE_ltcWriteConfiguration(CellInf *cellInf);
void AE_ltcWakeUpSleep();
void AE_ltcWakeUpIdle();
static void AE_ltcTick(uint32_t );
uint32_t getUsTick();
void AE_delayMs(uint32_t delay_u32);
void AE_delayTenUs(uint32_t delay_u32);


#endif /* LTC681X_LTC6812_H_ */

/**
 * @brief ltc6812.h
 * @data 19 Oca 2024
 * @author: Mehmet Dinçer
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
#include "string.h"
#include "stdlib.h"
#include "math.h"
#include "temperature.h"

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<-MACROS->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#define WRITE_BYTES_IN_REG              (6)
#define READ_BYTES_IN_REG               (8)
#define CMD_LEN                         (4)
#define ENABLE                          (1)
#define DISABLE                         (0)
#define GPIO_A_RESET                    (0xF8)
#define GPIO_B_RESET                    (0x0F)
#define CMD_LEN                         (4)         //cmd packet len 4byte
#define REGISTER_LEN                    (6)
#define TRANSMIT_LEN                    (8)
#define RECEIVE_LEN                     (8)

#define CFGARO_GPIO1                    (3)         //index of GPIO1 parameter in register CFGRA0

//!< @refgroup GPIOA
#define GPIO_1                          (1 << 1)
#define GPIO_2                          (1 << 2)
#define GPIO_3                          (1 << 3)
#define GPIO_4                          (1 << 4)
#define GPIO_5                          (1 << 5)
#define GPIO_6                          (1 << 6)
#define GPIO_7                          (1 << 7)
#define GPIO_8                          (1 << 8)

//!< @refgroup cell
#define CELL_1                          (1 << 0 )
#define CELL_2                          (1 << 1 )
#define CELL_3                          (1 << 2 )
#define CELL_4                          (1 << 3 )
#define CELL_5                          (1 << 4 )
#define CELL_6                          (1 << 5 )
#define CELL_7                          (1 << 6 )
#define CELL_8                          (1 << 7 )
#define CELL_9                          (1 << 8 )
#define CELL_10                         (1 << 9 )
#define CELL_11                         (1 << 10)
#define CELL_12                         (1 << 11)
#define CELL_13                         (1 << 12)
#define CELL_14                         (1 << 13)
#define CELL_15                         (1 << 14)

//!< @refgroup dcc
#define DCC_0                           (1 << 0)
#define DCC_1                           (1 << 1)
#define DCC_2                           (1 << 2)
#define DCC_3                           (1 << 3)
#define DCC_4                           (1 << 4)
#define DCC_5                           (1 << 5)
#define DCC_6                           (1 << 6)
#define DCC_7                           (1 << 7)
#define DCC_8                           (1 << 8)
#define DCC_9                           (1 << 9)
#define DCC_10                          (1 << 10)
#define DCC_11                          (1 << 11)
#define DCC_12                          (1 << 12)
#define DCC_13                          (1 << 13)
#define DCC_14                          (1 << 14)
#define DCC_15                          (1 << 15)
#define DCC_ALL                         (0xFFFF)

//!< @refgroup CH
#define CELL_ALL                        (0x00)
#define CELL_1_6_11                     (0x01)
#define CELL_2_7_12                     (0x02)
#define CELL_3_8_13                     (0x03)
#define CELL_4_9_14                     (0x04)
#define CELL_5_10_15                    (0x05)

//!< @refgroup CHG
#define GPIO_ALL                        (0x00)
#define GPIO_1_6                        (0x01)
#define GPIO_2_7                        (0x02)
#define GPIO_3_8                        (0x03)
#define GPIO_4_9                        (0x04)
#define GPIO_5_0                        (0x05)
#define GPIO_2ND_REFERANCE              (0x06)

//!< @refgroup CHST
#define CHST_ALL                        (0x00)
#define CHST_SC                         (0x01)      //!< Sum of all cell
#define CHST_ITMP                       (0x02)      //!< Internal die temperature
#define CHST_VA                         (0x03)      //!< Analog power supply voltage
#define CHST_VD                         (0x04)      //!< Digital power supply voltage

//!< @refgroup sPin
#define S_PIN_1                         (1 << 0)
#define S_PIN_2                         (1 << 1)
#define S_PIN_3                         (1 << 2)
#define S_PIN_4                         (1 << 3)
#define S_PIN_5                         (1 << 4)
#define S_PIN_6                         (1 << 5)
#define S_PIN_7                         (1 << 6)
#define S_PIN_8                         (1 << 7)
#define S_PIN_9                         (1 << 8)
#define S_PIN_10                        (1 << 9)
#define S_PIN_11                        (1 << 10)
#define S_PIN_12                        (1 << 11)
#define S_PIN_13                        (1 << 12)
#define S_PIN_14                        (1 << 13)
#define S_PIN_15                        (1 << 14)
#define S_PIN_ALL                       (0xFFFF)

//!< @refgroup pwmDutyLevel
#define PWM_DUTY_LEVEL_0                (0)     //!< %0
#define PWM_DUTY_LEVEL_1                (1)     //!< %6.7
#define PWM_DUTY_LEVEL_2                (2)     //!< %13.3
#define PWM_DUTY_LEVEL_3                (3)     //!< ...
#define PWM_DUTY_LEVEL_4                (4)
#define PWM_DUTY_LEVEL_5                (5)
#define PWM_DUTY_LEVEL_6                (6)
#define PWM_DUTY_LEVEL_7                (7)
#define PWM_DUTY_LEVEL_8                (8)
#define PWM_DUTY_LEVEL_9                (9)
#define PWM_DUTY_LEVEL_10               (10)
#define PWM_DUTY_LEVEL_11               (11)
#define PWM_DUTY_LEVEL_12               (12)
#define PWM_DUTY_LEVEL_13               (13)
#define PWM_DUTY_LEVEL_14               (14)
#define PWM_DUTY_LEVEL_15               (15)    //!< %100

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<-GLOBAL VARIABLES->>>>>>>>>>>>>>>>>>>>>>>>>>>>>
extern spiBASE_t * ltcSpi_ps;                                 // spi base address

#define AE_LTC_CS_ON()                  (ltcSpi_ps->PC3 &= ~(1 << 0)) //drive low the cs0 pin
#define AE_LTC_CS_OFF()                 (ltcSpi_ps->PC3 |= 1 << 0)    //drive hight the cs0 pins

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<-ENUMS->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
typedef enum{
    LTC_WRONG_CRC,
    LTC_OK,
    LTC_OPEN_WIRE,
    LTC_IN_BALANCE,
    LTC_BALANCE_COMPLETED
}LTC_status;

typedef enum{
    WRCFGA,     //Write Configuration Register Group A
    WRCFGB      //Write Configuration Register Group B
}WRCFG;

typedef enum{
    RDCFGA,
    RDCFGB
}RDCFG;

//!< @refgrouDTC discharge time macros
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

typedef enum{
    MODE_422HZ = 0x00,  // ADCOPT = 0
    MODE_1KHZ = 0x10,   // ADCOPT = 1
    MODE_27KHZ = 0x01,  // ADCOPT = 0
    MODE_14KHZ = 0x11,  // ADCOPT = 1
    MODE_7KHZ = 0x02,
    MODE_3KHZ = 0x12,
    MODE_26HZ = 0x03,
    MODE_2KHZ = 0x13
}AdcMode;

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<-STRUCTURES->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
typedef struct{
    union{
        uint16_t cfg;
        struct{
            uint16_t : 8;
            uint16_t GPIO5 : 1;      //!< Write: 0 -> GPIOx Pin Pull-Down ON;
                                    //!< 1 -> GPIOx Pin Pull-Down OFF (Default)
            uint16_t GPIO4 : 1;
            uint16_t GPIO3 : 1;
            uint16_t GPIO2 : 1;
            uint16_t GPIO1 : 1;
            uint16_t REFON : 1;      //!< 1 -> References Remain Powered Up Until Watchdog Timeout
                                    //!< 0 -> References Shut Down After Conversions (Default)
            uint16_t DTEN : 1;       //!< 1 -> Enables the Discharge Timer for Discharge Switches
                                    //!< 0 -> Disables Discharge Timer
            uint16_t ADCOPT : 1;     //!< @refgroup ADCOPT, 0 -> Selects Modes 27kHz, 7kHz, 422Hz or 26Hz with MD[1:0] Bits in ADC Conversion Commands (Default)
                                    //!< 1 -> Selects Modes 14kHz, 3kHz, 1kHz or 2kHz with MD[1:0] Bits in ADC Conversion Commands
        };
    }CFGAR0;

    union{
        uint16_t cfg;
        struct{
            uint16_t : 8;
            uint16_t VUV7 : 1;       //!< Comparison Voltage = (VUV + 1) • 16 • 100μV
                                    //!< Default: VUV = 0x000
            uint16_t VUV6 : 1;
            uint16_t VUV5 : 1;
            uint16_t VUV4 : 1;
            uint16_t VUV3 : 1;
            uint16_t VUV2 : 1;
            uint16_t VUV1 : 1;
            uint16_t VUV0 : 1;
        };
    }CFGAR1;

    union{
        uint16_t cfg;
        struct{
            uint16_t : 8;
            uint16_t VOV3 : 1;       //!< Comparison Voltage = VOV • 16 • 100μV
                                    //!< Default: VOV = 0x000
            uint16_t VOV2 : 1;
            uint16_t VOV1 : 1;
            uint16_t VOV0 : 1;
            uint16_t VUV11 : 1;      //!< Comparison Voltage = (VUV + 1) • 16 • 100μV
                                    //!< Default: VUV = 0x000
            uint16_t VUV10 : 1;
            uint16_t VUV9 : 1;
            uint16_t VUV8 : 1;
        };
    }CFGAR2;

    union{
        uint16_t cfg;
        struct{
            uint16_t : 8;
            uint16_t VOV11 : 1;
            uint16_t VOV10 : 1;
            uint16_t VOV9 : 1;
            uint16_t VOV8 : 1;
            uint16_t VOV7 : 1;
            uint16_t VOV6 : 1;
            uint16_t VOV5 : 1;
            uint16_t VOV4 : 1;
        };
    }CFGAR3;

    union{
        uint16_t cfg;
        struct{
            uint16_t : 8;
            uint16_t DCC8 : 1;        //!< x = 1 to 15: 1 -> Turn ON Shorting Switch for Cell x
                                    //!< 0 -> Turn OFF Shorting Switch for Cell x (Default)
                                    //!< x = 0: 1 -> Turn ON GPIO9 Pull-Down
                                    //!< 0 -> Turn OFF GPIO9 Pull-Down (Default)
            uint16_t DCC7 : 1;
            uint16_t DCC6 : 1;
            uint16_t DCC5 : 1;
            uint16_t DCC4 : 1;
            uint16_t DCC3 : 1;
            uint16_t DCC2 : 1;
            uint16_t DCC1 : 1;
        };
    }CFGAR4;

    union{
        uint16_t cfg;
        struct{
            uint16_t : 8;
            uint16_t DCT3 : 1;        //!<  Discharge Time Out Value @refgrouDTC
            uint16_t DCT2 : 1;
            uint16_t DCT1 : 1;
            uint16_t DCTO : 1;
            uint16_t DCC12 : 1;
            uint16_t DCC11 : 1;
            uint16_t DCC10 : 1;
            uint16_t DCC9 : 1;
        };
    }CFGAR5;

    uint16_t pec0;      //!< pec 7-14
    uint16_t pec1;      //!<  pec 0-7
}CFGAR;


typedef struct{
    union{
        uint16_t cfg;
        struct{
            uint16_t : 8;      //!< tms send 16 bit data
            uint16_t       : 1;
            uint16_t DCC15 : 1;
            uint16_t DCC14 : 1;
            uint16_t DCC13 : 1;
            uint16_t GPIO9 : 1;
            uint16_t GPIO8 : 1;
            uint16_t GPIO7 : 1;
            uint16_t GPIO6 : 1;
        };
    }CFGBR0;

    union{
        uint16_t cfg;
        struct{
            uint16_t : 8;
            uint16_t MUTE : 1;
            uint16_t FDRF : 1;
            uint16_t PS1  : 1;
            uint16_t PS0  : 1;
            uint16_t DTMEN : 1;
            uint16_t DCC0 : 1;
            uint16_t      : 2;
        };
    }CFGBR1;

    uint16_t CFGBR2;    //!< reserved
    uint16_t CFGBR3;    //!< reserved
    uint16_t CFGBR4;    //!< reserved
    uint16_t CFGBR5;    //!< reserved

    uint16_t pec0;      //!< pec 7-14
    uint16_t pec1;      //!<  pec 0-7
}CFGBR;

typedef struct{
    uint8_t numberOfSerialCell;     // cell number in a slave
    uint8_t numberOfSlave;          // slave number
    bool adcopt;                    // using to configure system ADC read speed
    bool refon;                     // to understand better investigate the core working diagram
    uint16_t gioAPullOffPin;         // @refgroup gioA   set as pull of
    uint16_t gioBPullOffPin;         // @refgroup gioB   set as pull of
    bool dischargeTimeMonitor;
}BatteryConf;

typedef struct{
    float cell1;
    float cell2;
    float cell3;
    float cell4;
    float cell5;
    float cell6;
    float cell7;
    float cell8;
    float cell9;
    float cell10;
    float cell11;
    float cell12;
    float cell13;
    float cell14;
    float cell15;
 }CellVolt;

 typedef struct{
     float gpio1;
     float gpio2;
     float gpio3;
     float gpio4;
     float gpio5;
     float ref;
     float gpio6;
     float gpio7;
     float gpio8;
     float gpio9;
 }Gpio;

typedef struct{
    float sumOfCell;
    float internalDieTemp;  //!< internal die temperature Temperature Measurement Voltage = ITMP • 100μV/7.6mV/°C – 276°C
    float analogPowerSupplyVolt;
}StatusRegA;

typedef struct{
    float digitalPowerSupplyVolt;
    union{
        uint16_t flag;
        struct{
            uint16_t : 1;
            uint16_t cell15 : 1;
            uint16_t cell14 : 1;
            uint16_t cell13 : 1;
            uint16_t cell12 : 1;
            uint16_t cell11 : 1;
            uint16_t cell10 : 1;
            uint16_t cell9 : 1;
            uint16_t cell8 : 1;
            uint16_t cell7 : 1;
            uint16_t cell6 : 1;
            uint16_t cell5 : 1;
            uint16_t cell4 : 1;
            uint16_t cell3 : 1;
            uint16_t cell2 : 1;
            uint16_t cell1 : 1;
        };
    }CellOverFlag;

    union{
        uint16_t flag;
        struct{
            uint16_t : 1;
            uint16_t cell15 : 1;
            uint16_t cell14 : 1;
            uint16_t cell13 : 1;
            uint16_t cell12 : 1;
            uint16_t cell11 : 1;
            uint16_t cell10 : 1;
            uint16_t cell9 : 1;
            uint16_t cell8 : 1;
            uint16_t cell7 : 1;
            uint16_t cell6 : 1;
            uint16_t cell5 : 1;
            uint16_t cell4 : 1;
            uint16_t cell3 : 1;
            uint16_t cell2 : 1;
            uint16_t cell1 : 1;
        };
    }CellUnderFlag;

    uint8_t muxFail;    //!< Read: 0 → Multiplexer Passed Self Test; 1 → Multiplexer Failed Self Test
    uint8_t thsd;       //!< 0 → Thermal Shutdown Has Not Occurred; 1 → Thermal Shutdown Has Occurred
    uint8_t revionCode;
}StatusRegB;

typedef struct{
    union{
        uint16_t flg;
        struct{

        };
    };
}OverVoltageFlag;

typedef struct{
    CFGAR cfgAr;
    CFGBR cfgBr;
    BatteryConf batConf;
    CellVolt volt;
    Gpio gpio;
    StatusRegA statusRegA;
    StatusRegB statusRegB;
    uint16_t pwmDuty[12];                /* ***Do not use this variable
                                         pwm blocks valid on PWM Register Group and PWM/S Control Register
                                         Group B so, send 12 bytes data*/
}Ltc682x;


//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<-GLOBAL VARIABLES->>>>>>>>>>>>>>>>>>>>>>>>>>>>>
static uint16_t cmdWRCFGA_pu16[4]   = {0x00, 0x01, 0x3D, 0x6E}; // last 2 index CRC
static uint16_t cmdWRCFGB_pu16[4]   = {0x00, 0x24, 0xB1, 0x9E};
static uint16_t cmdRDCFGA_pu16[4]   = {0x00, 0x02, 0x2B, 0x0A};
static uint16_t cmdRDCFGB_pu16[4]   = {0x00, 0x26, 0x2C, 0xC8};
static uint16_t cmdRDCVA_pu16[4]    = {0x00, 0x04, 0x07, 0xC2};
static uint16_t cmdRDCVB_pu16[4]    = {0x00, 0x06, 0x9A, 0x94};
static uint16_t cmdRDCVC_pu16[4]    = {0x00, 0x08, 0x5E, 0x52};
static uint16_t cmdRDCVD_pu16[4]    = {0x00, 0x0A, 0xC3, 0x04};
static uint16_t cmdRDCVE_pu16[4]    = {0x00, 0x09, 0xD5, 0x60};
static uint16_t cmdRDAUXA_pu16[4]   = {0x00, 0x0C, 0xEF, 0xCC};
static uint16_t cmdRDAUXB_pu16[4]   = {0x00, 0x0E, 0x72, 0x9A};
static uint16_t cmdRDAUXC_pu16[4]   = {0x00, 0x0D, 0x64, 0xFE};
static uint16_t cmdRDAUXD_pu16[4]   = {0x00, 0x0F, 0xF9, 0xA8};
static uint16_t cmdRDSTATA_pu16[4]  = {0x00, 0x10, 0xED, 0x72};
static uint16_t cmdRDSTATB_pu16[4]  = {0x00, 0x12, 0x70, 0x24};
static uint16_t cmdWRSCTRL_pu16[4]  = {0x00, 0x14, 0x5C, 0xEC};
static uint16_t cmdWRPWM_pu16[4]    = {0x00, 0x20, 0x00, 0x00};
static uint16_t cmdWRPSB_pu16[4]    = {0x00, 0x1C, 0xB4, 0xE2};
static uint16_t cmdRDSCTRL_pu16[4]  = {0x00, 0x16, 0xC1, 0xBA};
static uint16_t cmdRDPWM_pu16[4]    = {0x00, 0x22, 0x9D, 0x56};
static uint16_t cmdRDPSB_pu16[4]    = {0x00, 0x1E, 0x29, 0xB4};
static uint16_t cmdSTSCTRL_pu16[4]  = {0x00, 0x19, 0x8E, 0x4E};
static uint16_t cmdCLRSCTRL_pu16[4] = {0x00, 0x18, 0x05, 0x7C};
static uint16_t cmdCLRCELL_pu16[4]  = {0x07, 0x11, 0xC9, 0xC0};
static uint16_t cmdCLRAUX_pu16[4]   = {0x07, 0x12, 0xDF, 0xA4};
static uint16_t cmdCLRSTAT_pu16[4]  = {0x07, 0x13, 0x54, 0x96};
static uint16_t cmdMute_pu16[4]     = {0x00, 0x28, 0xE8, 0x0E};
static uint16_t cmdUnMute_pu16[4]   = {0x00, 0x29, 0x63, 0x3C};
static uint16_t cmdPladc_pu16[4]    = {0x07, 0x14, 0xF3, 0x6C};
static uint16_t cmdClrCell_pu16[4]  = {0x07, 0x11, 0xC9, 0xC0};

static uint16_t cmdAdstat_pu16[4]   = {0x04, 0x68, 0xB3, 0xE2};

//<<<<<<<<<<<<<<<<<<<<<<<<<-FUNCTION PROTOTYPES->>>>>>>>>>>>>>>>>>>>>>>>>>
void AE_ltcInit(spiBASE_t * spi, Ltc682x * ltcBat);

void AE_ltcWrite(uint16_t * txData, uint16_t cmd[4]);
void AE_ltcCmdWrite(uint16_t cmd[4]);

LTC_status AE_ltcRead(uint16_t * rxData, uint16_t cmd[4]);
uint16_t AE_ltcCmdRead(uint16_t cmd[4]);

void AE_ltcWakeUpSleep();
void AE_ltcWakeUpIdle();

void AE_ltcWriteConfRegA(Ltc682x * ltcBat);
void AE_ltcWriteConfRegB(Ltc682x * ltcBat);

LTC_status AE_ltcReadConfRegA(Ltc682x * ltcBat);
LTC_status AE_ltcReadConfRegB(Ltc682x * ltcBat);

uint8_t AE_ltcAdcMeasureState();

void AE_ltcStartCellAdc(Ltc682x * ltcBat, AdcMode adcMode, uint8_t dischargePermit, uint8_t selectedCell);
LTC_status AE_ltcReadCellVoltage(Ltc682x * ltcBat);
LTC_status AE_ltcClearCellAdc(Ltc682x * ltcBat);

void AE_ltcSetUnderOverVoltage(Ltc682x * ltcBat, float underVolt, float overVolt);
LTC_status AE_ltcUnderOverFlag(Ltc682x * ltcBat);

void AE_ltcStartGpioAdc(Ltc682x * ltcBat, AdcMode adcMode, uint8_t GPIO_);
LTC_status AE_ltcReadGpioVoltage(Ltc682x * ltcBat);
LTC_status AE_ltcClearGpioAdc(Ltc682x * ltcBat);

void AE_ltcStartStatusAdc(Ltc682x * ltcBat, AdcMode adcMode, uint8_t GPIO_);
LTC_status AE_ltcReadStatusRegA(Ltc682x * ltcBat);
LTC_status AE_ltcReadStatusRegB(Ltc682x * ltcBat);
LTC_status AE_ltcClearStatusAdc(Ltc682x * ltcBat);

void AE_ltcStartPwm(Ltc682x * ltcBat, uint16_t S_PIN_, uint8_t PWM_DUTY_LEVEL_);
void AE_ltcPausePwm(Ltc682x * ltcBat);
void AE_ltcContinuePwm(Ltc682x * ltcBat);

void AE_ltcPreBalance(Ltc682x * ltcBat, DischargeTime DIS_, float underVolt, float overVolt, uint16_t DCC_);
void AE_ltcBalance(Ltc682x * ltcBat, float minVoltage);
LTC_status AE_ltcIsBalanceComplete(Ltc682x * ltcBat);
float AE_ltcMinCellVolt(Ltc682x * ltcBat);

LTC_status AE_ltcIsCellOpenWire(Ltc682x * ltcBat, AdcMode adcMode, uint8_t CELL_);
LTC_status AE_ltcIsGpioOpenWire(Ltc682x * ltcBat, AdcMode adcMode, uint8_t CELL_);

void AE_ltcTemperature(Ltc682x * ltcBat, float * temperature);

static void AE_ltcTick(uint32_t );
uint32_t getUsTick();
void AE_delayMs(uint32_t delay_u32);
void AE_delayTenUs(uint32_t delay_u32);


#endif /* LTC681X_LTC6812_H_ */

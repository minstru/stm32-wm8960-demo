/* USER CODE BEGIN Header */
/*
 * Copyright (c) 2025-present Mind Instrument
 * Distributed under the MIT software license, see the accompanying file LICENSE.txt.
 * For those auto-generated code, please refer to the original author license.
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "gpio.h"

#include "wm8960.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
eWm8960InputSrc audio_src = WM8960_INPUT_LINEIN;
#define IIC_SLAVE_ADDR       (0x1A<<1)

void set_wm8960_input_src(eWm8960InputSrc src);
void write_wm8960_reg(uint8_t reg_addr, uint16_t reg_data);

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t wm8960_shadow_reg[56];

uint16_t wm8960_line_cfg[] = {
    // addr, data
    15, 0x0000,     // reset

    //Audio Interface
    // bit[7:8]     bit[6]     bit[5]      bit[4:3]        bit[2:1]     bit[0]
    // 00,        ALRCGPIO=1, WL8=0,       DACCOMP=00,   ADCCOMP=00, LOOPBACK=0
    9, (0x1 << 6),

    // power management1
    // bit[8:7]          bit6    bit5    bit4    bit3    bit2    bit1    bit0
    // VMIDSEL[1:0]=10b, VREF=1, AINL=1, AINR=1, ADCL=1, ADCR=1, MICB=0, DIGENB=0
    25, (0x01 << 7) | (0x01 << 6) | (0x01 << 5) | (0x01 << 4) | (0x01 << 3) | (0x01 << 2) | (0x00 << 1) | 0x00,

    // power management2
    // bit8    bit7    bit6     bit5     bit4    bit3    bit1    bit0
    // DACL=1, DACR=1, LOUT1=1, ROUT1=1, SPKL=1, SPKR=1, OUT3=0, PLL_EN=1
    26, (0x01<<8) | (0x01<<7) | (0x01<<6) | (0x01<<5) | (0x01<<4) | (0x01<<3) | (0x00<<1) | (0x01<<0),

    // power management3 
    // bit5    bit4     bit3    bit2
    // LMIC=0, RMIC=0, LOMIX=1, ROMIX=1
    47, (0x00<<5) | (0x00<<4) | (0x01<<3) | (0x01<<2),  // disable MIC input

    // ADCL signal path
    // bit[8]  bit[7]  bit[6]   bit[5:4]           bit3
    // LMN1=0, LMP3=0, LMP2=0, LMICBOOST[1:0]=00b, LMIC2B=0
    32, (0x00<<8) | (0x00<<7) | (0x00<<6) | (0x00<<4) | (0x00<<3),      

    // ADCR signal path
    // bit[8]  bit[7]  bit[6]   bit[5:4]           bit3
    // RMN1=0, RMP3=0, RMP2=0, RMICBOOST[1:0]=00b, RMIC2B=0
    33, (0x00<<8) | (0x00<<7) | (0x00<<6) | (0x00<<4) | (0x00<<3),   

    // left input volume
    // bit8     bit7      bit6,   bit[5:0]
    // IPVU=1, LINMUTE=0, LIZC=0, LINVOL[5:0]=0x1F
    0, (0x01<<8) | (0x00<<7) | (0x1F<<0),   

    // right input volume
    // bit8     bit7      bit6,   bit[5:0]
    // IPVU=1, RINMUTE=0, RIZC=0, RINVOL[5:0]=0x1F
    1, (0x01<<8) | (0x00<<7) | (0x1F<<0),   

    // automatic level control
    17, (0x02<<7) | (0x07<<4) | (0x0B<<0),
    18, (0x00<<4) | (0x00<<0),
    19, (0x00<<8) | (0x03<<4) | (0x02<<0),


    // left out mix: Choose one of three
    // bit[8]   bit[7]   bit[6:4]
    // LD2LO=1, LI2LO=0, LI2LOVOL[2:0]=000b
    34, (0x01<<8),      // (1)enable Left DAC To Left output Mixer, LI2LO=0
    // LD2LO=0, LI2LO=1, LI2LOVOL[2:0]=000b
    // 34, (0x01<<7),      // (2)enable LINPUT3 To Left output Mixer
    // 34, 0,              // (3)enable Left Input Boost Mixer Output To Left output Mixer
    // bit7     bit[6:4]
    // LB2LO=1, LB2LOVOL[2:0]=000b
    // 45, (0x01<<7),      // (3)enable Left Input Boost Mixer Output To Left output Mixer

    // right out mix: choose one of two
    // bit[8]   bit[7]   bit[6:4]
    // RD2Ro=1, RI2RO=0, RI2ROVOL[2:0]=000b
    37, (0x01<<8),   // (1)enable Right DAC To Right output Mixer
    // RD2RO=0, RI2RO=0, RI2ROVOL[2:0]=000b
    // 37, 0,              // (2)enable Right Input Boost Mixer Output To Right output Mixer
    // bit7     bit[6:4]
    // RB2RO=1, RB2ROVOL[2:0]=000b
    // 46, (0x01<<7),      // (2)enable Right Input Boost Mixer Output To Right output Mixer

    // ADC & DAC control
    // bit7      bit[6:5]     bit3     bit[2:1]       bit0
    //DACDIV2=0, ADCPOL[1:0], DACMU=0, DEEMPH[1:0]=0, ADCPHD=1
    5, 0x0000,          // DAC unmute, disable high pass filter on left and right channels

    // LOUT1 volume
    // bit8      bit7     bit6~0
    // OUT1VU=1, LO1ZC=0, LOUT1VOL[6:0]=0x79->0dB
    2, (0x01<<8) | (0x00<<7) | 0x79,

    // ROUT1 volume
    // bit8      bit7     bit6~0
    // OUT1VU=1, LO1ZC=0, LOUT1VOL[6:0]=0x79->0dB
    3, (0x01<<8) | (0x00<<7) | 0x79,

    // Class D Control(3)
    // bit[5:3]          bit[2:0]
    // DCGAIN[2:0]=000b, ACGAIN[2:0]=000b
    51, 0x80 | (0x00<<3) | (0x00<<0),  //speaker gain: DCGAIN,ACGAIN

    // Class D Control(1)
    // bit[7:6]
    // SPK_OP_EN[1:0]=11b
    49, 0x0037 | (0x03<<6),     // enable left & right speaker

    // LOUT2 volume
    // bit[8]   bit[7]   bit[6:0]
    // SPKVU=1, SPKLZC=0, SPKLVOL[6:0]=0x7F
    40, (0x01<<8) | 0x0079,
    // ROUT2 volume
    // bit[8]   bit[7]   bit[6:0]
    // SPKVU=1, SPKLZC=0, SPKLVOL[6:0]=0x7F
    41, (0x01<<8) | 0x0079,

    // Input boost mixer (1)
    // bit[6:4]             bit[3:1]
    // LIN3BOOST[2:0]=000b, LIN2BOOST[2:0]=101b
    43, (0x0000<<4) | (0x0005<<1),      //enable Left Line in2, disable left line in3
    // Input boost mixer (2)
    // bit[6:4]             bit[3:1]
    // RIN3BOOST[2:0]=000b, RIN2BOOST[2:0]=101b
    44, (0x0000<<4) | (0x0005<<1),      //enable Right Line in2, diable right line in3

    // Audio Interface
    // bit8       bit7       bit6   bit5      bit4   bit[3:2]     bit[1:0]
    // ALRSWAP=0, BCLKINV=0, MS=0, DLRSWAP=0, LRP=0, WL[1:0]=00b, FORMAT[1:0]=10b
    7, (0x00<<2) | (0x02<<0),   // I2S mode, slave mode, 16bit
    // 7, (0x02<<2) | (0x02<<0),   // I2S mode, slave mode, 24bit

    // Clocking
    // bit[8:6]       bit[5:3]       bit[2:1]            bit0
    // ADCDIV[2:0]=0, DACDIV[2:0]=0, SYSCLKDIV[1:0]=10b, CLKSEL=1
    4, (0x02<<1) | (0x01<<0),   //sample rate: 48K，SYSCLKDIV=2，enable PLL

    // set PLL, MCLK=24MHz, SYSCLK=12.288MHz: 0x3126E8
    // bit[8:6]         bit5   bit4          bit[3:0]
    // OPCLKDIV[2:0]=0, SDM=1, PLLRESCALE=1, PLLN[3:0]=8
    52, (0x01<<5) | (0x01<<4) | (0x08<<0),
    // Reg53: PLLK[23:16]
    53, (0x3126E8 >> 16) & 0xFF,
    // Reg54: PLLK[15:8]
    54, (0x3126E8 >> 8) & 0xFF,
    // Reg55: PLLK[7:0]
    55, (0x3126E8 & 0xFF),

    // Additional control(1)
    // bit[8]   bit[7:6]       bit[4]      bit[3:2]          bit1       bit0
    // TSDEN=1, VSEL[1:0]=11b, DMONOMIX=0, DATSEL[1:0]=00b, TOCLKSEL=0, TOEN=1
    23, (0x01<<8) | (0x03<<6) | (0x00<<4) | (0x00<<2) | (0x00<<1) | (0x01<<0),

    // Headphone insert detection
    // Additional control (2)
    // bit[6]    bit[5]     bit[3]  bit[2]
    // HPSWEN=1, HPSWPOL=0, TRIS=0, LRCM=0
    24, (0x01<<6) | (0x00<<5) | (0x00<<3) | (0x00<<2),

    // Additional Control (4)
    // bit[7]     bit[6:4]           bit[3:2]        bit[1]     bit[0]
    // GPIOPOL=0, GPIOSEL[2:0]=000b, HPSEL[1:0]=11b, TSENSEN=1, MBSEL=0
    48, (0x03<<2) | (0x01<<1) | (0x00<<0),  // MBSEL=0: Bias voltag->0.9*AVDD
};


uint16_t wm8960_mic_cfg[] = {
    // addr, data
    15, 0x0000,     // reset

    //Audio Interface
    // bit[7:8]     bit[6]     bit[5]      bit[4:3]        bit[2:1]     bit[0]
    // 00,        ALRCGPIO=1, WL8=0,       DACCOMP=00,   ADCCOMP=00, LOOPBACK=0
    9, (0x1 << 6),

    // power management1
    // bit[8:7]          bit6    bit5    bit4    bit3    bit2    bit1    bit0
    // VMIDSEL[1:0]=01b, VREF=1, AINL=1, AINR=1, ADCL=1, ADCR=1, MICB=1, DIGENB=0
    25, (0x01 << 7) | (0x01 << 6) | (0x01 << 5) | (0x01 << 4) | (0x01 << 3) | (0x01 << 2) | (0x01 << 1) | 0x00,

    // power management2
    // bit8    bit7    bit6     bit5     bit4    bit3    bit1    bit0
    // DACL=1, DACR=1, LOUT1=1, ROUT1=1, SPKL=1, SPKR=1, OUT3=0, PLL_EN=1
    26, (0x01<<8) | (0x01<<7) | (0x01<<6) | (0x01<<5) | (0x01<<4) | (0x01<<3) | (0x00<<1) | (0x01<<0),

    // power management3 
    // bit5    bit4     bit3    bit2
    // LMIC=1, RMIC=1, LOMIX=1, ROMIX=1
    47, (0x01<<5) | (0x01<<4) | (0x01<<3) | (0x01<<2),

    // ADCL signal path
    // bit[8]  bit[7]  bit[6]   bit[5:4]           bit3
    // LMN1=1, LMP3=0, LMP2=0, LMICBOOST[1:0]=00b, LMIC2B=1
    32, (0x01<<8) | (0x00<<7) | (0x00<<6) | (0x00<<4) | (0x01<<3),      

    // ADCR signal path
    // bit[8]  bit[7]  bit[6]   bit[5:4]           bit3
    // RMN1=1, RMP3=0, RMP2=0, RMICBOOST[1:0]=00b, RMIC2B=1
    33, (0x01<<8) | (0x00<<7) | (0x00<<6) | (0x00<<4) | (0x01<<3),   

    // left input volume
    // bit8     bit7      bit6,   bit[5:0]
    // IPVU=1, LINMUTE=0, LIZC=0, LINVOL[5:0]=0x1F
    0, (0x01<<8) | (0x00<<7) | (0x1F<<0),   

    // right input volume
    // bit8     bit7      bit6,   bit[5:0]
    // IPVU=1, RINMUTE=0, RIZC=0, RINVOL[5:0]=0x1F
    1, (0x01<<8) | (0x00<<7) | (0x1F<<0),   

    // automatic level control
    17, (0x02<<7) | (0x07<<4) | (0x0B<<0),
    18, (0x00<<4) | (0x00<<0),
    19, (0x00<<8) | (0x03<<4) | (0x02<<0),

    // left out mix: choose one of three
    // bit[8]   bit[7]   bit[6:4]
    // LD2LO=1, LI2LO=0, LI2LOVOL[2:0]=000b
    34, (0x01<<8),      // (1)enable Left DAC To Left output Mixer, LI2LO=0
    // LD2LO=0, LI2LO=1, LI2LOVOL[2:0]=000b
    // 34, (0x01<<7),      // (2)enable LINPUT3 To Left output Mixer
    // 34, 0,              // (3)enable Left Input Boost Mixer Output To Left output Mixer
    // bit7     bit[6:4]
    // LB2LO=1, LB2LOVOL[2:0]=000b
    // 45, (0x01<<7),      // (3)enable Left Input Boost Mixer Output To Left output Mixer

    // right out mix: choose one of two
    // bit[8]   bit[7]   bit[6:4]
    // RD2Ro=1, RI2RO=0, RI2ROVOL[2:0]=000b
    37, (0x01<<8),   // (1)enable Right DAC To Right output Mixer
    // RD2RO=0, RI2RO=0, RI2ROVOL[2:0]=000b
    // 37, 0,              // (2)enable Right Input Boost Mixer Output To Right output Mixer
    // bit7     bit[6:4]
    // RB2RO=1, RB2ROVOL[2:0]=000b
    // 46, (0x01<<7),      // (2)enable Right Input Boost Mixer Output To Right output Mixer

    // ADC & DAC control
    // bit7      bit[6:5]     bit3     bit[2:1]       bit0
    //DACDIV2=0, ADCPOL[1:0], DACMU=0, DEEMPH[1:0]=0, ADCPHD=0
    5, 0x0000,          // DAC unmute, disable high pass filter on left and right channels

    // LOUT1 volume
    // bit8      bit7     bit6~0
    // OUT1VU=1, LO1ZC=0, LOUT1VOL[6:0]=0x79->0dB
    2, (0x01<<8) | (0x00<<7) | 0x79,

    // ROUT1 volume
    // bit8      bit7     bit6~0
    // OUT1VU=1, LO1ZC=0, LOUT1VOL[6:0]=0x79->0dB
    3, (0x01<<8) | (0x00<<7) | 0x79,

    // Class D Control(3)
    // bit[5:3]          bit[2:0]
    // DCGAIN[2:0]=000b, ACGAIN[2:0]=000b
    51, 0x80 | (0x00<<3) | (0x00<<0),  //speaker gain: DCGAIN,ACGAIN

    // Class D Control(1)
    // bit[7:6]
    // SPK_OP_EN[1:0]=11b
    49, 0x0037 | (0x03<<6),     // enable left & right speaker

    // LOUT2 volume
    // bit[8]   bit[7]   bit[6:0]
    // SPKVU=1, SPKLZC=0, SPKLVOL[6:0]=0x79
    40, (0x01<<8) | 0x0079,
    // ROUT2 volume
    // bit[8]   bit[7]   bit[6:0]
    // SPKVU=1, SPKLZC=0, SPKLVOL[6:0]=0x79
    41, (0x01<<8) | 0x0079,

    // Input boost mixer (1)
    // bit[6:4]             bit[3:1]
    // LIN3BOOST[2:0]=000b, LIN2BOOST[2:0]=000b
    43, (0x0000<<4) | (0x0000<<1),      //disable Left Line in2/3
    // Input boost mixer (2)
    // bit[6:4]             bit[3:1]
    // RIN3BOOST[2:0]=000b, RIN2BOOST[2:0]=000b
    44, (0x0000<<4) | (0x0000<<1),      //disable Right Line in2/3

    // Audio Interface
    // bit8       bit7       bit6   bit5      bit4   bit[3:2]     bit[1:0]
    // ALRSWAP=0, BCLKINV=0, MS=0, DLRSWAP=0, LRP=0, WL[1:0]=10b, FORMAT[1:0]=10b
    7, (0x00<<2) | (0x02<<0),   // I2S mode, slave mode, 16bit
    // 7, (0x02<<2) | (0x02<<0),   // I2S mode, slave mode, 24bit

    // Clocking
    // bit[8:6]       bit[5:3]       bit[2:1]            bit0
    // ADCDIV[2:0]=0, DACDIV[2:0]=0, SYSCLKDIV[1:0]=10b, CLKSEL=1
    4, (0x02<<1) | (0x01<<0),   //sample rate: 48K，SYSCLKDIV=2，enable PLL

    // set PLL, MCLK=24MHz, SYSCLK=12.288MHz: 0x3126E8
    // bit[8:6]         bit5   bit4          bit[3:0]
    // OPCLKDIV[2:0]=0, SDM=1, PLLRESCALE=1, PLLN[3:0]=8
    52, (0x01<<5) | (0x01<<4) | (0x08<<0),
    // Reg53: PLLK[23:16]
    53, (0x3126E8 >> 16) & 0xFF,
    // Reg54: PLLK[15:8]
    54, (0x3126E8 >> 8) & 0xFF,
    // Reg55: PLLK[7:0]
    55, (0x3126E8 & 0xFF),

    // Additional control(1)
    // bit[8]   bit[7:6]       bit[4]      bit[3:2]          bit1       bit0
    // TSDEN=1, VSEL[1:0]=11b, DMONOMIX=0, DATSEL[1:0]=00b, TOCLKSEL=0, TOEN=1
    23, (0x01<<8) | (0x03<<6) | (0x00<<4) | (0x00<<2) | (0x00<<1) | (0x01<<0),

    // HP insert detection
    // Additional control (2)
    // bit[6]    bit[5]     bit[3]  bit[2]
    // HPSWEN=1, HPSWPOL=0, TRIS=0, LRCM=0
    24, (0x01<<6) | (0x00<<5) | (0x00<<3) | (0x00<<2),

    // Additional Control (4)
    // bit[7]     bit[6:4]           bit[3:2]        bit[1]     bit[0]
    // GPIOPOL=0, GPIOSEL[2:0]=000b, HPSEL[1:0]=11b, TSENSEN=1, MBSEL=0
    48, (0x03<<2) | (0x01<<1) | (0x00<<0),  // MBSEL=0: Bias voltag->0.9*AVDD
};
/* USER CODE END 0 */


void write_wm8960_reg(uint8_t reg_addr, uint16_t reg_data)
{
    uint8_t I2C_Data[2];

    I2C_Data[0] = (reg_addr << 1) | ((reg_data >> 8) & 0x0001);
    I2C_Data[1] = reg_data & 0x00FF;

    if (HAL_I2C_Master_Transmit(&hi2c1, IIC_SLAVE_ADDR, I2C_Data, 2, HAL_MAX_DELAY) != HAL_OK)
    {
        // while(1);
    }
    wm8960_shadow_reg[reg_addr] = reg_data;
}

void init_wm8960(eWm8960InputSrc src)
{
    audio_src = src;

    if (audio_src == WM8960_INPUT_LINEIN)
    {
        write_wm8960_reg(wm8960_line_cfg[0], wm8960_line_cfg[1]);
        HAL_Delay(10);  // delay 10ms

        // default work mode: Line In
        for (uint32_t i = 1; i < sizeof(wm8960_line_cfg)/(2 * sizeof(uint16_t)); i++)
        {
            write_wm8960_reg(wm8960_line_cfg[2 * i], wm8960_line_cfg[2 * i + 1]);
        }
    }
    else 
    {
        write_wm8960_reg(wm8960_mic_cfg[0], wm8960_mic_cfg[1]);
        HAL_Delay(10);  // delay 10ms

        // default work mode: Line In
        for (uint32_t i = 1; i < sizeof(wm8960_mic_cfg)/(2 * sizeof(uint16_t)); i++)
        {
            write_wm8960_reg(wm8960_mic_cfg[2 * i], wm8960_mic_cfg[2 * i + 1]);
        }
    }
}

void set_wm8960_input_src(eWm8960InputSrc src)
{
    audio_src = src;

    if (audio_src == WM8960_INPUT_LINEIN)
    {
        // power management1
        // bit[8:7]          bit6    bit5    bit4    bit3    bit2    bit1    bit0
        // VMIDSEL[1:0]=10b, VREF=1, AINL=1, AINR=1, ADCL=1, ADCR=1, MICB=0, DIGENB=0
        write_wm8960_reg(25, 
            (0x01 << 7) | (0x01 << 6) | (0x01 << 5) | (0x01 << 4) | (0x01 << 3) | (0x01 << 2) | (0x00 << 1) | 0x00);

        // power management3 
        // bit5    bit4     bit3    bit2
        // LMIC=0, RMIC=0, LOMIX=1, ROMIX=1
        write_wm8960_reg(47, (0x00<<5) | (0x00<<4) | (0x01<<3) | (0x01<<2));    // disable MIC input

        // ADCL signal path
        // bit[8]  bit[7]  bit[6]   bit[5:4]           bit3
        // LMN1=0, LMP3=0, LMP2=0, LMICBOOST[1:0]=00b, LMIC2B=0
        write_wm8960_reg(32, (0x00<<8) | (0x00<<7) | (0x00<<6) | (0x00<<4) | (0x00<<3));

        // ADCR signal path
        // bit[8]  bit[7]  bit[6]   bit[5:4]           bit3
        // RMN1=0, RMP3=0, RMP2=0, RMICBOOST[1:0]=00b, RMIC2B=0
        write_wm8960_reg(33, (0x00<<8) | (0x00<<7) | (0x00<<6) | (0x00<<4) | (0x00<<3));

        // Input boost mixer (1)
        // bit[6:4]             bit[3:1]
        // LIN3BOOST[2:0]=000b, LIN2BOOST[2:0]=101b
        write_wm8960_reg(43, (0x0000<<4) | (0x0005<<1));      //enable Left Line in2, disable left line in3
        // Input boost mixer (2)
        // bit[6:4]             bit[3:1]
        // RIN3BOOST[2:0]=000b, RIN2BOOST[2:0]=101b
        write_wm8960_reg(44, (0x0000<<4) | (0x0005<<1));      //enable Right Line in2, diable right line in3
    }
    else
    {
        // power management1
        // bit[8:7]          bit6    bit5    bit4    bit3    bit2    bit1    bit0
        // VMIDSEL[1:0]=01b, VREF=1, AINL=1, AINR=1, ADCL=1, ADCR=1, MICB=1, DIGENB=0
        write_wm8960_reg(25, 
            (0x01 << 7) | (0x01 << 6) | (0x01 << 5) | (0x01 << 4) | (0x01 << 3) | (0x01 << 2) | (0x01 << 1) | 0x00);

        // power management3 
        // bit5    bit4     bit3    bit2
        // LMIC=1, RMIC=1, LOMIX=1, ROMIX=1
        write_wm8960_reg(47, (0x01<<5) | (0x01<<4) | (0x01<<3) | (0x01<<2));

        // ADCL signal path
        // bit[8]  bit[7]  bit[6]   bit[5:4]           bit3
        // LMN1=1, LMP3=0, LMP2=0, LMICBOOST[1:0]=00b, LMIC2B=1
        write_wm8960_reg(32, (0x01<<8) | (0x00<<7) | (0x00<<6) | (0x00<<4) | (0x01<<3));

        // ADCR signal path
        // bit[8]  bit[7]  bit[6]   bit[5:4]           bit3
        // RMN1=1, RMP3=0, RMP2=0, RMICBOOST[1:0]=00b, RMIC2B=1
        write_wm8960_reg(33, (0x01<<8) | (0x00<<7) | (0x00<<6) | (0x00<<4) | (0x01<<3));

        // Input boost mixer (1)
        // bit[6:4]             bit[3:1]
        // LIN3BOOST[2:0]=000b, LIN2BOOST[2:0]=000b
        write_wm8960_reg(43, (0x0000<<4) | (0x0000<<1));      //disable Left Line in2/3
        // Input boost mixer (2)
        // bit[6:4]             bit[3:1]
        // RIN3BOOST[2:0]=000b, RIN2BOOST[2:0]=101b
        write_wm8960_reg(44, (0x0000<<4) | (0x0000<<1));      //disable Right Line in2/3
    }
}

// vol_db: -74 ~ +6 dB
// when vol_db < -73, mute speaker output
void set_hp_vol(int32_t vol_db)
{
    if (vol_db < -73)
    {
        vol_db = -73;
    }
    else if (vol_db > 6)
    {
        vol_db = 6;
    }
    vol_db += 73 + 0x30;

    write_wm8960_reg(2, (0x01<<8) | (0x00<<7) | vol_db);
    write_wm8960_reg(3, (0x01<<8) | (0x00<<7) | vol_db);
}

// vol_db: -74 ~ +6 dB
// when vol_db < -73, mute speaker output
void set_spk_vol(int32_t vol_db)
{
    uint8_t mute = 0;
    if (vol_db < -73)
    {
        mute = 1;
    }
    else if (vol_db > 6)
    {
        vol_db = 6;
    }
    vol_db += 73 + 0x30;

    if (mute == 1)
    {
        write_wm8960_reg(49, 0x0037 | (0x00<<6));     // disable left & right speaker
    }
    else
    {
        write_wm8960_reg(49, 0x0037 | (0x03<<6));     // enable left & right speaker
    }

    // LOUT2 volume
    // bit[8]   bit[7]   bit[6:0]
    // SPKVU=1, SPKLZC=0, SPKLVOL[6:0]
    write_wm8960_reg(40, (0x01<<8) | (0x00<<7) | vol_db);

    // ROUT2 volume
    // bit[8]   bit[7]   bit[6:0]
    // SPKVU=1, SPKLZC=0, SPKLVOL[6:0]=0x79
    write_wm8960_reg(41, (0x01<<8) | (0x00<<7) | vol_db);
}

void set_adclrc_gpio(uint8_t state)
{
    uint16_t val = wm8960_shadow_reg[48];
    if (state == 0)
    {
        // Set GPIO low
        val = val & (~(0x0007 << 4)) | (0x06 << 4);
        write_wm8960_reg(48, val);
    }
    else
    {
        // Set GPIO high
        val = val & (~(0x0007 << 4)) | (0x07 << 4);
        write_wm8960_reg(48, val);
    }
    wm8960_shadow_reg[48] = val;
}
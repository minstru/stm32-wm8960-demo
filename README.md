# STM32F429 WM8960 Audio Project

## Project Introduction

This project is based on the STM32F429 microcontroller and implements control of the WM8960 audio codec. The project uses the IAR Embedded Workbench development environment and provides complete audio capture and playback functionality.

For detailed documentation, please visit: [www.minstru.com/modules/wm8960/](https://www.minstru.com/modules/wm8960).

## Features

- ✅ STM32F429 communicates with WM8960 via I2S interface
- ✅ Supports audio capture (microphone input)
- ✅ Supports audio playback (headphone/speaker output)
- ✅ I2C configuration of WM8960 registers
- ✅ Interrupt/DMA mode for audio data processing
- ✅ Configurable sample rates and audio formats

## Hardware Requirements

### Main Controller
- STM32F429ZIT6 microcontroller
- Supports ARM Cortex-M4 core

### Audio Chip
- WM8960 audio codec
- Supports 24-bit audio data
- Sample rate range: 8kHz - 48kHz

### Connection Method

- I2S working in Master transmit mode

```
STM32F429             <->    WM8960
    NC                 ->   MCLK
PB10(I2S2_CK)          ->   BCLK
PB12(I2S2_WS)          ->   DACLRC
PC3(I2S2_SD)           ->   DACDAT
PC2(I2S2_ext_SD)       <-   ADCDAT
    NC                 ->   ADCLRC
PB6(I2C1_SCL)          ->   SCL
PB9(I2C1_SDA)          <->  SDA
PB7(LED)
PC13(Button)
```

- I2S working in Master receive mode

```
STM32F429             <->    WM8960
    NC                 ->   MCLK
PB10(I2S2_CK)          ->   BCLK
PB12(I2S2_WS)          ->   DACLRC
PC3(I2S2_SD)           ->   ADCDAT	// Different connection from transmit mode
PC2(I2S2_ext_SD)       <-   DACDAT  // Different connection from transmit mode
    NC                 ->   ADCLRC
PB6(I2C1_SCL)          ->   SCL
PB9(I2C1_SDA)          <->  SDA
PB7(LED)
PC13(Button)
```

## Software Structure

```
Project/
├── Core/
|   ├── Src/
|   │   ├── main.c              # Main program
|   │   ├── stm32f4xx_it.c      # Interrupt service routines
|   │   └── dma.c     			# DMA configuration
|   │   └── i2c.c     			# I2C interface configuration
|   │   └── i2s.c     			# I2S interface configuration
|   │   ├── wm8960.c            # WM8960 driver
|   │   ├── audio_tx.c       	# Play audio by enabling I2S transmit DMA
|   │   ├── audio_tx_rx.c       # Loopback audio by enabling I2S transmit and receive DMA
|   │   ├── freertos.c      	# Audio task
|   ├── Inc/            		# Header file directory
├── Drivers/
|   ├── CMSIS/          		# Cortex-M4 core support package
|   ├── STM32F4xx_HAL_Driver/   # STM32F4 HAL library
├── Middlewares/    			# Middleware (optional)
└── EWARM/          			# IAR project files
|__ F429ZIT.ioc					
```

## Development Environment Configuration

### Required Software
- IAR Embedded Workbench for ARM 8.x or higher
- STM32CubeF4 HAL library (included in the project)

### Project Setup
1. Open IAR Embedded Workbench
2. Select `File → Open Workspace`
3. Navigate to the `EWARM/Project.eww` file
4. Confirm the following configurations:
   - Device: STM32F429ZITx
   - Debugger: ST-LINK (or other corresponding debugger)

## Usage Instructions

### Audio Driver
```c
// Initialize WM8960 chip: select LineIN or MIC as input source
void init_wm8960(eWm8960InputSrc src);

// Dynamically switch input source
void set_wm8960_input_src(eWm8960InputSrc src);

// Set LineOut volume
void set_hp_vol(int32_t vol_db);

// Set Speaker volume
void set_spk_vol(int32_t vol_db);

// Set ADCLRC pin output state when used as GPIO
void set_adclrc_gpio(uint8_t state);
```

### Loopback Mode

Use HAL_I2SEx_TransmitReceive_DMA() function to simultaneously transmit and receive audio. The HAL_I2SEx_TxRxCpltCallback() callback is called upon completion.

```c
// Provide two data buffers as receive buffer and transmit buffer, switch buffers in the transmit/receive completion callback
void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (stop_dma != 0)
    {
        tx_cplt = 1;
        return;
    }
    if (ping_pong == 0)
    {
        HAL_I2SEx_TransmitReceive_DMA(hi2s, pong_buf, ping_buf, BufSize);
        ping_pong = 1;
    }
    else if (ping_pong == 1)
    {
        HAL_I2SEx_TransmitReceive_DMA(hi2s, ping_buf, pong_buf, BufSize);
        ping_pong = 0;
    }
}
```

### Transmit Mode

Use HAL_I2S_Transmit_DMA() function to transmit audio data. The HAL_I2S_TxCpltCallback() function is called upon transmission completion. Continuously calling HAL_I2S_Transmit_DMA() within HAL_I2S_TxCpltCallback() enables continuous audio data transmission.

```c
// Continuously transmit new audio data in the transmission completion callback until all audio data is sent
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (stop_dma != 0)
    {
        tx_cplt = 1;
        return;
    }

    if (audio_rem_size > 0)
    {
        uint32_t dma_len = (uint32_t)(DMA_MAX(audio_rem_size));

        /* Update the Media layer and enable it for play */  
        HAL_I2S_Transmit_DMA(hi2s, wav_data_new_pos, dma_len);

        /* Update the remaining number of data to be played */
        audio_rem_size = audio_rem_size - dma_len;

        /* Update the current audio pointer position */
        wav_data_new_pos = wav_data_new_pos + dma_len;
    }
    else
    {
        tx_cplt = 1; 
    }
}
```

### Receive Mode

Use HAL_I2S_Receive_DMA() function to receive audio data. The HAL_I2S_RxCpltCallback() function is called upon reception completion. Continuously calling HAL_I2S_Receive_DMA() within HAL_I2S_RxCpltCallback() enables continuous audio data reception.

### Audio Input Source Switching

Use PC13 (Button) to repeatedly switch input sources, with the corresponding PB7 LED indicating the current status.

```c
if (button_pressed) 
{
    osDelay(1000);
    button_pressed = 0;
    stop_dma = 1;

    // wait for dma transmission complete
    while(tx_cplt == 0)
    {
        osDelay(1);
    }

    stop_dma = 0;
    osDelay(100);

    if (audio_src == WM8960_INPUT_LINEIN)
    {
        // Switch to microphone input
        set_wm8960_input_src(WM8960_INPUT_MIC);
    }
    else 
    {
        // Switch to line input
        set_wm8960_input_src(WM8960_INPUT_LINEIN);
    }

    osDelay(100);

    // test_audio_tx();
    test_audio_tx_rx();

    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    count = 0;
}
```

## Frequently Asked Questions

### Q: Unable to detect WM8960 device
- Check I2C line connections
- Use set_adclrc_gpio() method to control the LED connected to ADLRC to verify I2C operation success

### Q: Audio data has noise
- Confirm MCLK, BCLK, LRCLK clock configurations are correct
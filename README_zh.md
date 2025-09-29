# STM32F429 WM8960 Audio Project

## 项目简介

本项目基于STM32F429微控制器，实现了对WM8960音频编解码器的控制。项目使用IAR Embedded Workbench开发环境，提供了完整的音频采集和播放功能。

音频模块的详细资料，请访问：[www.minstru.com/modules/wm8960/](https://www.minstru.com/modules/wm8960)。

## 功能特性

- ✅ STM32F429与WM8960通过I2S接口通信
- ✅ 支持音频采集（麦克风输入）
- ✅ 支持音频播放（耳机/扬声器输出）
- ✅ I2C配置WM8960寄存器
- ✅ 中断/DMA方式处理音频数据
- ✅ 可配置的采样率和音频格式

## 硬件要求

### 主控制器
- STM32F429ZIT6微控制器
- 支持ARM Cortex-M4内核

### 音频芯片
- WM8960音频编解码器
- 支持24-bit音频数据
- 采样率范围：8kHz - 48kHz

### 连接方式

- I2S工作在Master发送模式

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

- I2S工作在Master接收模式

```
STM32F429             <->    WM8960
    NC                 ->   MCLK
PB10(I2S2_CK)          ->   BCLK
PB12(I2S2_WS)          ->   DACLRC
PC3(I2S2_SD)           ->   ADCDAT	// 和发送模式下的连接不同
PC2(I2S2_ext_SD)       <-   DACDAT  // 和发送模式下的连接不同
    NC                 ->   ADCLRC
PB6(I2C1_SCL)          ->   SCL
PB9(I2C1_SDA)          <->  SDA
PB7(LED)
PC13(Button)
```

## 软件结构

```
Project/
├── Core/
|   ├── Src/
|   │   ├── main.c              # 主程序
|   │   ├── stm32f4xx_it.c      # 中断服务程序
|   │   └── dma.c     			# dma配置
|   │   └── i2c.c     			# I2C接口配置
|   │   └── i2s.c     			# I2S接口配置
|   │   ├── wm8960.c            # WM8960驱动
|   │   ├── audio_tx.c       	# 通过使能I2S的发送DMA播放音频
|   │   ├── audio_tx_rx.c       # 通过使能I2S的发送和接收DMA回环音频
|   │   ├── freertos.c      	# audio task
|   ├── Inc/            		# 头文件目录
├── Drivers/
|   ├── CMSIS/          		# Cortex-M4内核支持包
|   ├── STM32F4xx_HAL_Driver/   # STM32F4 HAL库
├── Middlewares/    			# 中间件（可选）
└── EWARM/          			# IAR工程文件
|__ F429ZIT.ioc					
```

## 开发环境配置

### 必需软件
- IAR Embedded Workbench for ARM 8.x或更高版本
- STM32CubeF4 HAL库（已包含在项目中）

### 工程设置
1. 打开IAR Embedded Workbench
2. 选择 `File → Open Workspace`
3. 导航到 `EWARM/Project.eww` 文件
4. 确认以下配置：
   - Device: STM32F429ZITx
   - Debugger: ST-LINK（或其他对应调试器）

## 使用说明

### 音频驱动
```c
// 初始化WM8960芯片:选择用LineIN还是MIC作为输入源
void init_wm8960(eWm8960InputSrc src);

// 动态切换输入源
void set_wm8960_input_src(eWm8960InputSrc src);

// 设置LineOut的音量
void set_hp_vol(int32_t vol_db);

// 设置Speaker的音量
void set_spk_vol(int32_t vol_db);

// 设置ADCLRC引脚作为GPIO时的输出状态
void set_adclrc_gpio(uint8_t state);
```

### 回环模式

通过HAL_I2SEx_TransmitReceive_DMA()函数实现同时发送音频和接收音频，在完成时会调用HAL_I2SEx_TxRxCpltCallback()回调。

```c
// 提供两个数据buf作为接收buf和发送buf，在发送接收完成回调中切换buf
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

### 发送模式

通过HAL_I2S_Transmit_DMA()函数发送音频数据，在发送完成时会调用HAL_I2S_TxCpltCallback()函数。在HAL_I2S_TxCpltCallback()中继续调用HAL_I2S_Transmit_DMA()就可以实现不断的发送音频数据了。

```c
// 在发送完成回调中不断发送新的音频数据，直到音频数据发送完
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

### 接收模式

通过HAL_I2S_Receive_DMA()函数接收音频数据，在接收完成时会调用HAL_I2S_RxCpltCallback()函数。在HAL_I2S_RxCpltCallback()中继续调用HAL_I2S_Receive_DMA()就可以实现连续接收音频数据了。

### 音频输入源切换

通过PC13(Button)可以反复切换输入源，并通过PB7对应的LED指示当前状态。

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

## 常见问题

### Q: 无法检测到WM8960设备
- 检查I2C线路连接
- 通过set_adclrc_gpio()方法控制ADLRC所连接的LED，验证I2C操作是否成功

### Q: 音频数据有噪声
- 确认MCLK、BCLK、LRCLK时钟配置正确
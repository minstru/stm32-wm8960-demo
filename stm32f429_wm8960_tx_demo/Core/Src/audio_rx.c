/*
 * Copyright (c) 2025-present Mind Instrument
 * Distributed under the MIT software license, see the accompanying file LICENSE.txt.
 * For those auto-generated code, please refer to the original author license.
 */

#include <stdio.h>
#include <math.h>
#include "main.h"
#include "cmsis_os.h"
#include "dma.h"
#include "i2s.h"


#define BufferSize (20000)

uint16_t input_buf[BufferSize];
uint8_t rx_cplt = 0;

void test_audio_rx()
{
    rx_cplt = 0;
    HAL_I2S_Receive_DMA(&hi2s2, input_buf, BufferSize);
}


void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    rx_cplt = 1;
}

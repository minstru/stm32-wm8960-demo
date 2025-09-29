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


#define BufSize  (2 * 48000 / 5)

uint16_t ping_buf[BufSize];
uint16_t pong_buf[BufSize];
uint32_t ping_pong = 0;

extern uint8_t stop_dma;
extern uint8_t tx_cplt;


void test_audio_tx_rx()
{
    tx_cplt = 0;
    ping_pong = 0;

    for(uint16_t i = 0; i < BufSize; i += 2) {
        ping_buf[i] = 0;
        ping_buf[i + 1] = 0;

        pong_buf[i]     = sin(2*3.14*(i/2)*500/48000) * 1024;
        pong_buf[i + 1] = sin(2*3.14*(i/2)*500/48000) * 1024;
    }

    if (ping_pong == 0)
    {
        HAL_I2SEx_TransmitReceive_DMA(&hi2s2, pong_buf, ping_buf, BufSize);
        ping_pong = 1;
    }
}


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

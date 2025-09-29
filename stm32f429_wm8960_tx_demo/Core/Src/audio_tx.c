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
#include "wave_data.h"
#include "wm8960.h"

#define DMA_MAX_SZE     32000
#define DMA_MAX(x)      (((x) <= DMA_MAX_SZE)? (x):DMA_MAX_SZE)

uint32_t audio_total_size;  /* This variable holds the total size of the audio file */
int32_t audio_rem_size;     /* This variable holds the remaining data in audio file */
uint16_t *wav_data_new_pos;  /* This variable holds the current position of audio pointer */

extern uint8_t stop_dma;
extern uint8_t tx_cplt;


void test_audio_tx()
{
    set_hp_vol(-20);
    
    /* Set the total number of data to be played (count in half-word) */
    audio_total_size = sizeof(WaveData) / 2;

    tx_cplt = 0;

    uint32_t dma_len = (uint32_t)(DMA_MAX(audio_total_size));

    /* Update the Media layer and enable it for play */
    HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t *)WaveData, dma_len);

    /* Update the remaining number of data to be played */
    audio_rem_size = audio_total_size - dma_len;

    /* Update the current audio pointer position */
    wav_data_new_pos = (uint16_t *)WaveData + dma_len;
}


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

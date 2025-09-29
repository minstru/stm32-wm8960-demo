/*
 * Copyright (c) 2025-present Mind Instrument
 * Distributed under the MIT software license, see the accompanying file LICENSE.txt.
 * For those auto-generated code, please refer to the original author license.
 */

#ifndef __WM8960_H__
#define __WM8960_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "cmsis_os.h"

typedef enum {
    WM8960_INPUT_LINEIN,
    WM8960_INPUT_MIC
} eWm8960InputSrc;

extern eWm8960InputSrc audio_src;

void init_wm8960(eWm8960InputSrc src);
void set_wm8960_input_src(eWm8960InputSrc src);
void set_hp_vol(int32_t vol_db);
void set_spk_vol(int32_t vol_db);


#ifdef __cplusplus
}
#endif

#endif /* __WM8960_H__ */
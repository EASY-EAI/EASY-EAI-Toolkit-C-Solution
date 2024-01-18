/**
 *
 * Copyright 2023 by Guangzhou Easy EAI Technologny Co.,Ltd.
 * website: www.easy-eai.com
 *
 * Author: Jiehao.Zhong <zhongjiehao@easy-eai.com>
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * License file for more details.
 * 
 */

#ifndef __WAVINFO_OPT_H__
#define __WAVINFO_OPT_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "wav_info.h"

#include <alsa/asoundlib.h>

extern wav_info *create_wavInfo(uint32_t sampleRate, uint16_t channels, snd_pcm_format_t fmt, int32_t recTimes/*秒*/);
extern snd_pcm_format_t get_pcmFormat(wav_info *pWavInfo);
extern int32_t destory_wavInfo(wav_info *pWavInfo);

#ifdef __cplusplus
}
#endif


#endif

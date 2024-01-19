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

#include "wavInfo_opt.h"

// 根据[常见]采样格式转换为WAV文件格式
static uint16_t wavFmt(snd_pcm_format_t fmt)
{
    uint16_t wavFmt;
    switch (fmt){
        case SND_PCM_FORMAT_UNKNOWN:
            wavFmt = WAV_FMT_UNKNOW;  break;
        case SND_PCM_FORMAT_S8:
        case SND_PCM_FORMAT_U8:
        case SND_PCM_FORMAT_S16:
        case SND_PCM_FORMAT_U16:
        case SND_PCM_FORMAT_S24:
        case SND_PCM_FORMAT_U24:
        case SND_PCM_FORMAT_S32:
        case SND_PCM_FORMAT_U32:
            wavFmt = WAV_FMT_PCM; break;
        case SND_PCM_FORMAT_FLOAT:
        case SND_PCM_FORMAT_FLOAT64:
            wavFmt = WAV_FMT_FLOAT; break;
        default: 
            wavFmt = WAV_FMT_UNKNOW; break;
    }
    return wavFmt;
}

wav_info *create_wavInfo(uint32_t sampleRate, uint16_t channels, snd_pcm_format_t fmt, int32_t recTimes)
{
    wav_info *pWavIno = calloc(1, sizeof(wav_info));
    if(NULL == pWavIno){
        printf("create wavInfo faild!\n");
        return NULL;
    }
    
    size_t bitDepth = snd_pcm_format_physical_width(fmt);
    uint16_t frameSize = channels * (bitDepth>>3); //一帧音频数据所占的内存大小
    uint32_t pcm1sSize = sampleRate * frameSize;   //一秒音频数据所占的内存大小
    pWavIno->format.fmt_id   = FMT;
    pWavIno->format.fmt_size = LE_INT(16);
    pWavIno->format.fmt      = LE_SHORT(wavFmt(fmt));
    pWavIno->format.channels = LE_SHORT(channels);
    pWavIno->format.sample_rate     = LE_INT(sampleRate);
    pWavIno->format.byte_rate       = LE_INT(pcm1sSize);
    pWavIno->format.block_align     = LE_SHORT(frameSize);
    pWavIno->format.bits_per_sample = LE_SHORT(bitDepth);

    uint32_t dataSize = recTimes * pcm1sSize;
    pWavIno->data.data_id   = DATA;
    pWavIno->data.data_size = LE_INT(dataSize);
    
    pWavIno->head.id     = RIFF;
    pWavIno->head.size   = LE_INT(36 + dataSize);
    pWavIno->head.format = WAVE;

    return pWavIno;
}

snd_pcm_format_t get_pcmFormat(wav_info *pWavInfo)
{
    snd_pcm_format_t snd_format = SND_PCM_FORMAT_UNKNOWN;
    
    if((LE_SHORT(pWavInfo->format.fmt) != WAV_FMT_PCM) &&
       (LE_SHORT(pWavInfo->format.fmt) != WAV_FMT_FLOAT))
        return snd_format;

    switch(LE_SHORT(pWavInfo->format.bits_per_sample))
    {
        case 64:
            snd_format = SND_PCM_FORMAT_FLOAT64; break;
        case 32:    //同是32位还有: SND_PCM_FORMAT_U32、SND_PCM_FORMAT_S32
            snd_format = SND_PCM_FORMAT_FLOAT; break;
        case 24:    //同是24位还有: SND_PCM_FORMAT_U24
            snd_format = SND_PCM_FORMAT_S24; break;
        case 16:    //同是24位还有: SND_PCM_FORMAT_U16
            snd_format = SND_PCM_FORMAT_S16; break;
        case 8:
            snd_format = SND_PCM_FORMAT_U8; break;
        default:
            snd_format = SND_PCM_FORMAT_UNKNOWN; break;
    }
    return snd_format;
}

int32_t destory_wavInfo(wav_info *pWavInfo)
{
    if(pWavInfo){
        free(pWavInfo);
    }else{
        return -1;
    }

    return 0;
}


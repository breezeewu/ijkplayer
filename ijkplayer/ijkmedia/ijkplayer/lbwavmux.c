/*
 * lbwavmuxer.c
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */
#include "lbwavmux.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef lbtrace
#define lbtrace printf
#endif

#ifndef lberror
#define lberror printf
#endif

#pragma   pack(4)
//wav header struct
typedef  struct  lbwav_header{
    
    char        fcc_id[4];
    
    int32_t      dwsize;
    
    char        fcctype[4];
    
} wav_hdr;

typedef  struct  lbwav_fmt{
    
    char        fcc_id[4];
    
    int32_t      dwsize;
    
    int16_t      wformat_tag;
    
    int16_t      wchannels;
    
    int32_t      dwsample_per_sec;
    
    int32_t      dwavg_bytes_per_sec;
    
    int16_t      wblock_align;
    
    int16_t      uibits_per_sample;
    
}FMT;

typedef  struct  lbwav_data{
    
    char        fcc_id[4];
    
    int32_t      dwsize;
    
}DATA;

typedef struct lbwav_mux_context
{
    FILE* pwav_file;
    struct lbwav_header wav_hdr;
    struct lbwav_fmt    wav_fmt;
    struct lbwav_data   wav_data;
} lbwav_mux_ctx;
#pragma pack(pop)
// pcm 转wav



struct lbwav_mux_context* lbwav_open_context(const char* pwav_url, int channel, int samplerate, int bitspersample)
{
    lbtrace("%s(pwav_url:%s, channel:%d, samplerate:%d, bitspersample:%d)\n", __func__, pwav_url, channel, samplerate, bitspersample);
    struct lbwav_mux_context* pwmc = (struct lbwav_mux_context*)malloc(sizeof(struct lbwav_mux_context));
    memset(pwmc, 0, sizeof(struct lbwav_mux_context));
    pwmc->pwav_file = fopen(pwav_url, "wb");
    strncpy(pwmc->wav_hdr.fcc_id,"RIFF",4);
    strncpy(pwmc->wav_hdr.fcctype,"WAVE",4);
    pwmc->wav_hdr.dwsize = 0;
    
    
    // wav fmt
    pwmc->wav_fmt.dwsample_per_sec = samplerate;
    
    pwmc->wav_fmt.dwavg_bytes_per_sec = samplerate * bitspersample * channel / 8;
    
    pwmc->wav_fmt.uibits_per_sample = bitspersample;
    
    strncpy(pwmc->wav_fmt.fcc_id,"fmt ", 4);
    
    pwmc->wav_fmt.dwsize = 16;
    
    pwmc->wav_fmt.wblock_align = 2;
    
    pwmc->wav_fmt.wchannels = channel;
    
    pwmc->wav_fmt.wformat_tag = 1;
    
    // wav data struct
    strncpy(pwmc->wav_data.fcc_id, "data", 4);
    
    pwmc->wav_data.dwsize = 0;
    lbtrace("sizeof(lbwav_header):%ld, sizeof(lbwav_fmt):%ld, sizeof(lbwav_data):%ld\n", sizeof(struct lbwav_header), sizeof(struct lbwav_fmt), sizeof(struct lbwav_data));
    fseek(pwmc->pwav_file, sizeof(struct lbwav_header)+sizeof(struct lbwav_fmt)+sizeof(struct lbwav_data), SEEK_SET);
    return pwmc;
}

int lbwav_write_data(struct lbwav_mux_context* pwmc, const char* pdata, int len)
{
    if(NULL == pwmc || NULL == pdata || len <= 0)
    {
        lberror("Invalid parameter, pwmc:%p, pdata:%p, len:%d\n", pwmc, pdata, len);
        return -1;
    }
    
    size_t writed = fwrite(pdata, 1, len, pwmc->pwav_file);
    if(writed > 0)
    {
        pwmc->wav_data.dwsize += writed;
    }

    return (int)writed;
}

int lbwav_write_tail(struct lbwav_mux_context* pwmc)
{
    if(NULL == pwmc || NULL == pwmc->pwav_file)
    {
        lberror("Invalid parameter, pwmc:%p, pwmc->pwav_file:%p\n", pwmc, pwmc ? pwmc->pwav_file : NULL);
        return -1;
    }

    fseek(pwmc->pwav_file, 0, SEEK_SET);
    size_t begin_pos = ftell(pwmc->pwav_file);
    int wav_hdr_len = fwrite(&pwmc->wav_hdr, 1, sizeof(pwmc->wav_hdr), pwmc->pwav_file);
    int wav_fmt_len = fwrite(&pwmc->wav_fmt, 1, sizeof(pwmc->wav_fmt), pwmc->pwav_file);
    int wav_data_len = fwrite(&pwmc->wav_data, 1, sizeof(pwmc->wav_data), pwmc->pwav_file);
    size_t end_pos = ftell(pwmc->pwav_file);
    fclose(pwmc->pwav_file);
    lbtrace("lbwav_write_tail, wchannels:%d, dwsize:%u, wav_hdr_len:%d, wav_fmt_len:%d, wav_data_len:%d, begin_pos:%ld, end_pos:%ld\n", pwmc->wav_fmt.wchannels, pwmc->wav_data.dwsize, wav_hdr_len, wav_fmt_len, wav_data_len, begin_pos, end_pos);
    pwmc->pwav_file = NULL;
    return 0;
}

void lbwav_close_contextp(struct lbwav_mux_context** ppwmc)
{
    lbtrace("lbwav_close_contextp(ppwmc:%p)\n", ppwmc);
    if(ppwmc && *ppwmc)
    {
        struct lbwav_mux_context* pwmc = *ppwmc;
        lbwav_write_tail(pwmc);
        free(pwmc);
        *ppwmc = pwmc = NULL;
    }
}

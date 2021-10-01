/*
 * avmuxer.c
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#include "config.h"
#include "avresample.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <libswresample/swresample.h>

#ifndef lbtrace
#define lbtrace printf
#endif
#ifndef lberror
#define lberror printf
#endif
typedef struct avresample_context
{
    struct SwrContext* pswrctx;
    int         nsrc_channel;
    int         nsrc_samplerate;
    int         nsrc_format;
    int         ndst_channel;
    int         ndst_samplerate;
    int         ndst_format;
    AVFrame*    pdst_frame;
} resample_ctx;
struct avresample_context* avresample_alloc_context()
{
    struct avresample_context* pswrctx = (struct avresample_context*)malloc(sizeof(struct avresample_context));
    memset(pswrctx, 0, sizeof(struct avresample_context));
    return pswrctx;
}

void avresample_free_contextp(struct avresample_context** ppresample)
{
    if(ppresample && *ppresample)
    {
        struct avresample_context* presample = *ppresample;
        swr_free(&presample->pswrctx);
        free(presample);
        *ppresample = NULL;
    }
}

struct avresample_context* avresample_init(struct avresample_context* presample, int dst_channel, int dst_samplerate, int dst_format)
{
    if(NULL == presample)
    {
        presample = avresample_alloc_context();
    }
    
    if(presample->pswrctx)
    {
        swr_free(&presample->pswrctx);
    }
    
    /*presample->nsrc_channel = src_channel;
    presample->nsrc_samplerate = src_samplerate;
    presample->nsrc_format = src_format;*/
    presample->ndst_channel = dst_channel;
    presample->ndst_samplerate = dst_samplerate;
    presample->ndst_format = dst_format;
    sv_trace("avresample_init(channel:%d, ndst_samplerate:%d, ndst_format:%d)\n", presample->ndst_channel, presample->ndst_samplerate, presample->ndst_format);
    /*struct SwrContext *swr_alloc_set_opts(struct SwrContext *s,
    int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
    int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
    int log_offset, void *log_ctx);*/
    /*presample->pswrctx = swr_alloc_set_opts(NULL, av_get_default_channel_layout(presample->ndst_channel), (enum AVSampleFormat)presample->ndst_format, presample->ndst_samplerate, av_get_default_channel_layout(presample->nsrc_channel), (enum AVSampleFormat)presample->nsrc_format, presample->nsrc_samplerate, 0, NULL);
    if(NULL == presample->pswrctx)
    {
        avresample_free_contextp(&presample);
    }*/
    return presample;
}

int avresample_resample(struct avresample_context* presample, AVFrame* pdstframe, AVFrame* psrcframe)
{
    if(NULL == presample || NULL == pdstframe || NULL == psrcframe)
    {
        lberror("Invalid parameter, presample:%p, pdstframe:%p, psrcframe:%p\n", presample, pdstframe, psrcframe);
        return -1;
    }
    
    if(psrcframe->channels == presample->ndst_channel && psrcframe->sample_rate == presample->ndst_samplerate && psrcframe->format == presample->ndst_format)
    {
        av_frame_move_ref(pdstframe, psrcframe);
        return pdstframe->nb_samples;
    }
    if(0 != confirm_swr_avaiable(presample, psrcframe))
    {
        return -1;
    }
    int nbsamples = (int)av_rescale_rnd(swr_get_delay(presample->pswrctx, presample->nsrc_samplerate) + psrcframe->nb_samples, presample->ndst_samplerate, presample->nsrc_samplerate, AV_ROUND_UP);
    if(pdstframe->nb_samples != nbsamples || pdstframe->channels != presample->ndst_channel || pdstframe->sample_rate != presample->ndst_samplerate || pdstframe->format != presample->ndst_format)
    {
        av_frame_unref(pdstframe);
        pdstframe->nb_samples = nbsamples;
        pdstframe->channels = presample->ndst_channel;
        pdstframe->channel_layout = av_get_default_channel_layout(presample->ndst_channel);
        pdstframe->format = presample->ndst_format;
        pdstframe->sample_rate = presample->ndst_samplerate;
        av_frame_get_buffer(pdstframe, 4);
        //return 0;
    }
    int ret = swr_convert(presample->pswrctx, pdstframe->data, nbsamples, psrcframe->data, psrcframe->nb_samples);
    
    return ret;
}

int avresample_resample_frame(struct avresample_context* presample, AVFrame* pframe)
{
    if(NULL == presample || NULL == pframe)
    {
        lberror("Invalid parameter, presample:%p, pframe:%p\n", presample, pframe);
        return -1;
    }
    
    if(NULL == presample->pdst_frame)
    {
        presample->pdst_frame = av_frame_alloc();
    }
    
    int ret = avresample_resample(presample, presample->pdst_frame, pframe);
    if(ret > 0)
    {
        av_frame_unref(pframe);
        av_frame_move_ref(pframe, presample->pdst_frame);
    }
    
    return ret > 0 ? 0 : -1;
}

int confirm_swr_avaiable(struct avresample_context* presample, AVFrame* psrcframe)
{
    if(NULL == presample->pswrctx || presample->nsrc_channel != psrcframe->channels || presample->nsrc_samplerate != psrcframe->sample_rate || presample->nsrc_format != psrcframe->format)
    {
        if(presample->pswrctx)
        {
            swr_free(&presample->pswrctx);
        }
        presample->nsrc_channel = psrcframe->channels;
        presample->nsrc_samplerate = psrcframe->sample_rate;
        presample->nsrc_format = psrcframe->format;
        presample->pswrctx = swr_alloc_set_opts(NULL, av_get_default_channel_layout(presample->ndst_channel), (enum AVSampleFormat)presample->ndst_format, presample->ndst_samplerate, av_get_default_channel_layout(presample->nsrc_channel), (enum AVSampleFormat)presample->nsrc_format, presample->nsrc_samplerate, 0, NULL);
        lbtrace("pswrctx:%p = swr_alloc_set_opts(NULL, av_get_default_channel_layout(ndst_channel:%d), ndst_format:%d, presample->ndst_samplerate:%d, av_get_default_channel_layout(nsrc_channel:%d), nsrc_format:%d, nsrc_samplerate:%d, 0, NULL)", presample->pswrctx, presample->ndst_channel, presample->ndst_format, presample->ndst_samplerate, presample->nsrc_channel, presample->nsrc_format, presample->nsrc_samplerate);
        if(NULL == presample->pswrctx)
        {
            lberror("swr_alloc_set_opts failed!\n");
            return -1;
        }
        int ret = swr_init(presample->pswrctx);
        if(ret < 0)
        {
            lberror("swr_init failed, ret:%d\n", ret);
            return ret;
        }
        
        return 0;
    }
    
    return 0;
}

typedef struct sample_buffer
{
    char* pbuf;
    int nmax_buf_len;
    int nbuf_begin;
    int nbuf_end;

    int channel;
    int samplerate;
    int format;
    int64_t lpts;
} sam_buf;

struct sample_buffer* lbsample_buffer_open(int channel, int samplerate, int format, int max_size)
{
    sam_buf* psb = malloc(sizeof(sam_buf));
    memset(psb, 0, sizeof(sam_buf));

    psb->pbuf = (char*)malloc(max_size);
    psb->nmax_buf_len = max_size;
    psb->channel = channel;
    psb->samplerate = samplerate;
    psb->format = format;
    
    return psb;
}

void lbsample_buffer_close(struct sample_buffer ** ppsb)
{
    if(ppsb && *ppsb)
    {
        sam_buf* psb = *ppsb;
        if(psb && psb->pbuf)
        {
            free(psb->pbuf);
        }
        free(psb);
        *ppsb = psb = NULL;
    }
}

int lbsample_buffer_deliver(struct sample_buffer * psb, const char* pbuf, int len, int64_t pts)
{
    if(psb)
    {
        if(lbsample_buffer_remain(psb) > len)
        {
            if(psb->nbuf_begin == 0)
            {
                psb->lpts = pts;
            }
            
            if(psb->nmax_buf_len - psb->nbuf_end < len)
            {
                lbsample_buffer_align(psb);
            }

            memcpy(psb->pbuf + psb->nbuf_end, pbuf, len);
            psb->nbuf_end += len;
            return len;
        }
        else
        {
            return 0;
        }
    }
    
    return 0;
}

int lbsample_buffer_fetch(struct sample_buffer* psb, char* pbuf, int len, int64_t* ppts)
{
    if(lbsample_buffer_size(psb) >= len)
    {
        memcpy(pbuf, psb->pbuf + psb->nbuf_begin, len);
        psb->nbuf_begin += len;
        if(psb->nbuf_begin == psb->nbuf_end)
        {
            psb->nbuf_begin = psb->nbuf_end = 0;
        }
        if(ppts)
        {
            *ppts = psb->lpts;
        }
        long offset = len*1000000 / (psb->channel*psb->samplerate * av_get_bytes_per_sample(psb->format));
        psb->lpts += offset;
        return len;
    }
    
    return 0;
}


int lbsample_buffer_remain(struct sample_buffer* psb)
{
    if(psb)
    {
        return psb->nmax_buf_len - (psb->nbuf_end - psb->nbuf_begin);
    }
    return 0;
}

void lbsample_buffer_align(struct sample_buffer* psb)
{
    if(lbsample_buffer_size(psb) > 0)
    {
        memmove(psb->pbuf, psb->pbuf + psb->nbuf_begin, psb->nbuf_end - psb->nbuf_begin);
        psb->nbuf_end = psb->nbuf_end - psb->nbuf_begin;
        psb->nbuf_begin = 0;
        
    }
    else
    {
        psb->nbuf_begin = psb->nbuf_end = 0;
    }
}

int lbsample_buffer_size(struct sample_buffer* psb)
{
    if(psb)
    {
        return psb->nbuf_end - psb->nbuf_begin;
    }
    
    return 0;
}

void lbsample_buffer_reset(struct sample_buffer* psb)
{
    if(psb)
    {
        psb->nbuf_begin = psb->nbuf_end = 0;
    }
}

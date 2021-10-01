#pragma once
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <libavcodec/avcodec.h>
#ifndef lbtrace
#define lbtrace printf
#endif
#ifndef lberror
#define lberror printf
#endif

typedef struct avencoder_context
{
    int     codec_type;
    int     codec_id;
    int     format;
    int     width;
    int     height;
    int     channel;
    int     samplerate;
    int     bitrate;
    int     fps;
    AVCodecContext* pcodec_ctx;
} enc_ctx;

static enc_ctx* avencoder_alloc_context()
{
    enc_ctx* penc_ctx = (struct enc_ctx*)malloc(sizeof(enc_ctx));
    memset(penc_ctx, 0, sizeof(enc_ctx));
    
    return penc_ctx;
}

void avencoder_free_context(struct enc_ctx** ppenc_ctx)
{
    if(ppenc_ctx && *ppenc_ctx)
    {
        enc_ctx* penc_ctx = *ppenc_ctx;
        if(penc_ctx->pcodec_ctx)
        {
            avcodec_free_context(&penc_ctx->pcodec_ctx);
            penc_ctx->pcodec_ctx = NULL;
        }
        free(penc_ctx);
        *ppenc_ctx = NULL;
    }
}

static int avencoder_context_open(struct avencoder_context** ppenc_ctx, int codec_type, int codec_id, int param1, int param2, int praram3, int format, int bitrate)
{
    struct avencoder_context* ptmp_enc = NULL;
    if(NULL == ppenc_ctx)
    {
        lberror("avencoder_context_open failed, invalid parameter, ppenc_ctx:%p\n", ppenc_ctx);
        return -1;
    }
    ptmp_enc = *ppenc_ctx;
    if(NULL == ptmp_enc)
    {
        ptmp_enc = avencoder_alloc_context();
    }
    //int avencoder_init(struct enc_ctx* penc_ctx, int codec_type, int codec_id, int param1, int param2, int param3, int format, int bitrate);
    int ret = avencoder_init((struct avencoder_context*)ptmp_enc, codec_type, codec_id, param1, param2, praram3, format, bitrate);
    if(ret < 0)
    {
        lberror("init encoder failed, ret:%d\n", ret);
        if(NULL == *ppenc_ctx && ptmp_enc)
        {
            avencoder_free_contextp(&ptmp_enc);
            return -1;
        }
    }
    *ppenc_ctx = ptmp_enc;
    return ret;
}
//static int avencoder_init(enc_ctx* penc_ctx, int codec_type, int codec_id, int param1, int param2, int param3, int format, int bitrate);
/*static enc_ctx* avencoder_context_open(enc_ctx* penc_ctx, int codec_type, int codec_id, int param1, int param2, int praram3, int format, int bitrate)
{
    enc_ctx* ptmp_enc = NULL;
    if(NULL == penc_ctx)
    {
        ptmp_enc = penc_ctx = avencoder_alloc_context();
    }
    //int avencoder_init(struct enc_ctx* penc_ctx, int codec_type, int codec_id, int param1, int param2, int param3, int format, int bitrate);
    int ret = avencoder_init((struct enc_ctx*)penc_ctx, codec_type, codec_id, param1, param2, praram3, format, bitrate);
    if(ret < 0)
    {
        lberror("init encoder failed, ret:%d\n", ret);
        if(ptmp_enc)
        {
            avencoder_free_context(&ptmp_enc);
            return NULL;
        }
    }
    
    return penc_ctx;
}*/

static int avencoder_init(enc_ctx* penc_ctx, int codec_type, int codec_id, int param1, int param2, int param3, int format, int bitrate)
{
    if(NULL == penc_ctx)
    {
        lberror("Invalid parameter penc_ctx:%p\n", penc_ctx);
        return -1;
    }
    AVCodec* pcodec = avcodec_find_encoder(codec_id);
    if(NULL == pcodec)
    {
        lberror("can't find codec id:%d encoder\n", codec_id);
        return -1;
    }
    AVCodecContext* pcodec_ctx = avcodec_alloc_context3(pcodec);
    if(NULL == pcodec_ctx)
    {
        lberror("create codec id:%d AVCodecContext failed\n", codec_id);
        return -1;
    }
    pcodec_ctx->codec_type = codec_type;
    pcodec_ctx->codec_id = codec_id;
    if(AVMEDIA_TYPE_VIDEO == codec_type)
    {
        pcodec_ctx->width = param1;
        pcodec_ctx->height = param2;
        pcodec_ctx->pix_fmt = format;
        pcodec_ctx->time_base.num = param3;
        pcodec_ctx->time_base.den = 1;
        pcodec_ctx->sample_aspect_ratio.den = 1;
        pcodec_ctx->sample_aspect_ratio.num = 1;
        penc_ctx->width         = width;
        penc_ctx->height        = height;
    }
    else if(AVMEDIA_TYPE_AUDIO == codec_type)
    {
        pcodec_ctx->channels = param1;
        pcodec_ctx->sample_rate = param2;
        pcodec_ctx->sample_fmt = format;
        pcodec_ctx->time_base.num = param3;
        pcodec_ctx->time_base.den = 1;
        penc_ctx->channel         = param1;
        penc_ctx->samplerate      = param2;
    }
    else
    {
        lberror("Invalid codec_type:%d\n", codec_id);
        return 0;
    }

    int ret = avcodec_open2(pcodec_ctx, pcodec, NULL);
    if(ret < 0)
    {
        lberror("avcodec_open2 codec id:%d failed\n", codec_id);
        avcodec_free_context(&pcodec_ctx);
        return ret;
    }

    penc_ctx->codec_type    = codec_type;
    penc_ctx->codec_id      = codec_id;
    penc_ctx->format        = format;
    penc_ctx->bitrate       = bitrate;
    penc_ctx->pcodec_ctx    = pcodec_ctx;
    return ret;
}

int avencoder_encode_frame(enc_ctx* penc_ctx, AVFrame* pframe, char* pout_buf, int out_len)
{
    if(NULL == penc_ctx || NULL == pframe || NULL == penc_ctx->pcodec_ctx || NULL == pout_buf)
    {
        lberror("Invalid parameter penc_ctx:%p, pframe:%p, pout_buf:%p or codec context not init\n", penc_ctx, pframe, pout_buf);
        return -1;
    }

    int got_picture = 0;
    AVPacket pkt;
    av_init_packet(&pkt);
    //av_new_packet(&pkt, (penc_ctx->pcodec_ctx->width * penc_ctx->pcodec_ctx->height));
    int ret = avcodec_send_frame(penc_ctx->pcodec_ctx, pframe);
    if(ret < 0)
    {
        lberror("avcodec_send_frame failed, ret:%d\n", ret);
        return ret;
    }
    
    ret = avcodec_receive_packet(penc_ctx->pcodec_ctx, &pkt);
    if (AVERROR(EAGAIN) == ret || AVERROR_EOF == ret)
    {
        return 0;
    }
    else
    {
        assert(0);
        return -1;
    }
    int enclen = pkt.size;
    if(pkt.size < out_len)
    {
        memcpy(pout_buf, pkt.data, enclen);
    }
    av_packet_unref(&pkt);
    return enclen;
}

int encoder_frame(int codecid, AVFrame* pframe, char* pout_buf, int out_len)
{
    if(NULL == pframe || NULL == pout_buf || out_len <= 0)
    {
        lberror("Invalid parameter pframe:%p, pout_buf:%p, out_len:%d\n", pframe, pout_buf, out_len);
        return -1;
    }
    int codec_type = -1;
    if(pframe->width > 0)
    {
        codec_type = AVMEDIA_TYPE_VIDEO;
    }
    else if(pframe->channels > 0)
    {
        codec_type = AVMEDIA_TYPE_AUDIO;
    }
    else
    {
        lberror("Invalid avframe, pframe->width:%d, pframe->channel:%d\n", pframe->width, pframe->channels);
        return -1;
    }
    enc_ctx* pencctx = NULL;
    int ret = avencoder_context_open(&pencctx, codec_type, codecid, pframe->width, pframe->height, 25, pframe->format, 0);
    if(NULL == pencctx)
    {
        lberror("avencoder context ope failed, pencctx:%p = avencoder_context_open(NULL, codec_type:%d, codec_id:%d, pframe->width:%d, pframe->height:%d, pframe->format:%d)\n", pencctx, codec_type, codecid, pframe->width, pframe->height, pframe->format);
        return -1;
    }
    
    ret = avencoder_encode_frame(pencctx, pframe, pout_buf, out_len);
    
    avencoder_free_context(&pencctx);
    
    return ret;
}



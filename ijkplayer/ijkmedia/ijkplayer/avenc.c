/*
 * avmuxer.c
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#include "config.h"
#include "avenc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include "cachebuf.h"
#include "avresample.h"
#include "list.h"
#include "audio_suquence_header.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include "libavutil/imgutils.h"
#include <libavutil/samplefmt.h>
#include "ff_cmdutils.h"
#include "lbsc_util_conf.h"
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
    int     dstfmt;
    AVFrame*                    pframe;
    AVFrame*                    pswrframe;
    AVCodecContext*             pcodec_ctx;
    cache_ctx*                  pcache_ctx;
    struct avresample_context*  presample;
    struct adts_context*        padts_ctx;
    struct list_context*        plist;
    pthread_mutex_t*            pmutex;
#ifdef ENABLE_WRITE_INPUT_AND_OUTPUT
    FILE*                       pinfile;
    FILE*                       poutfile;
    int                         benable_write_input;
    int                         benable_write_output;
#endif
} enc_ctx;

struct avencoder_context* avencoder_alloc_context()
{
    struct avencoder_context* penc_ctx = (struct avencoder_context*)malloc(sizeof(struct avencoder_context));
    memset(penc_ctx, 0, sizeof(struct avencoder_context));
    penc_ctx->pmutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
#ifdef ENABLE_WRITE_INPUT_AND_OUTPUT
    penc_ctx->pinfile = NULL;
    penc_ctx->poutfile = NULL;
#endif
    INIT_RECURSIVE_MUTEX(penc_ctx->pmutex);
    return penc_ctx;
}

void avencoder_free_contextp(struct avencoder_context** ppenc_ctx)
{
    if(ppenc_ctx && *ppenc_ctx)
    {
        struct avencoder_context* penc_ctx = *ppenc_ctx;
        pthread_mutex_lock(penc_ctx->pmutex);
        if(penc_ctx->pcodec_ctx)
        {
            avcodec_free_context(&penc_ctx->pcodec_ctx);
            penc_ctx->pcodec_ctx = NULL;
        }
        
        if(penc_ctx->presample)
        {
            avresample_free_contextp(&penc_ctx->presample);
        }
        
        if(penc_ctx->pcache_ctx)
        {
            cache_context_closep(&penc_ctx->pcache_ctx);
        }
        
        if(penc_ctx->padts_ctx)
        {
            adts_demux_close(&penc_ctx->padts_ctx);
        }
        
        if(penc_ctx->pframe)
        {
            av_frame_free(&penc_ctx->pframe);
        }
        
        if(penc_ctx->pswrframe)
        {
            av_frame_free(&penc_ctx->pswrframe);
        }
        pthread_mutex_unlock(penc_ctx->pmutex);
        if(penc_ctx->pmutex)
        {
            pthread_mutex_destroy(penc_ctx->pmutex);
            free(penc_ctx->pmutex);
            penc_ctx->pmutex = NULL;
        }
#ifdef ENABLE_WRITE_INPUT_AND_OUTPUT
        if(penc_ctx->pinfile)
        {
            fclose(penc_ctx->pinfile);
        }
        if(penc_ctx->poutfile)
        {
            fclose(penc_ctx->poutfile);
        }
#endif
        free(penc_ctx);
        *ppenc_ctx = NULL;
    }
}

int avencoder_context_open(struct avencoder_context** ppenc_ctx, int codec_type, int codec_id, int param1, int param2, int praram3, int format, int bitrate)
{
    lbdebug("avencoder_context_open(ppenc_ctx:%p, codec_type:%d, codec_id:%d, param1:%d, param2:%d, praram3:%d, format:%d, bitrate:%d)\n", ppenc_ctx, codec_type, codec_id, param1, param2, praram3, format, bitrate);
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
    int in_data_log = 0;
    get_default_int_config("encoder_in_data_log_file", &ptmp_enc->benable_write_input);
    get_default_int_config("encoder_out_data_log_file", &ptmp_enc->benable_write_output);
    *ppenc_ctx = ptmp_enc;
    return ret;
}

int avencoder_init(struct avencoder_context* penc_ctx, int codec_type, int codec_id, int param1, int param2, int param3, int format, int bitrate)
{
    lbtrace("avencoder_init(penc_ctx:%p, codec_type:%d, codec_id:%d, param1:%d, param2:%d, param3:%d, format:%d, bitrate:%d)\n", penc_ctx, codec_type, codec_id, param1, param2, param3, format, bitrate);
    if(NULL == penc_ctx)
    {
        lberror("Invalid parameter penc_ctx:%p\n", penc_ctx);
        return -1;
    }
    av_register_all();
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
    pthread_mutex_lock(penc_ctx->pmutex);
    penc_ctx->codec_type = pcodec_ctx->codec_type = codec_type;
    penc_ctx->codec_id = pcodec_ctx->codec_id = codec_id;
    penc_ctx->format = format;
    penc_ctx->bitrate = bitrate;
    pcodec_ctx->bit_rate = bitrate;
    if(AVMEDIA_TYPE_VIDEO == codec_type)
    {
        penc_ctx->width = pcodec_ctx->width = param1;
        penc_ctx->height = pcodec_ctx->height = param2;
        if(AV_CODEC_ID_MJPEG == pcodec_ctx->codec_id && AV_PIX_FMT_YUV420P == penc_ctx->format)
        {
            penc_ctx->format = pcodec_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
            lbtrace("penc_ctx->format:%d change to AV_PIX_FMT_YUVJ420P\n", penc_ctx->format);
        }
        else
        {
            penc_ctx->format = pcodec_ctx->pix_fmt = format;
        }
        //penc_ctx->format = pcodec_ctx->pix_fmt = format;
        penc_ctx->fps = param3;
        pcodec_ctx->time_base.num = param3;
        pcodec_ctx->time_base.den = 1;
        pcodec_ctx->sample_aspect_ratio.den = 1;
        pcodec_ctx->sample_aspect_ratio.num = 1;
    }
    else if(AVMEDIA_TYPE_AUDIO == codec_type)
    {
        penc_ctx->channel = pcodec_ctx->channels = param1;
        penc_ctx->samplerate = pcodec_ctx->sample_rate = param2;
        pcodec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;//format;
        pcodec_ctx->time_base.num = param3;
        pcodec_ctx->time_base.den = 1;
        penc_ctx->dstfmt = pcodec_ctx->sample_fmt;
    }
    else
    {
        lberror("Invalid codec_type:%d\n", codec_id);
        pthread_mutex_unlock(penc_ctx->pmutex);
        return 0;
    }

    int ret = avcodec_open2(pcodec_ctx, pcodec, NULL);
    lbdebug("ret:%d = avcodec_open2(pcodec_ctx:%p, pcodec:%p, NULL)\n", ret, pcodec_ctx, pcodec);
    if(ret < 0)
    {
        lberror("avcodec_open2 codec id:%d failed\n", codec_id);
        avcodec_free_context(&pcodec_ctx);
        pthread_mutex_unlock(penc_ctx->pmutex);
        return ret;
    }

    penc_ctx->pcodec_ctx = pcodec_ctx;
    pthread_mutex_unlock(penc_ctx->pmutex);

    return 0;
}

int avencoder_encode_frame(struct avencoder_context* penc_ctx, AVFrame* pframe, char* pout_buf, int out_len)
{
    lbtrace("(penc_ctx:%p, pframe:%p, pout_buf:%p, out_len:%d) begin\n", penc_ctx, pframe, pout_buf, out_len);
    int ret = 0;
    pthread_mutex_lock(penc_ctx->pmutex);
    do
    {
        if(NULL == penc_ctx || NULL == pframe || NULL == penc_ctx->pcodec_ctx || NULL == pframe->data[0])
        {
            lberror("Invalid parameter penc_ctx:%p, pframe:%p, pframe->data[0]:%p or codec context not init\n", penc_ctx, pframe, pframe ? pframe->data[0]: NULL);
            ret = -1;
            break;
        }

        AVFrame* pencframe = pframe;
        if(AVMEDIA_TYPE_AUDIO == penc_ctx->codec_type)
        {
            if(NULL == penc_ctx->pswrframe)
            {
                penc_ctx->pswrframe = av_frame_alloc();
            }
            if(NULL == penc_ctx->presample)
            {
                penc_ctx->presample = avresample_init(NULL, penc_ctx->channel, penc_ctx->samplerate, penc_ctx->dstfmt);
            }
            
            avresample_resample(penc_ctx->presample, penc_ctx->pswrframe, pframe);
            pencframe = penc_ctx->pswrframe;
        }
        
        //av_init_packet(&pkt);
        //av_new_packet(&pkt, (penc_ctx->pcodec_ctx->width * penc_ctx->pcodec_ctx->height));
        ret = avcodec_send_frame(penc_ctx->pcodec_ctx, pencframe);
        //lbtrace("ret:%d = avcodec_send_frame(penc_ctx->pcodec_ctx:%p, pencframe:%p)\n", ret, penc_ctx->pcodec_ctx, pencframe);
        if(ret < 0)
        {
            lberror("avcodec_send_frame failed, ret:%d\n", ret);
            break;
        }
        AVPacket* pkt = av_packet_alloc();
        ret = avcodec_receive_packet(penc_ctx->pcodec_ctx, pkt);
        lbtrace("ret:%d = avcodec_receive_packet(penc_ctx->pcodec_ctx, pkt:%p)\n", ret, pkt);
        if (AVERROR(EAGAIN) == ret || AVERROR_EOF == ret)
        {
            break;
        }
        else if(0 != ret)
        {
            assert(0);
            break;
        }
        ret = pkt->size;
        if(AVMEDIA_TYPE_AUDIO == penc_ctx->codec_type && AV_CODEC_ID_AAC == penc_ctx->codec_id)
        {
            
            if(NULL == penc_ctx->padts_ctx)
            {
                penc_ctx->padts_ctx = adts_demux_sequence_header((char*)penc_ctx->pcodec_ctx->extradata, penc_ctx->pcodec_ctx->extradata_size);
            }
            assert(penc_ctx->padts_ctx);
            int adts_pkt_size = pkt->size + penc_ctx->padts_ctx->adts_header_size;
            AVPacket* adtspkt = av_packet_alloc();
            av_packet_copy_props(adtspkt, pkt);
            av_new_packet(adtspkt, adts_pkt_size);
            //uint8_t* pdata = (uint8_t*)av_malloc(adts_pkt_size);
            int aachdrlen = muxer_adts_header(penc_ctx->padts_ctx, pkt->size, (char*)adtspkt->data, adts_pkt_size);
            if(aachdrlen <= 0)
            {
                lberror("muxer adts header failed, aaclen:%d\n", aachdrlen);
                ret = 0;
                break;
            }
            memcpy(adtspkt->data + aachdrlen, pkt->data, pkt->size);
            av_packet_free(&pkt);
            pkt = adtspkt;
            ret = adts_pkt_size;
        }
        
        if(pout_buf && pkt && pkt->size < out_len)
        {
            memcpy(pout_buf, pkt->data, pkt->size);
#ifdef ENABLE_WRITE_INPUT_AND_OUTPUT
            if(penc_ctx->benable_write_output && NULL == penc_ctx->poutfile && get_log_path())
            {
                char outpath[156];
                sprintf(outpath, "%s/enc_out_%s.data", get_log_path(), 0 == penc_ctx->codec_type ? "video" : "audio");
                penc_ctx->poutfile = fopen(outpath, "wb");
            }

            if(penc_ctx->poutfile && pkt->size > 0)
            {
                fwrite(pkt->data, 1, pkt->size, penc_ctx->poutfile);
            }
            av_packet_free(&pkt);
#endif
        }
        else
        {
            if(NULL == penc_ctx->plist)
            {
                penc_ctx->plist = list_context_create(100);
            }
            
            int res = push(penc_ctx->plist, pkt);
            if(res < 0)
            {
                lberror("push packet list failed, res:%d\n", res);
            }
        }
        //av_packet_unref(pkt);
    } while(0);
    pthread_mutex_unlock(penc_ctx->pmutex);
    lbtrace("ret:%d = avencoder_encode_frame end\n", ret);
    return ret;
}

int avencoder_encoder_data(struct avencoder_context* penc_ctx, char* pdata, int datalen, char* pout_buf, int out_len, long pts)
{
    if(NULL == penc_ctx)
    {
        lberror("Invalid paramter, penc_ctx:%p\n", penc_ctx);
        return -1;
    }
#ifdef ENABLE_WRITE_INPUT_AND_OUTPUT
    if(penc_ctx->benable_write_input && NULL == penc_ctx->pinfile && get_log_path())
    {
        char inpath[156];
        sprintf(inpath, "%s/enc_in_%s.data", get_log_path(), 0 == penc_ctx->codec_type ? "video" : "audio");
        penc_ctx->pinfile = fopen(inpath, "wb");
    }
    if(penc_ctx->pinfile && datalen > 0)
    {
        fwrite(pdata, 1, datalen, penc_ctx->pinfile);
    }
#endif
    
    if(!penc_ctx || !penc_ctx->pcodec_ctx || !pdata)
    {
        lberror("Invalid parameter, penc_ctx:%p, penc_ctx->pcodec_ctx:%p, pdata:%p\n", penc_ctx, penc_ctx->pcodec_ctx, pdata);
        return -1;
    }
    int ret = 0;
    lbtrace("(penc_ctx:%p, pdata:%p, datalen:%d, pout_buf:%p, out_len:%d, pts:%ld)\n", penc_ctx, pdata, datalen, pout_buf, out_len, pts);
    pthread_mutex_lock(penc_ctx->pmutex);
    if(NULL == penc_ctx->pframe)
    {
        penc_ctx->pframe = av_frame_alloc();
    }
    
    if(AVMEDIA_TYPE_VIDEO == penc_ctx->codec_type)
    {
        ret = av_image_fill_arrays(penc_ctx->pframe->data, penc_ctx->pframe->linesize, (uint8_t*)pdata, (enum AVPixelFormat)penc_ctx->format, penc_ctx->width, penc_ctx->height, 4);
    }
    else if(AVMEDIA_TYPE_AUDIO == penc_ctx->codec_type)
    {
        if(NULL == penc_ctx->pcache_ctx)
        {
            penc_ctx->pcache_ctx = cache_context_open(1024*10);
        }
        
        int ret = cache_deliver_data(penc_ctx->pcache_ctx, (uint8_t*)pdata, datalen);
        if(penc_ctx->pframe->nb_samples != penc_ctx->pcodec_ctx->frame_size)
        {
            penc_ctx->pframe->nb_samples = penc_ctx->pcodec_ctx->frame_size;
            penc_ctx->pframe->channels = penc_ctx->channel;
            penc_ctx->pframe->sample_rate = penc_ctx->samplerate;
            penc_ctx->pframe->channel_layout = av_get_default_channel_layout(penc_ctx->channel);
            penc_ctx->pframe->format = penc_ctx->format;
            av_frame_get_buffer(penc_ctx->pframe, 2);
        }
        int recvlen =  cache_fetch_data(penc_ctx->pcache_ctx, penc_ctx->pframe->data[0], penc_ctx->pframe->linesize[0]);
        if(recvlen <= 0)
        {
            pthread_mutex_unlock(penc_ctx->pmutex);
            return 0;
        }
    }
    penc_ctx->pframe->pts = pts;
    pthread_mutex_unlock(penc_ctx->pmutex);

    ret = avencoder_encode_frame(penc_ctx, penc_ctx->pframe, pout_buf, out_len);
    
    return ret;
}

int avencoder_get_packet(struct avencoder_context* penc_ctx, char* pdata, int* pdatalen, long* pts)
{
    if(NULL == penc_ctx || NULL == pdatalen)
    {
        lberror("Invalid parameter, penc_ctx:%p, pdatalen:%p\n", penc_ctx, pdatalen);
        return -1;
    }
    //lbtrace("(penc_ctx:%p, pdata:%p, *pdatalen:%d, pts:%p)\n", penc_ctx, pdata, *pdatalen, pts);
    int copylen = 0;
    pthread_mutex_lock(penc_ctx->pmutex);
    if(penc_ctx->plist && list_size(penc_ctx->plist) > 0)
    {
        AVPacket* pkt = front(penc_ctx->plist);
        //lbtrace("pkt:%p = front(penc_ctx->plist)\n", pkt);
        int datalen = *pdatalen;
        if(pkt && pdatalen)
        {
            *pdatalen = pkt->size;
        }
        //lbtrace("*pdatalen:%d, datalen:%d\n", *pdatalen, datalen);
        if(pdata && datalen >= pkt->size)
        {
            memcpy(pdata, pkt->data, pkt->size);
            //lbtrace("memcpy(pdata:%p, pkt->data:%p, pkt->size:%d)\n", pdata, pkt->data, pkt->size);
            *pdatalen = pkt->size;
            pop(penc_ctx->plist);
            copylen = pkt->size;
            //lbtrace("copylen:%d\n", copylen);
            av_packet_free(&pkt);
            //lbtrace("av_packet_free(&pkt)\n");
        }
    }
    pthread_mutex_unlock(penc_ctx->pmutex);
#ifdef ENABLE_WRITE_INPUT_AND_OUTPUT
    if(penc_ctx->benable_write_output && NULL == penc_ctx->poutfile && get_log_path())
    {
        char outpath[156];
        sprintf(outpath, "%s/out.%s", get_log_path(), penc_ctx->codec_type == 0 ? "jpg":"aac");
        penc_ctx->poutfile = fopen(outpath, "wb");
    }

    if(penc_ctx->poutfile && copylen > 0)
    {
        fwrite(pdata, 1, copylen, penc_ctx->poutfile);
    }
#endif
    lbtrace("avencoder_get_packet packet size:%d\n", copylen);
    return copylen;
}

int encoder_frame(int codecid, AVFrame* pframe, char* pout_buf, int out_len)
{
    if(NULL == pframe || NULL == pout_buf || out_len <= 0)
    {
        lberror("Invalid parameter, pframe:%p, pout_buf:%p, out_len:%d\n", pframe, pout_buf, out_len);
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
    struct avencoder_context* pencctx = NULL;
    int ret = avencoder_context_open(&pencctx, codec_type, codecid, pframe->width, pframe->height, 25, pframe->format, 0);
    if(NULL == pencctx)
    {
        lberror("avencoder context open failed, pencctx:%p = avencoder_context_open(NULL, codec_type:%d, codec_id:%d, pframe->width:%d, pframe->height:%d, pframe->format:%d)\n", pencctx, codec_type, codecid, pframe->width, pframe->height, pframe->format);
        return -1;
    }
    
    ret = avencoder_encode_frame(pencctx, pframe, pout_buf, out_len);
    
    avencoder_free_contextp(&pencctx);
    
    return ret;
}

AVFrame* decoder_packet(int mediatype, int codecid, char* ph264, int h264len)
{
    lbtrace("mediatype:%d, codecid:%d, ph264:%p, h264len:%d\n", mediatype, codecid, ph264, h264len);
    int ret = -1;
    AVCodecContext* pcodecctx = NULL;
    AVPacket* pkt = NULL;
    AVFrame* pframe = NULL;
    pkt = av_packet_alloc();
    char* pdata = av_malloc(h264len);
    memcpy(pdata, ph264, h264len);
    //ret = av_new_packet(pkt, h264len);
    ret = av_packet_from_data(pkt, pdata, h264len);
    do{
        av_register_all();
        AVCodec* pcodec = avcodec_find_decoder((enum AVCodecID)codecid);
        if(NULL == pcodec)
        {
            lberror("codec id:%d not support in current ffmpeg config\n", codecid);
            break;
        }
        pcodecctx = avcodec_alloc_context3(pcodec);
        if(NULL == pcodecctx)
        {
            lberror("pcodecctx:%p = avcodec_alloc_context3 failed\n", pcodecctx);
            break;
        }
        
        pcodecctx->codec_type = AVMEDIA_TYPE_VIDEO;
        pcodecctx->codec_id = (enum AVCodecID)codecid;
        ret = avcodec_open2(pcodecctx, pcodec, NULL);
        if(ret < 0)
        {
            lberror("ret:%d = avcodec_open2(pcodecctx:%p, pcodec:%p, NULL)", ret, pcodecctx, pcodec);
            break;
        }
        
        pkt->pts = 0;
        pkt->dts = 0;
        pkt->duration = 1;
        pkt->stream_index = 0;
        pkt->flags = 1;
        lbtrace("before avcodec_send_packet(pcodecctx, pkt), size:%d\n",  pkt->size);
        ret = avcodec_send_packet(pcodecctx, pkt);
        av_packet_unref(pkt);
        AVFrame* pdecframe = av_frame_alloc();
        while((ret = avcodec_receive_frame(pcodecctx, pdecframe)) == AVERROR(EAGAIN))
        {
            ret = avcodec_send_packet(pcodecctx, pkt);
        }
        lbtrace("ret:%d = avcodec_receive_frame(pcodecctx, pframe)\n", ret);
        if(pdecframe->linesize[0] > 0)
        {
            ret = 0;
            pframe = pdecframe;
            pdecframe = NULL;
            /*pframe = av_frame_alloc();
            pframe->width = pdecframe->width;
            pframe->height = pdecframe->height;
            pframe->format = pdecframe->format;
            pframe->pts = 0;
            av_frame_get_buffer(pframe, 4);
            av_frame_copy(pframe, pdecframe);
            //pframe = av_frame_clone(pdecframe);
            av_frame_free(&pdecframe);*/
            lbtrace("pframe->width:%d, pframe->height:%d, pframe->format:%d, pframe->linesize[0]:%d, pframe->linesize[1]:%d, pframe->linesize[2]:%d", pframe->width, pframe->height, pframe->format, pframe->linesize[0], pframe->linesize[1], pframe->linesize[2]);
        }
        else
        {
            lbtrace("pframe->linesize[0]:%d <= 0 decoder h264 packet failed!\n", pdecframe->linesize[0]);
            ret = -1;
            av_frame_free(&pdecframe);
            pdecframe = NULL;
        }
    }while(0);
    
    if(pcodecctx)
    {
        avcodec_free_context(&pcodecctx);
        pcodecctx = NULL;
    }
    if(pkt)
    {
        av_packet_free(&pkt);
        pkt = NULL;
    }
    
    return pframe;
}

int convert_h26x_to_jpg(int codecid, char* penc_data, int data_len, char* pjpg, int jpglen)
{
    lbtrace("convert_h264_to_jpg(penc_data:%p, data_len:%d, pjpg:%p, jpglen:%d)\n", penc_data, data_len, pjpg, jpglen);
    AVFrame* pframe = decoder_packet(AVMEDIA_TYPE_VIDEO, codecid, penc_data, data_len);
    if(NULL == pframe)
    {
        lberror("pframe:%p = decoder_packet\n", pframe);
        return -1;
    }
    if(AV_PIX_FMT_YUV420P == pframe->format)
    {
        pframe->format = AV_PIX_FMT_YUVJ420P;
        lbtrace("convert_h26x_to_jpg pframe->format:%d change to AV_PIX_FMT_YUVJ420P\n", pframe->format);
    }
    int ret = encoder_frame(AV_CODEC_ID_MJPEG, pframe, pjpg, jpglen);
    av_frame_free(&pframe);
    lbtrace("ret:%d = encoder_frame(AV_CODEC_ID_MJPEG, pframe:%p, pjpg:%p, jpglen:%d)\n", ret, pframe, pjpg, jpglen);
	sv_memory(3, pjpg, 32, "jpg data:");
    return ret;
}

int h26x_keyframe_to_jpg_file(int codecid, char* penc_data, int data_len, char* pjpgfile)
{
    int ret = -1;
    char* pjpgdata = NULL;
    lbtrace("h26x_keyframe_to_jpg_file(penc_data:%p, data_len:%d, pjpgfile:%p)\n", penc_data, data_len, pjpgfile);
    if(NULL == penc_data || NULL == pjpgfile)
    {
        lberror("Invalid parameter, penc_data:%p, data_len:%d, pjpgfile:%s\n", penc_data, data_len, pjpgfile);
        return -1;
    }
    
    pjpgdata = (char*)malloc(1024*1024);
    if(pjpgdata)
    {
        ret = convert_h26x_to_jpg(codecid, penc_data, data_len, pjpgdata, 1024*1024);
        if(ret > 0)
        {
            FILE* pfile = fopen(pjpgfile, "wb");
            if(pfile)
            {
                int wlen = fwrite(pjpgdata, 1, ret, pfile);
                fclose(pfile);
                ret = wlen >= ret ? 0 : -1;
            }
        }
    }
    free(pjpgdata);
    return ret;
}

#ifdef ENABLE_WRITE_INPUT_AND_OUTPUT
int set_enable_write_test_data(struct avencoder_context* penc_ctx, int bwrite_input, int bwrite_output)
{
    if(NULL == penc_ctx)
    {
        assert(penc_ctx);
        return -1;
    }
    
    penc_ctx->benable_write_input = bwrite_input;
    penc_ctx->benable_write_output = bwrite_output;
    return 0;
}
#endif

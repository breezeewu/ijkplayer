/*
 * avdemuxer.c
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */
#include "config.h"
#include <unistd.h>
#include "avmuxer.h"
#include "avdemux.h"
#include "libavformat/avformat.h"
#include <pthread.h>
#include <stdint.h>
#include "avcc.h"
#include "audio_suquence_header.h"
#include "lbsc_media_parser.h"
#include "list.h"

AVRational std_tb = {1, AV_TIME_BASE};
typedef struct avdemux_context
{
    AVFormatContext*            pfmtctx;
    struct avmuxer_context*     pdev_mc;
    deliver_callback            pdev_cb;
    pthread_mutex_t*            pmutex;
    pthread_t                   tid;
    int64_t                     llduration;
    int                         nvidx;
    int                         naidx;
    int                         brun;
    // video codec info
    int                         vcodec_id;
    int                         width;
    int                         height;
    int                         pix_fmt;
    int                         vbitrate;
    char*                       pvextra_data;
    int                         nvextra_data_size;
    
    // audio codec info
    
    int                         acodec_id;
    int                         channel;
    int                         samplerate;
    int                         smp_fmt;
    int                         abitrate;
    char*                       paextra_data;
    int                         naextra_data_size;
} mux_ctx;

struct avdemux_context* avdemux_open_context(const char* psource)
{
    AVFormatContext*  pfmtctx = NULL;
    do
    {
        AVDictionary* popt_dict = NULL;
        av_dict_set(&popt_dict, "stimeout", "30000000", 0);
        int ret = avformat_open_input(&pfmtctx, psource, NULL, &popt_dict);
        if(ret < 0)
        {
            lberror("faile to open input url:%s, ret:%d\n", psource, ret);
            break;
        }
        
        ret = avformat_find_stream_info(pfmtctx, NULL);
        if(ret < 0)
        {
            lberror("ret:%d = avformat_find_stream_info(pfmtctx:%p, NULL) failed\n", ret, pfmtctx);
            return NULL;
        }
        
        int vidx = av_find_best_stream(pfmtctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if(vidx < 0)
        {
            lberror("find video stream faieded, vidx:%d\n", vidx);
            break;
        }
        
        int aidx = av_find_best_stream(pfmtctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
        if(aidx < 0)
        {
            lberror("find audio stream faieded, aidx:%d\n", aidx);
            //return NULL;
        }
        
        struct avdemux_context* pdc = (struct avdemux_context*)malloc(sizeof(struct avdemux_context));
        memset(pdc, 0, sizeof(struct avdemux_context));
        pdc->pmutex = malloc(sizeof(pthread_mutex_t));
        pthread_mutexattr_t attr;
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(pdc->pmutex, &attr);
        //pdc->pmutex = (struct avdemux_context*)malloc(sizeof(struct avdemux_context));
        pdc->nvidx = vidx;
        pdc->naidx = aidx;
        if(vidx >= 0)
        {
            pdc->width = pfmtctx->streams[vidx]->codecpar->width;
            pdc->height = pfmtctx->streams[vidx]->codecpar->height;
            pdc->acodec_id = pfmtctx->streams[vidx]->codecpar->codec_id;
            pdc->pix_fmt = pfmtctx->streams[vidx]->codecpar->format;
            pdc->vbitrate = pfmtctx->streams[vidx]->codecpar->bit_rate;
            if(pfmtctx->streams[vidx]->codecpar->extradata && pfmtctx->streams[vidx]->codecpar->extradata_size > 0)
            {
                pdc->pvextra_data = (char*)malloc(pfmtctx->streams[vidx]->codecpar->extradata_size);
                memcpy(pdc->pvextra_data, pfmtctx->streams[vidx]->codecpar->extradata, pfmtctx->streams[vidx]->codecpar->extradata_size);
                pdc->nvextra_data_size = pfmtctx->streams[vidx]->codecpar->extradata_size;
            }
        }
        
        if(aidx >= 0)
        {
            pdc->channel = pfmtctx->streams[aidx]->codecpar->channels;
            pdc->samplerate = pfmtctx->streams[aidx]->codecpar->sample_rate;
            pdc->smp_fmt = pfmtctx->streams[aidx]->codecpar->format;
            pdc->abitrate = pfmtctx->streams[aidx]->codecpar->bit_rate;
            if(pfmtctx->streams[aidx]->codecpar->extradata && pfmtctx->streams[aidx]->codecpar->extradata_size > 0)
            {
                pdc->paextra_data = (char*)malloc(pfmtctx->streams[aidx]->codecpar->extradata_size);
                memcpy(pdc->paextra_data, pfmtctx->streams[aidx]->codecpar->extradata, pfmtctx->streams[aidx]->codecpar->extradata_size);
                pdc->naextra_data_size = pfmtctx->streams[aidx]->codecpar->extradata_size;
            }
        }
        pdc->llduration = pfmtctx->duration;
        pdc->pfmtctx = pfmtctx;
        return pdc;
    }while(0);
    if(pfmtctx)
    {
        avformat_close_input(&pfmtctx);
    }
    
    return NULL;
}

int avdemux_seek(struct avdemux_context* pdc, int64_t seek_pts)
{
    if(pdc)
    {
        int seek_flag = AVSEEK_FLAG_BYTE;
        int ret = av_seek_frame(pdc->pfmtctx, -1, seek_pts, seek_flag);
        return ret;
    }
    
    return -1;
}

int avdemux_set_deliver_callback(struct avdemux_context* pdc, deliver_callback pdev_cb, struct avmuxer_context* pdev_mc)
{
    lbtrace("avdemux_set_deliver_callback(pdc:%p, pdev_cb:%p, pdev_mc:%p)\n", pdc, pdev_cb, pdev_mc);
    if(NULL == pdc)
    {
        lberror("Invalid parameter, pdc:%p\n", pdc);
        return -1;
    }
    pdc->pdev_cb = pdev_cb;
    pdc->pdev_mc = pdev_mc;
    
    return 0;
}

struct AVPacket* avdemux_read_packet(struct avdemux_context* pdc)
{
    if(NULL == pdc || NULL == pdc->pfmtctx)
    {
        lberror("Invalid parameter, pdc:%p, pdc->pfmtctx:%p\n", pdc, pdc ? pdc->pfmtctx : NULL);
        return NULL;
    }
    AVPacket* pkt = av_packet_alloc();
    int ret = av_read_frame(pdc->pfmtctx, pkt);
    if(ret < 0)
    {
        lberror("ret:%d = av_read_frame(pdc->pfmtctx:%p, pkt:%p)\n", ret, pdc->pfmtctx, pkt);
        av_packet_free(&pkt);
        return NULL;
    }

    if(-1 != pkt->pts)
    {
        pkt->pts = av_rescale_q(pkt->pts, pdc->pfmtctx->streams[pkt->stream_index]->time_base, std_tb);
    }

    if(-1 != pkt->dts)
    {
        pkt->dts = av_rescale_q(pkt->dts, pdc->pfmtctx->streams[pkt->stream_index]->time_base, std_tb);
    }

    return pkt;
}

void avdemux_close(struct avdemux_context* pdc)
{
    if(pdc && pdc->pfmtctx)
    {
        if(pdc->pfmtctx)
        {
            pthread_mutex_lock(pdc->pmutex);
            avformat_close_input(&pdc->pfmtctx);
            if(pdc->pvextra_data)
            {
                free(pdc->pvextra_data);
                pdc->pvextra_data = NULL;
                pdc->nvextra_data_size = 0;
            }
            
            if(pdc->paextra_data)
            {
                free(pdc->paextra_data);
                pdc->paextra_data = NULL;
                pdc->naextra_data_size = 0;
            }
            
            pdc->width = 0;
            pdc->height = 0;
            pdc->vbitrate = 0;
            pdc->nvidx = -1;
            pdc->vcodec_id = AV_CODEC_ID_NONE;
            pdc->pix_fmt = AV_PIX_FMT_NB;
            
            pdc->channel = 0;
            pdc->samplerate = 0;
            pdc->smp_fmt = AV_SAMPLE_FMT_NB;
            pdc->abitrate = 0;
            pdc->acodec_id = AV_CODEC_ID_NONE;
            pdc->naidx = -1;
            pthread_mutex_unlock(pdc->pmutex);
        }
    }
}


int avdemux_proc(void* powner)
{
    struct avdemux_context* pdc = (struct avdemux_context*)powner;
    int ret = 0;
    pdc->brun = 1;
    lbtrace("avdemux proc begin, pdc:%p\n", pdc);
    while(pdc)
    {
        struct AVPacket* pkt = avdemux_read_packet(pdc);
        if(NULL == pkt)
        {
            lberror("pkt:%p = avdemux_read_packet(pdc:%p)\n", pkt, pdc);
            break;
        }

        ret = pdc->pdev_cb(pdc->pdev_mc, pdc->pfmtctx->streams[pkt->stream_index]->codecpar->codec_id, pkt->data, pkt->size, pkt->pts);
        
    }
    
    if(pdc->nvidx >= 0)
    {
        ret = pdc->pdev_cb(pdc->pdev_mc, pdc->pfmtctx->streams[pdc->nvidx]->codecpar->codec_id, NULL, 0, 0);
    }
    
    if(pdc->naidx >= 0)
    {
        ret = pdc->pdev_cb(pdc->pdev_mc, pdc->pfmtctx->streams[pdc->naidx]->codecpar->codec_id, NULL, 0, 0);
    }
    pdc->brun = 0;
    avdemux_close(pdc);
    lbtrace("avdemux_proc end, ret:%d\n", ret);
    return ret;
}

int avdemux_start(struct avdemux_context* pdc)
{
    int ret = 0;
    if(NULL == pdc || NULL == pdc->pfmtctx)
    {
        lberror("Invalid parameter, pdc:%p, pdc->pfmtctx:%p\n", pdc, pdc ? pdc->pfmtctx : NULL);
        return -1;
    }
    
    pthread_mutex_lock(pdc->pmutex);
    if(0 == pdc->tid)
    {
        ret = pthread_create(&pdc->tid, NULL, avdemux_proc, pdc);
        lbtrace("ret:%d = pthread_create(&pmuxctx->tid:%0x, NULL, avdemux_proc, pdc)", ret, pdc->tid);
        if(ret != 0)
        {
            lberror("pthread create failed, ret:%d, reason:%s\n", ret, strerror(errno));
            //pthread_mutex_unlock(pmuxctx->pmutex);
            //return ret;
        }
        
        //return ret;
    }
    pthread_mutex_unlock(pdc->pmutex);
    
    return 0;
}

void avdemux_stop(struct avdemux_context* pdc)
{
    if(pdc && pdc->brun)
    {
        pdc->brun = 0;
        lbtrace("avmuxer_stop pthread_join, pdc->tid:%0x\n", pdc->tid);
        pthread_join(pdc->tid, NULL);
        lbtrace("avmuxer_stop after pthread_join\n");
        pdc->tid = 0;
    }
    
}

int avdemux_is_running(struct avdemux_context* pdc)
{
    return pdc->brun;
}

int64_t avdemux_get_duration(struct avdemux_context* pdc)
{
    if(pdc)
    {
        return pdc->llduration;
    }
    
    return -1;
}

void avdemux_close_contextp(struct avdemux_context** ppdc)
{
    if(ppdc && *ppdc)
    {
        struct avdemux_context* pdc = *ppdc;
        if(pdc->brun)
        {
            avdemux_stop(pdc);
        }
        
        if(pdc->pmutex)
        {
            pthread_mutex_destroy(pdc->pmutex);
            free(pdc->pmutex);
            pdc->pmutex = NULL;
        }
        
        free(pdc);
        *ppdc = pdc = NULL;
    }
}

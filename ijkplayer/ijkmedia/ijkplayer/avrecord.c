/*
 * avrecorder.c
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */
#include "config.h"
#include <unistd.h>
#include "avmuxer.h"
#include "avdemux.h"
#include <pthread.h>
#include <stdint.h>
#include "avdev_ctrl.h"
#include "lbsc_util_conf.h"

typedef struct avrecord_context
{
    struct avdemux_context*     pdc;
    struct avmuxer_context*     pmc;
#ifdef ENABLE_WAITING_FOR_KEYFRAME
    struct avdeliver_control_context* pdev_ctrl_ctx;
#endif
} avrec_ctx;

struct avrecord_context* avrecord_open_context(const char* psrc_url, const char* psink_url, const char* pformat, const char* ptmp_url)
{
    lbdebug("avrecord_open_context(psrc_url:%s, psink_url:%s, pformat:%s, ptmp_url:%s)\n", psrc_url, psink_url, pformat, ptmp_url);
    int ret = 0;
    struct avdemux_context* pdc = NULL;
    struct avmuxer_context* pmc = NULL;
    struct avrecord_context* prc = NULL;
    do
    {
        if(psrc_url)
        {
            pdc = avdemux_open_context(psrc_url);
            if(NULL == pdc)
            {
                lberror("pdc:%p = avdemux_open_context(psrc_url:%s) failed\n", pdc, psrc_url);
                break;
            }
        }
        pmc = avmuxer_open_context();
        ret = avmuxer_set_sink(pmc, psink_url, ptmp_url, pformat);
        if(ret < 0)
        {
            lberror("ret:%d = avmuxer_set_sink(pmc, psink_url, ptmp_url, pformat) failed\n", ret);
            break;
        }
        
        prc = (struct avrecord_context*)malloc(sizeof(struct avrecord_context));
        memset(prc, 0, sizeof(struct avrecord_context));
        prc->pdc = pdc;
        prc->pmc = pmc;
        lbtrace("prc:%p = avrecord_open_context\n", prc);
        return prc;
    }while(0);
    
    if(pdc)
    {
        avdemux_close_contextp(&pdc);
    }

    if(pmc)
    {
        avmuxer_close_contextp(&pmc);
    }
    
    return NULL;
}

struct avrecord_context* avrecord_open_contextex(struct avrecord_context** pprc, const char* psrc_url, const char* psink_url, const char* pformat, const char* ptmp_url)
{
    lbdebug("avrecord_open_context(psrc_url:%s, psink_url:%s, pformat:%s, ptmp_url:%s)\n", psrc_url, psink_url, pformat, ptmp_url);
    int ret = 0;
    struct avdemux_context* pdc = NULL;
    struct avmuxer_context* pmc = NULL;
    struct avrecord_context* prc = NULL;
    if(NULL == pprc || NULL == psink_url)
    {
        lberror("Invalid param pprc:%p, psink_url:%p\n", pprc, psink_url);
        return NULL;
    }

    do
    {
        if(psrc_url)
        {
            pdc = avdemux_open_context(psrc_url);
            if(NULL == pdc)
            {
                lberror("pdc:%p = avdemux_open_context(psrc_url:%s) failed\n", pdc, psrc_url);
                break;
            }
        }
        pmc = avmuxer_open_context();
        ret = avmuxer_set_sink(pmc, psink_url, ptmp_url, pformat);
        if(ret < 0)
        {
            lberror("ret:%d = avmuxer_set_sink(pmc, psink_url, ptmp_url, pformat) failed\n", ret);
            break;
        }
        
        prc = (struct avrecord_context*)malloc(sizeof(struct avrecord_context));
        memset(prc, 0, sizeof(struct avrecord_context));
        prc->pdc = pdc;
        prc->pmc = pmc;
        *pprc = prc;
        long id = (long)prc;
        lbtrace("prc:%p = avrecord_open_context, id:%ld, pprc:%p\n", prc, id, pprc);
        return id;
    }while(0);
    
    if(pdc)
    {
        avdemux_close_contextp(&pdc);
    }

    if(pmc)
    {
        avmuxer_close_contextp(&pmc);
    }
    
    return NULL;
}

int avrecord_start(struct avrecord_context* prc, int64_t pts)
{
    int ret = -1;
    if(NULL == prc || NULL == prc->pmc)
    {
        lberror("prc:%p, or prc->pmc:%p is not init\n", prc, prc ? prc->pmc:NULL);
        return -1;
    }
    
    if(prc->pdc)
    {
        if(pts > 0)
        {
            ret = avdemux_seek(prc->pdc, pts);
        }
        avdemux_set_deliver_callback(prc->pdc, avmuxer_deliver_packet, prc->pmc);
        ret = avdemux_start(prc->pdc);
        lbtrace("ret:%d = avdemux_start(prc->pdc)\n", ret);
    }
    
    if(prc->pmc)
    {
        ret = avmuxer_start(prc->pmc);
    }

    return ret;
}

void avrecord_stop(struct avrecord_context* prc, int bcancel)
{
    int ret = -1;
    if(NULL == prc || NULL == prc->pmc)
    {
        lberror("prc:%p, or prc->pmc:%p is not init\n", prc, prc ? prc->pmc:NULL);
        return ;
    }

    if(prc->pdc)
    {
        avdemux_stop(prc->pdc);
        lbtrace("ret:%d = avdemux_start(prc->pdc)\n", ret);
    }
    
    if(prc->pmc)
    {
        avmuxer_stop(prc->pmc, bcancel);
    }
}

int avrecord_is_running(struct avrecord_context* prc)
{
    return avmuxer_is_running(prc ? prc->pmc : NULL);
}

void avrecord_close_contextp(struct avrecord_context** pprc)
{
    struct avrecord_context* prc = NULL;
    if(pprc && *pprc)
    {
        struct avrecord_context* prc = *pprc;
        avrecord_stop(prc, 0);
        
        avdemux_close_contextp(&prc->pdc);
        avmuxer_close_contextp(&prc->pmc);
#ifdef ENABLE_WAITING_FOR_KEYFRAME
        dev_ctrl_close_contextp(&prc->pdev_ctrl_ctx);
#endif
        free(prc);
        prc = *pprc = NULL;
    }
}

int avrecord_get_percent(struct avrecord_context* prc)
{
    if(NULL == prc || NULL == prc->pdc || NULL == prc->pmc)
    {
        lberror("Invalid parameter, prc:%p, prc->pdc:%p, prc->pmc:%p\n", prc, prc ? prc->pdc : NULL, prc ? prc->pmc : NULL);
        return -1;
    }
    
    if(avmuxer_is_eof(prc->pmc))
    {
        return 100;
    }
    
    if(prc && avdemux_get_duration(prc->pdc) > 0)
    {
        int percent = avmuxer_get_duration(prc->pmc) * 100 / avdemux_get_duration(prc->pdc);
        return percent;
    }
    //return avmuxer_get_duration(prc->pmc);
    return 0;
}

int avrecord_deliver_packet(struct avrecord_context* prc, int codec_id, char* pdata, int len, int64_t pts, long framenum)
{
    if(NULL == prc)
    {
        lberror("Invalid parameter, prc:%p\n", prc);
        return -1;
    }

#ifdef ENABLE_WAITING_FOR_KEYFRAME
    if(NULL == prc->pdev_ctrl_ctx)
    {
        prc->pdev_ctrl_ctx = dev_ctrl_open_context();
    }

    if(prc->pdev_ctrl_ctx && is_packet_dropable(prc->pdev_ctrl_ctx, codec_id, pdata, len, pts, framenum))
    {
        lbtrace("wait for key frame, drop packet codec_id:%d, pdata:%p, len:%d, pts:%" PRId64 ", framenum:%ld\n", codec_id, pdata, len, pts, framenum);
        return 0;
    }
#endif
    return avmuxer_deliver_packet(prc->pmc, codec_id, pdata, len, pts);
}

int avrecord_eof(struct avrecord_context* prc)
{
    if(prc && prc->pmc)
    {
        return avmuxer_is_eof(prc->pmc);
    }
    
    return 0;
}

void* avrecord_set_callback(struct avrecord_context* prc, avmuxer_callback cbfunc, void* powner)
{
    void* porg_owner = avmuxer_set_callback(prc->pmc, cbfunc, powner);
    lbdebug("porg_owner:%p = avmuxer_set_callback(prc->pmc:%p, cbfunc:%p, powner:%p)\n", porg_owner, prc->pmc, cbfunc, powner);
    return porg_owner;
}

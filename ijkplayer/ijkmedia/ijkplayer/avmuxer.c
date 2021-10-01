/*
 * avmuxer.c
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019dawson <dawson.wu@sunvalley.com.cn>
 */
#include "config.h"
#include <unistd.h>
#include "avmuxer.h"
#include "libavformat/avformat.h"
#include <pthread.h>
#include <stdint.h>
#include "avcc.h"
#include "audio_suquence_header.h"
#include "list.h"
#include "lbsc_media_parser.h"
#include "lbsc_util_conf.h"
#include "rec_demux.h"
#include "ipc_record.h"
#define FFMPEG_MUXER
typedef struct avmuxer_context
{
    AVFormatContext*            pfmtctx;
    struct list_context*        plist_ctx;
    pthread_mutex_t*            pmutex;
    int                         nidr_come;
    pthread_t                   tid;
    int                         ninterrupt;
    int                         brun;
    int                         nvidx;
    int                         naidx;
    char*                       psink;
    char*                       pdst_path;
    char*                       pformat;
    int                         bvideo_enable;
    int                         baudio_enable;
    int                         bheader_init;
    int64_t                     llmuxer_duration;
    int                         bcancel;
    int                         beof;
    
    // video codec info
    int                         vcodec_id;
    int                         width;
    int                         height;
    int                         pix_fmt;
    int                         vbitrate;
    int                         vtime_scale;
    int                         vtime_num;
    char*                       pvextra_data;
    int                         nvextra_data_size;
    char*                       sps;
    int                         nsps_len;
    char*                       pps;
    int                         npps_len;
    
    // audio codec info
    
    int                         acodec_id;
    int                         channel;
    int                         samplerate;
    int                         smp_fmt;
    int                         abitrate;
    int                         atime_scale;
    int                         atime_num;
    char*                       paextra_data;
    int                         naextra_data_size;
    
    // callback parameter
    void*                       powner;
    avmuxer_callback            pcbfunc;
    lbsc_xvc_ctx*               pxvc;
    AVBSFContext*               pvbsc;
    AVBSFContext*               pabsc;
    
    void*                       pmux_handle;

    // for write raw data file test
#ifdef ENABLE_SDK_CONFIG_FILE
    VAVA_HS_MUX_CTX*            phmc;
    FILE*                       pvideo_file;
    FILE*                       paudio_file;
    FILE*                       pwrite_video;
#endif
} mux_ctx;

struct avmuxer_context* avmuxer_open_context()
{
    struct avmuxer_context* pmuxctx = (struct avmuxer_context*)malloc(sizeof(struct avmuxer_context));
    memset(pmuxctx, 0, sizeof(struct avmuxer_context));
    pmuxctx->plist_ctx = list_context_create(200);
    pmuxctx->vcodec_id = -1;
    pmuxctx->acodec_id = -1;
    pmuxctx->bvideo_enable = 1;
    pmuxctx->baudio_enable = 1;
    pmuxctx->beof = 0;
    pmuxctx->pmutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutexattr_t attr;
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(pmuxctx->pmutex, &attr);
    
#ifdef ENABLE_SDK_CONFIG_FILE

    char rec_path[256];
    if(get_default_string_config("muxer_record_name", rec_path, 256) > 0)
    {
        pmuxctx->pmux_handle = ipc_record_muxer_open(get_format_by_log_path(rec_path), 4);
    }
#endif
    sv_trace("pmuxctx:%p, sizeof(struct avmuxer_context*):%ld, sizeof(long):%ld\n", pmuxctx, sizeof(struct avmuxer_context*), sizeof(long));
    return pmuxctx;
}

int avmuxer_set_sink(struct avmuxer_context* pmuxctx, const char* pdst_path, const char* ptmp_url, char* pformat)
{
    sv_trace("pmuxctx:%p, pdst_path:%s, ptmp_url:%s, pformat:%s\n", pmuxctx, pdst_path, ptmp_url, pformat);
    if(NULL == pmuxctx || NULL == pdst_path)
    {
        lberror("Invalid pameter pmuxctx:%p, pdstpath:%s\n", pmuxctx, pdst_path);
        return -1;
    }
    if(NULL == ptmp_url)
    {
        sv_trace("after NULL == ptmp_url\n");
        ptmp_url = pdst_path;
        pmuxctx->pdst_path = NULL;
        sv_trace("after pmuxctx->pdst_path = NULL\n");
    }
    else
    {
        sv_trace("pmuxctx:%p->pdst_path = malloc(strlen(pdst_path)+1)\n", pmuxctx);
        int pathlen = strlen(pdst_path) + 1;
        sv_trace("pathlen:%d\n", pathlen);
        char* pcopy_path = (char*)malloc(pathlen);
        sv_trace("strcpy(pmuxctx->pdst_path, pdst_path);\n");
        memcpy(pcopy_path, pdst_path, pathlen);
        sv_trace("before pmuxctx->pdst_path = pcopy_path\n");
        pmuxctx->pdst_path = pcopy_path;
        sv_trace("after pmuxctx->pdst_path = pcopy_path\n");
    }
    sv_trace("ptmp_url:%s, pmuxctx->pdst_path:%s\n", ptmp_url, pmuxctx->pdst_path);

    if(pmuxctx->psink)
    {
        free(pmuxctx->psink);
        pmuxctx->psink = NULL;
    }
    sv_trace("after free sink\n");
    pmuxctx->psink = (char*)malloc(strlen(ptmp_url)+1);
    memcpy(pmuxctx->psink, ptmp_url, strlen(ptmp_url)+1);
    sv_trace("pmuxctx->psink:%s, pmuxctx->pdst_path:%s\n", pmuxctx->psink, pmuxctx->pdst_path);
    if(pformat)
    {
        if(pmuxctx->pformat)
        {
            free(pmuxctx->pformat);
        }
        pmuxctx->pformat = (char*)malloc(strlen(pformat)+1);
        memcpy(pmuxctx->pformat, pformat, strlen(pformat)+1);
    }
    sv_trace("end, pmuxctx->pformat:%s\n", pmuxctx->pformat);
    return 0;
}

int avmuxer_start_context(struct avmuxer_context** ppmuxctx, const char* pdst_path, const char* ptmp_url, char* pformat)
{
    lbtrace("(ppmuxctx:%p, pdst_path:%s, ptmp_url:%s,pformat:%s\n", ppmuxctx, pdst_path, ptmp_url,pformat);
    struct avmuxer_context* pmuxctx = NULL;
    long muxerid = 0;
    int ret = -1;
    if(NULL == ppmuxctx || NULL == pdst_path)
    {
        lbtrace("NULL == ppmuxctx:%p || NULL == pdst_path:%p, invalid parameter!\n", ppmuxctx, pdst_path);
        return -1;
    }
    pmuxctx = *ppmuxctx;
    do{
        if(NULL == pmuxctx)
        {
            pmuxctx = avmuxer_open_context();
            lbtrace("pmuxctx:%p = avmuxer_open_context()\n", pmuxctx);
        }
        ret = avmuxer_set_sink(pmuxctx, pdst_path, ptmp_url, pformat);
        if(ret < 0)
        {
            break;
        }
        ret = avmuxer_start(pmuxctx);
        if(ret < 0)
        {
            break;
        }
        *ppmuxctx = pmuxctx;
        lbtrace("*ppmuxctx:%p,  pmuxctx:%p\n", *ppmuxctx, pmuxctx);
    }while(0);
    if(ret < 0 && NULL == *ppmuxctx)
    {
        avmuxer_close_contextp(&pmuxctx);
    }

    lbtrace("*ppmuxctx:%p, ret:%d\n", *ppmuxctx, ret);
    return ret;
}

void* avmuxer_set_callback(struct avmuxer_context* pmuxctx, avmuxer_callback cbfunc, void* powner)
{
    if(NULL == pmuxctx)
    {
        lberror("%s(pmuxctx:%p, cbfunc:%p, powner:%p), invalid param!\n", __func__, pmuxctx, cbfunc, powner);
        return -1;
    }

    void* org_owner = pmuxctx->powner;
    pmuxctx->pcbfunc = cbfunc;
    pmuxctx->powner = powner;
    lbtrace("%s(pmuxctx:%p, pmuxctx->pcbfunc:%p, pmuxctx->powner:%p, org_owner:%p)", __func__, pmuxctx, pmuxctx->pcbfunc, pmuxctx->powner, org_owner);
    return org_owner;
}

void avmuxer_close_contextp(struct avmuxer_context** ppmuxctx)
{
    if(ppmuxctx && *ppmuxctx)
    {
        struct avmuxer_context* pmuxctx = *ppmuxctx;
        pthread_mutex_lock(pmuxctx->pmutex);
#ifdef ENABLE_SDK_CONFIG_FILE
        if(pmuxctx->pmux_handle)
        {
            ipc_record_muxer_close(&pmuxctx->pmux_handle);
        }
#endif
        if(pmuxctx->plist_ctx)
        {
             (&pmuxctx->plist_ctx);
        }
        
        if(pmuxctx->pfmtctx)
        {
            avformat_free_context(pmuxctx->pfmtctx);
            pmuxctx->pfmtctx = NULL;
        }
        
        if(pmuxctx->psink)
        {
            free(pmuxctx->psink);
            pmuxctx->psink = NULL;
        }
        
        if(pmuxctx->pdst_path)
        {
            free(pmuxctx->pdst_path);
            pmuxctx->pdst_path = NULL;
        }
#ifdef ENABLE_WRITE_MUXER_RAW_DATA
        if(pmuxctx->pvideo_file)
        {
            fclose(pmuxctx->pvideo_file);
            pmuxctx->pvideo_file = NULL;
        }
        
        if(pmuxctx->paudio_file)
        {
            fclose(pmuxctx->paudio_file);
            pmuxctx->paudio_file = NULL;
        }
        
        if(pmuxctx->pwrite_video)
        {
            fclose(pmuxctx->pwrite_video);
            pmuxctx->pwrite_video = NULL;
        }
#endif
        pthread_mutex_destroy(pmuxctx->pmutex);
        free(pmuxctx->pmutex);
        pmuxctx->pmutex = NULL;

        free(pmuxctx);
        *ppmuxctx = pmuxctx = NULL;
    }
}
int avmuxer_add_stream(struct avmuxer_context* pmuxctx, int mediatype, int codec_id, int param1, int param2, int format, int bitrate, char* pextra_data, int extra_data_size)
{
    if(NULL == pmuxctx)
    {
        lberror("avmuxer_add_stream failed, invalid parameter pmuxctx:%p\n", pmuxctx);
        return -1;
    }
    
    if(1 == pmuxctx->bheader_init)
    {
        lberror("muxer has already init\n");
        return -1;
    }
    pthread_mutex_lock(pmuxctx->pmutex);
    if(AVMEDIA_TYPE_VIDEO == mediatype)
    {
        pmuxctx->vcodec_id  = codec_id;
        pmuxctx->width      = param1;
        pmuxctx->height     = param2;
        pmuxctx->pix_fmt    = format;
        pmuxctx->vbitrate   = bitrate;
        if(pextra_data && extra_data_size > 0)
        {
            pmuxctx->pvextra_data = (char*)malloc(extra_data_size);
            memcpy(pmuxctx->pvextra_data, pextra_data, extra_data_size);
            pmuxctx->nvextra_data_size = extra_data_size;
            sv_trace("size:%d pextra_data:", extra_data_size);
            sv_memory(3, pextra_data, extra_data_size, "pextra_data:");
        }
    }
    else if(AVMEDIA_TYPE_AUDIO == mediatype)
    {
        pmuxctx->acodec_id      = codec_id;
        pmuxctx->channel        = param1;
        pmuxctx->samplerate     = param2;
        pmuxctx->smp_fmt        = format;
        pmuxctx->abitrate       = bitrate;
        if(pextra_data && extra_data_size > 0)
        {
            pmuxctx->paextra_data = (char*)malloc(extra_data_size);
            memcpy(pmuxctx->paextra_data, pextra_data, extra_data_size);
            pmuxctx->naextra_data_size = extra_data_size;
        }
    }
    pthread_mutex_unlock(pmuxctx->pmutex);
    return 0;
}

int deliver_packet(struct avmuxer_context* pmc, int codec_id, char* pdata, int data_len, int keyflag, int64_t pts)
{
    if(NULL == pmc)
    {
        lberror("avmuxer_deliver_packet failed, invalid parameter pmc:%p\n", pmc);
        return -1;
    }
    pthread_mutex_lock(pmc->pmutex);
    AVPacket* pkt = av_packet_alloc();
    if(pdata && data_len > 0)
    {
        av_new_packet(pkt, data_len);
        memcpy(pkt->data, pdata, data_len);
    }
    else
    {
        lbtrace("codec_id:%d deliver end of stream\n", codec_id);
    }
    pkt->flags = keyflag;
    pkt->pts = pts;
    pkt->dts = pts;
    pkt->stream_index = codec_id >= 10000 ? 1 : 0;
    push(pmc->plist_ctx, pkt);
    pthread_mutex_unlock(pmc->pmutex);
    
    return 0;
}
#if 0
int avmuxer_deliver_packet(struct avmuxer_context* pmc, int codec_id, char* pdata, int data_len, int64_t pts)
{
    int ret = -1;
    int mediatype = get_media_type_from_codec_id(codec_id);
    int keyflag = 0;
    
    if(NULL == pmc)
    {
        lberror("avmuxer_deliver_packet failed, invalid parameter pmc:%p\n", pmc);
        return -1;
    }

    if(pmc->ninterrupt)
    {
        lberror("drop packet because amuxer has been interrupt, , pmuxctx->ninterrupt:%d\n", pmc->ninterrupt);
        return -1;
    }

    pthread_mutex_lock(pmc->pmutex);
    do{
        if(AVMEDIA_TYPE_VIDEO == mediatype && pmc->bvideo_enable && pdata)
        {
            if(AV_CODEC_ID_H264 == codec_id || AV_CODEC_ID_HEVC == codec_id)
            {
                int streamtype = AV_CODEC_ID_H264 == codec_id ? 4 : 5;
                
                keyflag = is_idr_frame(streamtype, pdata, data_len);
                if(keyflag && pmc->vcodec_id < 0)
                {
                    char avcc[1024];
                    int avcclen = 0;
                    int width = 0, height = 0;
                    if(NULL == pmc->pxvc)
                    {
                        pmc->pxvc = lbsc_open_xvc_context(streamtype);
                        if(NULL == pmc->pxvc)
                        {
                            lberror("pmc->pxvc:%p = lbsc_open_xvc_context, codec_id:%d\n", pmc->pxvc, codec_id);
                            ret = -1;
                            break;
                        }
                    }
                    ret = parse_stream(pmc->pxvc, pdata, data_len);
                    if(ret < 0)
                    {
                        lberror("ret:%d = parse_stream(pmuxctx->pxvc:%p, pdata:%p, data_len:%d) failed\n", ret, pmc->pxvc, pdata, data_len);
                        break;
                    }
                    width = pmc->pxvc->m_nwidth;
                    height = pmc->pxvc->m_nheight;
                    avcclen = lbsc_get_sequence_header(pmc->pxvc, avcc, 1024);
                    ret = avmuxer_add_stream(pmc, AVMEDIA_TYPE_VIDEO, codec_id, width, height, AV_PIX_FMT_YUV420P, 1024*1024, avcc, avcclen);
                    if(ret < 0)
                    {
                        lberror("ret:%d = avmuxer_add_stream(pmc:%p, AVMEDIA_TYPE_VIDEO, codec_id:%d, width:%d, height:%d, AV_PIX_FMT_YUV420P, 1024*1024, avcc, avcclen:%d) failed\n", ret, pmc, codec_id, width, height, avcclen);
                        break;
                    }
                    //ret = deliver_packet(pmc, codec_id, pdata, data_len, keyflag, pts);
                }
            }
            else
            {
                //...
                lberror("not support video codec_id:%d\n", codec_id);
            }
        }
        else if(AVMEDIA_TYPE_AUDIO == mediatype && pmc->baudio_enable && pdata)
        {
            if(pmc->acodec_id < 0)
            {
                if(AV_CODEC_ID_AAC == codec_id)
                {
                    char sh[256];
                    adts_ctx* pac = adts_demux_open(pdata, data_len);
                    int sh_len = mux_sequence_header(pac, sh, 256);
                    if(sh_len <= 0)
                    {
                        lberror("adts header mux sequence header failed, sh_len:%d\n", sh_len);
                        adts_demux_close(&pac);
                        break;
                    }
                    ret = avmuxer_add_stream(pmc, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_AAC, pac->channel_configuration, pac->samplerate, AV_SAMPLE_FMT_S32, 128, sh, sh_len);
                    adts_demux_close(&pac);
                    if(ret < 0)
                    {
                        lberror("ret:%d = avmuxer_add_stream(pmc:%p, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_AAC, pac->channel_configuration:%d, pac->samplerate:%d, AV_SAMPLE_FMT_S32, 128, sh, sh_len:%d) failed\n", ret, pmc, pac->channel_configuration, pac->samplerate, sh_len);
                        break;
                    }
                }
                else
                {
                    //...
                    lberror("not support audio codec_id:%d\n", codec_id);
                }
                if(pmc->bvideo_enable && pmc->vcodec_id < 0)
                {
                    ret = 1;
                    lberror("video key frame not come, drop audio frame\n");
                    break;
                }
            }
            keyflag = 1;
        }
        ret = deliver_packet(pmc, codec_id, pdata, data_len, keyflag, pts);
        /*AVPacket* pkt = av_packet_alloc();
        if(pdata && data_len > 0)
        {
            av_new_packet(pkt, data_len);
            memcpy(pkt->data, pdata, data_len);
        }
        
        pkt->flags = keyflag;
        pkt->pts = pts;
        pkt->dts = pts;
        pkt->stream_index = mediatype;
        push(pmc->plist_ctx, pkt);*/
        pthread_mutex_unlock(pmc->pmutex);
        return ret;
    }while(0);
    
    pthread_mutex_unlock(pmc->pmutex);
    lberror("avmuxer_deliver_packet failed, ret:%d\n", ret);
    return ret;
}
#else
int avmuxer_deliver_packet(struct avmuxer_context* pmc, int codec_id, char* pdata, int data_len, int64_t pts)
{
    int ret = -1;
    int key_flag = 1;
    int ipc_codec_id = -1;
    int mediatype = get_media_type_from_codec_id(codec_id);
    int keyflag = 0;
    if(NULL == pmc)
    {
        lberror("avmuxer_deliver_packet failed, invalid parameter pmc:%p\n", pmc);
        return -1;
    }
#ifdef ENABLE_SDK_CONFIG_FILE
    if(codec_id < 10000 && pdata)
    {
        key_flag = is_idr_frame(AV_CODEC_ID_H264 == codec_id ? 4 : 5, pdata, data_len);
        if(AV_CODEC_ID_H264 == codec_id)
        {
            ipc_codec_id = eipc_codec_id_h264;
        }
        else
        {
            ipc_codec_id = eipc_codec_id_h265;
        }
    }
    else
    {
        ipc_codec_id = eipc_codec_id_aac;
    }
    ipc_record_muxer_write_packet(pmc->pmux_handle, ipc_codec_id, pdata, data_len, pts/1000, key_flag);
#endif
    if(pmc->ninterrupt)
    {
        lberror("drop packet because amuxer has been interrupt, , pmuxctx->ninterrupt:%d\n", pmc->ninterrupt);
        return -1;
    }
    
    if(NULL == pdata || data_len <= 0)
    {
        return deliver_packet(pmc, codec_id, pdata, data_len, keyflag, pts);
    }

    pthread_mutex_lock(pmc->pmutex);
    do{
        if(AVMEDIA_TYPE_VIDEO == mediatype && pmc->bvideo_enable)
        {
            if(AV_CODEC_ID_H264 == codec_id || AV_CODEC_ID_HEVC == codec_id)
            {
                int streamtype = AV_CODEC_ID_H264 == codec_id ? 4 : 5;
                keyflag = is_idr_frame(streamtype, pdata, data_len);
                if(keyflag)
                {
                    if(NULL == pmc->pxvc)
                    {
                        pmc->pxvc = lbsc_open_xvc_context(streamtype);
                    }
                    ret = parse_stream(pmc->pxvc, pdata, data_len);
                    if(ret < 0)
                    {
                        lberror("ret:%d = parse_stream(pmuxctx->pxvc:%p, pdata:%p, data_len:%d) failed\n", ret, pmc->pxvc, pdata, data_len);
                        sv_memory(3, pdata, data_len, "parser h26x stream failed\n, invalid stream data:");
                        break;
                    }
                    
                    if(pmc->vcodec_id < 0)
                    {
                        char sh[1024];
                        int sh_len = lbsc_get_sequence_header(pmc->pxvc, sh, 1024);
                        ret = avmuxer_add_stream(pmc, AVMEDIA_TYPE_VIDEO, codec_id, pmc->pxvc->m_nwidth, pmc->pxvc->m_nheight, AV_PIX_FMT_YUV420P, 1024*1024, sh, sh_len);
                        if(ret < 0)
                        {
                            lberror("ret:%d = avmuxer_add_stream(pmc:%p, AVMEDIA_TYPE_VIDEO, codec_id:%d, width:%d, height:%d, AV_PIX_FMT_YUV420P, 1024*1024, sh, sh_len:%d) failed\n", ret, pmc, codec_id, pmc->pxvc->m_nwidth, pmc->pxvc->m_nheight, sh_len);
                            break;
                        }
                    }
                    else if(pmc->width != pmc->pxvc->m_nwidth || pmc->height != pmc->pxvc->m_nheight)
                    {
                        lberror("video resolution change, orgin width:%d, height:%d, new width:%d, new height:%d, interrupt muxer\n", pmc->width, pmc->height, pmc->pxvc->m_nwidth, pmc->pxvc->m_nheight);
                        sv_memory(3, pmc->pvextra_data, pmc->nvextra_data_size, "orgin extra data:");
                        pmc->ninterrupt = INTERRUPT_REASON_RES_CHANGE;
                        avmuxer_callback_notifiy(pmc, AVMUXER_MSG_INTERRUPT, 0, pmc->ninterrupt);
                        return -1;
                    }
                }
            }
            else
            {
                //...
                lberror("not support video codec_id:%d\n", codec_id);
            }
        }
        else if(AVMEDIA_TYPE_AUDIO == mediatype && pmc->baudio_enable && pdata)
        {
            if(pmc->acodec_id < 0)
            {
                if(AV_CODEC_ID_AAC == codec_id)
                {
                    char sh[256];
                    adts_ctx* pac = adts_demux_open(pdata, data_len);
                    int sh_len = mux_sequence_header(pac, sh, 256);
                    if(sh_len <= 0)
                    {
                        lberror("adts header mux sequence header failed, sh_len:%d\n", sh_len);
                        adts_demux_close(&pac);
                        break;
                    }
                    ret = avmuxer_add_stream(pmc, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_AAC, pac->channel_configuration, pac->samplerate, AV_SAMPLE_FMT_S32, 128, sh, sh_len);
                    adts_demux_close(&pac);
                    if(ret < 0)
                    {
                        lberror("ret:%d = avmuxer_add_stream(pmc:%p, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_AAC, pac->channel_configuration:%d, pac->samplerate:%d, AV_SAMPLE_FMT_S32, 128, sh, sh_len:%d) failed\n", ret, pmc, pac->channel_configuration, pac->samplerate, sh_len);
                        break;
                    }
                }
                else
                {
                    //...
                    lberror("not support audio codec_id:%d\n", codec_id);
                }
                if(pmc->bvideo_enable && pmc->vcodec_id < 0)
                {
                    ret = 1;
                    lberror("video key frame not come, drop audio frame\n");
                    break;
                }
            }
            keyflag = 1;
        }
        ret = deliver_packet(pmc, codec_id, pdata, data_len, keyflag, pts);
        pthread_mutex_unlock(pmc->pmutex);
        return ret;
    }while(0);
    
    pthread_mutex_unlock(pmc->pmutex);
    lberror("avmuxer_deliver_packet failed, ret:%d\n", ret);
    return ret;
}
#endif
int avmuxer_is_stream_prepare(struct avmuxer_context* pmuxctx)
{
    if(!pmuxctx)
    {
        return 0;
    }
    if(pmuxctx->pfmtctx)
    {
        assert(0);
        return 1;
    }
    if(list_size(pmuxctx->plist_ctx) > MAX_PACKET_BUFFER_COUNT)
    {
        return pmuxctx->vcodec_id >= 0 || pmuxctx->acodec_id >= 0;
    }
    if(pmuxctx->bvideo_enable && pmuxctx->baudio_enable)
    {
        return pmuxctx->vcodec_id >= 0 && pmuxctx->acodec_id >= 0;
    }
    else if(pmuxctx->bvideo_enable)
    {
        return pmuxctx->vcodec_id >= 0;
    }
    else if(pmuxctx->baudio_enable)
    {
        return pmuxctx->acodec_id >= 0;
    }
    else
    {
        return 0;
    }
}

int get_file_name(const char* ppath, char* name, int len)
{
    if(NULL == ppath || NULL == name || len <= 0)
    {
        lberror("Invalid parameter ppath:%s, name:%s\n", ppath, name);
        return -1;
    }
    int namelen = 0;
    char* pname = strrchr(ppath, '/');
    if(NULL == pname)
    {
        pname = ppath;
    }
    char* pext = strchr(pname, '.');
    if(NULL == pext)
    {
        pext = pname + strlen(pname);
    }
    namelen = pext - pname;
    memset(name, 0, len);
    memcpy(name, pname, namelen);
    lbtrace("ppath:%s, pname:%s, pext:%d, name:%s\n", ppath, pname, pext, name);
    return namelen;
}

void* muxer_proc(void* powner)
{
    struct avmuxer_context* pmuxctx = (struct avmuxer_context*)powner;
    int ret = 0;
    assert(pmuxctx);
    //assert(pmuxctx->pfmtctx);
    assert(pmuxctx->plist_ctx);
    list_ctx* plist = pmuxctx->plist_ctx;
    pmuxctx->brun = 1;
    int64_t start_pts = AV_NOPTS_VALUE;
    //int64_t astart_pts = AV_NOPTS_VALUE;
    int veof = 0;
    int aeof = 0;
    int64_t cur_pts = AV_NOPTS_VALUE;
    int64_t lllast_vpts = AV_NOPTS_VALUE;
    int64_t lllast_vdts = AV_NOPTS_VALUE;
    int64_t lllast_apts = AV_NOPTS_VALUE;
    int64_t lllast_adts = AV_NOPTS_VALUE;
    lbtrace("muxer_proc begin\n");
    while(pmuxctx->brun && !pmuxctx->ninterrupt)
    {
        //lbtrace("while, list_size(plist):%d\n", list_size(plist));
        if(list_size(plist) <= 0)
        {
            if(veof && aeof)
            {
                lbtrace("media muxer end of stream\n");
                break;
            }
            usleep(200000);
            continue;
        }
        
        if(!pmuxctx->pfmtctx)
        {
            //lbtrace("avmuxer_is_stream_prepare\n");
            if(avmuxer_is_stream_prepare(pmuxctx))
            {
                //lbtrace("avmuxer_open(pmuxctx:%p)\n", pmuxctx);
                int ret = avmuxer_open(pmuxctx);
                lbtrace("ret:%d = avmuxer_open(pmuxctx)\n", ret);
                if(ret < 0)
                {
                    avmuxer_callback_notifiy(pmuxctx, AVMUXER_MSG_ERROR, 0, ret);
                    avmuxer_close(pmuxctx);
                    lberror("avmuxer_open(pmuxctx) failed ret:%d\n", ret);
                    break;
                }
                avmuxer_callback_notifiy(pmuxctx, AVMUXER_MSG_START, 0, 0);
            }
            else
            {
                usleep(20000);
                continue;
            }
        }
        
        AVRational tb;
        tb.den = 1000000;
        tb.num = 1;
        int64_t pts = AV_NOPTS_VALUE, dts = AV_NOPTS_VALUE;
        AVPacket* pkt = pop(plist);

        if(pkt)
        {
            if(NULL == pkt->data)
            {
                if(AVMEDIA_TYPE_VIDEO == pkt->stream_index)
                {
                    veof = 1;
                    av_packet_free(&pkt);
                    continue;
                }
                else if(AVMEDIA_TYPE_AUDIO == pkt->stream_index)
                {
                    aeof = 1;
                    av_packet_free(&pkt);
                    continue;
                }
            }
            
            
            pts = pkt->pts;
            dts = pkt->dts;
            if(AVMEDIA_TYPE_VIDEO == pkt->stream_index && pmuxctx->vcodec_id >= 0)
            {
                pkt->stream_index = pmuxctx->nvidx;
            }
            else if(AVMEDIA_TYPE_AUDIO == pkt->stream_index && pmuxctx->acodec_id >= 0)
            {
                pkt->stream_index = pmuxctx->naidx;
            }
            else
            {
                //assert(0);
                lbtrace("drop packet idx:%d, pts:%"PRId64", size:%d\n", pkt->stream_index, pkt->pts, pkt->size);
                av_packet_free(&pkt);
                continue;
            } 

            if(AV_NOPTS_VALUE == start_pts)
            {
                start_pts = pkt->pts;
            }
            if(AV_NOPTS_VALUE != pkt->pts)
            {
                pkt->pts -= start_pts;
            }
            if(AV_NOPTS_VALUE != pkt->dts)
            {
                pkt->dts -= start_pts;
            }
            if(pkt->dts > pkt->pts)
            {
                assert(0);
            }
            pmuxctx->llmuxer_duration = pkt->pts > pmuxctx->llmuxer_duration ? pkt->pts : pmuxctx->llmuxer_duration;
            if (AV_NOPTS_VALUE != pkt->pts)
            {
                pts = pkt->pts;
                pkt->pts = av_rescale_q(pkt->pts, tb, pmuxctx->pfmtctx->streams[pkt->stream_index]->time_base);
            }

            if (AV_NOPTS_VALUE != pkt->dts)
            {
                pkt->dts = av_rescale_q(pkt->dts, tb, pmuxctx->pfmtctx->streams[pkt->stream_index]->time_base);
            }
            if(AVMEDIA_TYPE_VIDEO == pkt->stream_index)
            {
                if(lllast_vdts >= pkt->dts)
                {
                    pkt->dts = lllast_vpts + 1;
                    pkt->pts = lllast_vdts + 1;
                }
                lllast_vpts = pkt->pts;
                lllast_vdts = pkt->dts;
            }
            if(AVMEDIA_TYPE_AUDIO == pkt->stream_index)
            {
                if(lllast_adts >= pkt->dts)
                {
                    pkt->dts = lllast_apts + 1;
                    pkt->pts = lllast_adts + 1;
                }
                
                lllast_apts = pkt->pts;
                lllast_adts = pkt->dts;
            }
            
            if(pkt->dts > pkt->pts)
            {
                assert(0);
            }
            lbtrace("pts:%"PRId64", dts:%"PRId64", pkt->pts:%"PRId64", pkt->dts:%"PRId64", pkt->size:%d, pkt->index:%d\n", pts, dts, pkt->pts, pkt->dts, pkt->size, pkt->stream_index);

            ret = av_interleaved_write_frame(pmuxctx->pfmtctx, pkt);
            lbtrace("ret:%d = av_interleaved_write_frame(pmuxctx->pfmtctx, pkt)\n", ret);
            av_packet_free(&pkt);
            if(ret < 0)
            {
                avmuxer_callback_notifiy(pmuxctx, AVMUXER_MSG_ERROR, 0, ret);
                lberror("write frame failed, ret:%d\n", ret);
                break;
            }
            else
            {
                if(cur_pts < pts)
                {
                    cur_pts = pts;
                    avmuxer_callback_notifiy(pmuxctx, AVMUXER_MSG_PROGRESS, 0, cur_pts/1000);
                }
            }
        }
    }

    if(pmuxctx->pfmtctx)
    {
        ret = av_write_trailer(pmuxctx->pfmtctx);
        if(pmuxctx->bcancel)
        {
            remove(pmuxctx->pdst_path);
            lbtrace("avmuxer_callback_notifiy: AVMUXER_MSG_COMPLETE\n");
            avmuxer_callback_notifiy(pmuxctx, AVMUXER_MSG_COMPLETE, pmuxctx->ninterrupt, 0);
            lbtrace("user cancel muxer, delete and exit, pmuxctx->bcancel:%d\n", pmuxctx->bcancel);
            pmuxctx->bcancel = 0;
        }
        else if(0 == ret)
        {
            if(pmuxctx->pdst_path)
            {
                move_file(pmuxctx->psink, pmuxctx->pdst_path, 0);
#if DEBUG
                char name[256];
                int namelen = get_file_name(pmuxctx->pdst_path, name, 256);
                if(namelen > 0)
                {
                    char copypath[256];
                    sprintf(copypath, "%s/%s_copy.mp4", get_log_path(),name);
                    move_file(pmuxctx->pdst_path, copypath, 1);
                }
#endif
            }
            lbtrace("avmuxer_callback_notifiy begin\n");
            avmuxer_callback_notifiy(pmuxctx, AVMUXER_MSG_COMPLETE, pmuxctx->ninterrupt, cur_pts/1000);
            lbtrace("muxer success, move mp4 file from %s to %s\n", pmuxctx->psink, pmuxctx->pdst_path);
        }
        else
        {
            remove(pmuxctx->pdst_path);
            avmuxer_callback_notifiy(pmuxctx, AVMUXER_MSG_ERROR, 0, ret);
            lbtrace("muxer error, remove dst file %s\n", pmuxctx->pdst_path);
        }

    }
    lbtrace("muxer_proc end, ret:%d, pmuxctx->binterrupt:%d, list_size(plist):%d\n", ret, pmuxctx->ninterrupt, list_size(plist));
    pmuxctx->brun = 0;
    pmuxctx->beof = 1;
    //avmuxer_callback_notifiy(pmuxctx, AVMUXER_MSG_INTERRUPT, 0, pmuxctx->ninterrupt);
    return ret;
}
int avmuxer_start(struct avmuxer_context* pmuxctx)
{
    if(NULL == pmuxctx)
    {
        lbtrace("Invalid pameter, pmuxctx:%p\n", pmuxctx);
        return -1;
    }
    int ret = -1;
    pthread_mutex_lock(pmuxctx->pmutex);
    if(0 == pmuxctx->tid)
    {
        ret = pthread_create(&pmuxctx->tid, NULL, muxer_proc, pmuxctx);
        lbtrace("ret:%d = pthread_create(&pmuxctx->tid:%0x, NULL, muxer_proc, pmuxctx)", ret, pmuxctx->tid);
        if(ret != 0)
        {
            lberror("pthread create failed, ret:%d, reason:%s\n", ret, strerror(errno));
            //pthread_mutex_unlock(pmuxctx->pmutex);
            //return ret;
        }
        
        //return ret;
    }
    pthread_mutex_unlock(pmuxctx->pmutex);
    return ret;
}

void avmuxer_stop(struct avmuxer_context* pmuxctx, int bcancel)
{
    lbtrace("avmuxer_stop begin, pmuxctx:%p\n", pmuxctx);
    if(pmuxctx && pmuxctx->brun)
    {
        int wiat_num = 0;
		// wait for packet muxer end
        while(pmuxctx->brun && wiat_num++ < 20 && list_size(pmuxctx->plist_ctx) > 0)
        {
            lbtrace("avmuxer_top usleep(100), wiat_num:%d\n", wiat_num);
            usleep(20000);
        }
        pmuxctx->bcancel = bcancel;
        pmuxctx->brun = 0;
        lbtrace("avmuxer_stop pthread_join, pmuxctx->tid:%0x\n", pmuxctx->tid);
        pthread_join(pmuxctx->tid, NULL);
        lbtrace("avmuxer_stop after pthread_join\n");
        pmuxctx->tid = 0;
        
    }
    lbtrace("avmuxer_stop avmuxer_close\n");
    avmuxer_close(pmuxctx);
    lbtrace("avmuxer_stop end\n");
}

int avmuxer_is_running(struct avmuxer_context* pmuxctx)
{
    return pmuxctx ? pmuxctx->brun : 0;
}

int avmuxer_create_stream(struct avmuxer_context* pmuxctx, int mediatype, int codec_id, int param1, int param2, int format, int bitrate, char* pextra_data, int extra_data_size)
{
    if(NULL == pmuxctx || NULL == pmuxctx->pfmtctx)
    {
        lberror("avmuxer_create_stream Invalid parameter, pmuxctx:%p, pfmtctx:%p\n", pmuxctx, pmuxctx ? pmuxctx->pfmtctx : NULL);
        return -1;
    }
    AVFormatContext* pfmtctx = pmuxctx->pfmtctx;
    AVCodec* pcodec = avcodec_find_decoder(codec_id);
    AVStream* pst = avformat_new_stream(pfmtctx, pcodec);
    if(NULL == pst)
    {
        lberror("create new stream failed, pst:%p\n", pst);
        return -1;
    }
    if(pextra_data)
    {
        pst->codecpar->extradata = (uint8_t*)av_malloc(extra_data_size);
        memcpy(pst->codecpar->extradata, pextra_data, extra_data_size);
        pst->codecpar->extradata_size = extra_data_size;
    }
    if(AVMEDIA_TYPE_VIDEO == mediatype)
    {
        pst->id = pfmtctx->nb_streams - 1;
        pst->time_base.den = 1;
        pst->time_base.num = 15;
        pst->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
        if(AV_CODEC_ID_HEVC == codec_id)
        {
            pst->codecpar->codec_tag = MKTAG('h', 'v', 'c', '1');
        }
        pst->codecpar->codec_id = codec_id;
        pst->codecpar->bit_rate = bitrate;
        pmuxctx->width = pst->codecpar->width = param1;
        pmuxctx->height = pst->codecpar->height = param2;
        pst->codecpar->format = AV_PIX_FMT_YUV420P;
    }
    else if(AVMEDIA_TYPE_AUDIO == mediatype)
    {
        pst->id = pfmtctx->nb_streams - 1;
        /*pst->pts.val = 0;
        pst->pts.den = 1000;
        pst->pts.num = 1;*/
        pst->time_base.den = 1;
        pst->time_base.num = 1000;
        pst->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
        pst->codecpar->codec_id = codec_id;
        pst->codecpar->bit_rate = bitrate;
        pst->codecpar->channels = param1;
        pst->codecpar->sample_rate = param2;
        pst->codecpar->format = AV_SAMPLE_FMT_S32;
    }
    else
    {
        lberror("mediatype %d not support\n", mediatype);
        return -1;
    }
    
    return pst->id;
}

int avmuxer_open(struct avmuxer_context* pmuxctx)
{
    if(NULL == pmuxctx)
    {
        lberror("avmuxer prepare invalid parameter, pmuxctx:%p\n", pmuxctx);
        return -1;
    }
    int ret = 0;
    pthread_mutex_lock(pmuxctx->pmutex);
    do
    {
        
        assert(NULL == pmuxctx->pfmtctx);
        AVOutputFormat* poutfmt = av_guess_format(pmuxctx->pformat, pmuxctx->psink, NULL);
        ret = avformat_alloc_output_context2(&pmuxctx->pfmtctx, poutfmt, pmuxctx->pformat, pmuxctx->psink);
        lbtrace("ret:%d = avformat_alloc_output_context2(&pmuxctx->pfmtctx:%p, poutfmt:%p, pmuxctx->pformat:%s, pmuxctx->psink:%s)\n", ret, pmuxctx->pfmtctx, poutfmt, pmuxctx->pformat, pmuxctx->psink);
        if(ret < 0 || NULL == pmuxctx->pfmtctx)
        {
            lberror("create output context failed, ret:%d, pfmtctx:%p, pmuxctx->psink:%s\n", ret, pmuxctx->pfmtctx, pmuxctx->psink);
            break;
        }
        // create video stream if necessary
        if(pmuxctx->bvideo_enable && pmuxctx->vcodec_id >= 0)
        {
            pmuxctx->nvidx = avmuxer_create_stream(pmuxctx, AVMEDIA_TYPE_VIDEO, pmuxctx->vcodec_id, pmuxctx->width, pmuxctx->height, pmuxctx->pix_fmt, pmuxctx->vbitrate, pmuxctx->pvextra_data, pmuxctx->nvextra_data_size);
            lbtrace("pmuxctx->nvidx:%d = avmuxer_create_stream\n", pmuxctx->nvidx);
            if(pmuxctx->nvidx < 0)
            {
                lberror("create video stream failed, pmuxctx->vcodec_id:%d, pmuxctx->width:%d, pmuxctx->height:%d, pmuxctx->pix_fmt:%d, pmuxctx->vbitrate:%d\n", pmuxctx->vcodec_id, pmuxctx->width, pmuxctx->height, pmuxctx->pix_fmt, pmuxctx->vbitrate);
                break;
            }
        }
        
        // create audio stream if necessary
        if(pmuxctx->baudio_enable && pmuxctx->acodec_id >= 0)
        {
            pmuxctx->naidx = avmuxer_create_stream(pmuxctx, AVMEDIA_TYPE_AUDIO, pmuxctx->acodec_id, pmuxctx->channel, pmuxctx->samplerate, pmuxctx->smp_fmt, pmuxctx->abitrate, pmuxctx->paextra_data, pmuxctx->naextra_data_size);
            lbtrace("pmuxctx->naidx:%d = avmuxer_create_stream\n", pmuxctx->naidx);
            if(pmuxctx->naidx < 0)
            {
                lberror("create audio stream failed, pmuxctx->acodec_id:%d, pmuxctx->channel:%d, pmuxctx->samplerate:%d, pmuxctx->smp_fmt:%d, pmuxctx->abitrate:%d\n", pmuxctx->acodec_id, pmuxctx->channel, pmuxctx->samplerate, pmuxctx->smp_fmt, pmuxctx->abitrate);
                break;
            }
        }
        
        // open avio
        ret = avio_open(&pmuxctx->pfmtctx->pb, pmuxctx->psink, AVIO_FLAG_WRITE);
        lbtrace("ret:%d = avio_open(&pmuxctx->pfmtctx->pb, pmuxctx->psink:%s, AVIO_FLAG_WRITE)", ret, pmuxctx->psink);
        if(ret < 0)
        {
            lberror("avio open failed, ret:%d, sink:%s\n", ret, pmuxctx->psink);
            break;
        }
        AVDictionary* dict = NULL;
        av_dict_set(&dict, "movflags", "faststart", 0);
        av_dict_set(&dict, "brand", "mov", 0);
        //av_dict_set(&dict, "-tag:v", "hvc1", 2);
         
        ret = avformat_write_header(pmuxctx->pfmtctx, &dict);
        lbtrace("ret:%d = avformat_write_header(pmuxctx->pfmtctx, &dict)\n", ret);
        if(ret < 0)
        {
            lberror("avforamt write header failed, ret:%d\n", ret);
        }
        pmuxctx->bheader_init = 1;
    }while(0);

    if(ret < 0)
    {
        lberror("avmuxer_open failed, ret:%d\n", ret);
        avmuxer_close(pmuxctx);
        
    }
    pthread_mutex_unlock(pmuxctx->pmutex);
    lbtrace("avmuxer_open end\n");
    return ret;
}

void avmuxer_close(struct avmuxer_context* pmuxctx)
{
    if(NULL == pmuxctx || NULL == pmuxctx->pfmtctx)
    {
        return;
    }
    pthread_mutex_lock(pmuxctx->pmutex);
    for(int i = 0; i < pmuxctx->pfmtctx->nb_streams; i++)
    {
        avcodec_close(pmuxctx->pfmtctx->streams[i]->codec);
    }
    avio_closep(&pmuxctx->pfmtctx->pb);
    avformat_free_context(pmuxctx->pfmtctx);
    if(pmuxctx->pxvc)
    {
        lbsc_close_xvc_context(&pmuxctx->pxvc);
    }
    pmuxctx->pfmtctx = NULL;
    pmuxctx->nvidx = -1;
    pmuxctx->naidx = -1;
    pmuxctx->nidr_come = 0;
    pmuxctx->ninterrupt = 0;
    pthread_mutex_unlock(pmuxctx->pmutex);
}

int64_t avmuxer_get_duration(struct avmuxer_context* pmuxctx)
{
    if(NULL == pmuxctx)
    {
        return 0;
    }
    
    return pmuxctx->llmuxer_duration;
}

void move_file(const char* psrc, const char* pdst, int copy)
{
    if(!psrc || !pdst)
    {
        lberror("move file failed, invalid file path, psrc:%s, pdst:%s\n", psrc, pdst);
        return ;
    }
    if(0 == strcmp(psrc, pdst))
    {
        lbtrace("psrc == pdst:%s, not move", pdst);
        return ;
    }
    // if source file not exist, return;
    if(0 != access(psrc, 0))
    {
        lberror("source file not exist, psrc:%s\n", psrc);
        return ;
    }
    
    // if dest file exist, remove it
    if(0 == access(pdst, 0))
    {
        remove(pdst);
    }
    
    FILE* psrcfile = fopen(psrc, "rb");
    FILE* pdstfile = fopen(pdst, "wb");
    int max_copy_len = 1024*256;
    char* pbuf = (char*)malloc(max_copy_len);
    int64_t sum_write_size = 0;
    int64_t sum_read_size = 0;
    while(psrcfile && pdstfile)
    {
        int readlen = fread(pbuf, 1, max_copy_len, psrcfile);
        if(readlen <= 0)
        {
            break;
        }
        
        int writelen = fwrite(pbuf, 1, readlen, pdstfile);
        sum_write_size += writelen;
        sum_read_size += readlen;
    }
    
    if(psrcfile)
    {
        fclose(psrcfile);
        psrcfile = NULL;
    }
    
    if(pdstfile)
    {
        fclose(pdstfile);
        pdstfile = NULL;
        lbtrace("move tmppath:%s, dstpath:%s\n", psrc, pdst);
        // remove source file if copy success
        if(!copy)
        {
            remove(psrc);
        }
    }
    
    lbtrace("total read size:%"PRId64", total write size:%"PRId64"", sum_read_size, sum_write_size);
}

int avmuxer_callback_notifiy(struct avmuxer_context* pmuxctx, int msgid, int wparam, int lparam)
{
    if(NULL == pmuxctx || NULL == pmuxctx->pcbfunc)
    {
        lberror("pmuxctx:%p, pmuxctx->pcbfunc:%p\n", pmuxctx, NULL == pmuxctx ? NULL : pmuxctx->pcbfunc);
        return -1;
    }
    
    pmuxctx->pcbfunc(pmuxctx->powner, msgid, wparam, lparam);
    
    return 0;
}

int avmuxer_open_write_raw_data_file(struct avmuxer_context* pmuxctx, int codec_id, const char* pfile_path)
{
    if(NULL == pmuxctx || NULL == pmuxctx->pcbfunc)
    {
        lberror("pmuxctx:%p, pmuxctx->pcbfunc:%p\n", pmuxctx, NULL == pmuxctx ? NULL : pmuxctx->pcbfunc);
        return -1;
    }
    int mediatype = get_media_type_from_codec_id(codec_id);
    
    if(0 == mediatype)
    {
        if(pmuxctx->pvideo_file)
        {
            fclose(pmuxctx->pvideo_file);
            pmuxctx->pvideo_file = NULL;
        }
        pmuxctx->pvideo_file = fopen(pfile_path, "wb");
        return pmuxctx->pvideo_file ? 0 : -1;
    }
    else if(1 == mediatype)
    {
        if(pmuxctx->paudio_file)
        {
            fclose(pmuxctx->paudio_file);
            pmuxctx->paudio_file = NULL;
        }
        pmuxctx->paudio_file = fopen(pfile_path, "wb");
        return pmuxctx->paudio_file ? 0 : -1;
    }
    
    return -1;
}

int avmuxer_write_raw_data_hook(struct avmuxer_context* pmuxctx, int codec_id, char* pdata, int data_len, int64_t pts)
{
    if(NULL == pmuxctx || NULL == pmuxctx->pcbfunc)
    {
        lberror("pmuxctx:%p, pmuxctx->pcbfunc:%p\n", pmuxctx, NULL == pmuxctx ? NULL : pmuxctx->pcbfunc);
        return -1;
    }
    int mediatype = get_media_type_from_codec_id(codec_id);
    if(0 == mediatype && pmuxctx->pvideo_file)
    {
        return fwrite(pdata, 1, data_len, pmuxctx->pvideo_file);
    }
    else if(0 == mediatype && pmuxctx->paudio_file)
    {
        return fwrite(pdata, 1, data_len, pmuxctx->paudio_file);
    }
    
    return -1;
}

int avmuxer_is_eof(struct avmuxer_context* pmuxctx)
{
    return pmuxctx ? pmuxctx->beof : 0;
}

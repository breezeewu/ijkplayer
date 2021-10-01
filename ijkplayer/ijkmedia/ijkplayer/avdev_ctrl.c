/*
 * avmuxer.c
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include "lazylog.h"
#include "lbsc_media_parser.h"
#include "libavformat/avformat.h"

typedef struct avdeliver_control_context
{
    int bwait_keyframe;
    long lvideo_frame_num;
    long laudio_frame_num;
    int  duration_index;
    int  duration_list[30];
    int64_t lvideo_toaol_duration;
    int64_t lvideo_pts;
    int64_t laudio_pts;
    
    int64_t lfix_last_vpts;
    int64_t lfix_last_apts;
    int64_t lfix_start_pts;
    int64_t lfix_min_duration;
    
    long lvideo_packet_size;
    long laudio_packet_size;
    float estimate_frame_rate;
    
} avdev_ctrl_ctx;

struct avdev_ctrl_ctx* dev_ctrl_open_context()
{
    struct avdeliver_control_context* pdcc = (struct avdeliver_control_context*)malloc(sizeof(struct avdeliver_control_context));
    memset(pdcc, 0, sizeof(struct avdeliver_control_context));
    pdcc->bwait_keyframe = 1;
    pdcc->lvideo_frame_num = -1;
    pdcc->laudio_frame_num = -1;
    pdcc->duration_index = 0;
    pdcc->lvideo_pts = INT64_MIN;
    pdcc->laudio_pts = INT64_MIN;
    pdcc->lfix_last_vpts = INT64_MIN;
    pdcc->lfix_last_apts = INT64_MIN;
    pdcc->lfix_start_pts = INT64_MIN;
    pdcc->lfix_min_duration = 1000000;
    return pdcc;
}

void dev_ctrl_close_contextp(struct avdeliver_control_context** ppdcc)
{
    if(ppdcc && *ppdcc)
    {
        struct avdeliver_control_context* pdcc = *ppdcc;
        free(pdcc);
        pdcc = *ppdcc = NULL;
    }
}

int is_packet_dropable(struct avdeliver_control_context* pdcc, int codec_id, char* pdata, int data_len, int64_t pts, long framenum)
{
    int ret = 0;
    if(NULL == pdcc || NULL == pdata || -1 == framenum)
    {
        return 0;
    }

    if(codec_id >= 10000)
    {
        // audio packet
        pdcc->laudio_pts = pts;
        pdcc->laudio_frame_num = framenum;
        pdcc->laudio_packet_size = data_len;
    }
    else
    {
        // video packet
        if(INT64_MIN != pdcc->lvideo_pts)
        {
            pdcc->duration_index = 0;
        }
        else
        {
            int64_t duration = pts - pdcc->lvideo_pts;
            if(duration <= 200)
            {
                pdcc->duration_list[pdcc->duration_index++] = duration;
                pdcc->duration_index = pdcc->duration_index%30;
            }
        }
        pdcc->lvideo_pts = pts;
        do{
            if(AV_CODEC_ID_H264 != codec_id && AV_CODEC_ID_HEVC != codec_id)
            {
                break;
            }
            if(pdcc->lvideo_frame_num < 0 || framenum > pdcc->lvideo_frame_num + 1)
            {
                sv_trace("pdcc->lvideo_frame_num:%ld < 0 || framenum:%ld >= pdcc->lvideo_frame_num:%ld + 1, waiting for key frame\n", pdcc->lvideo_frame_num, framenum, pdcc->lvideo_frame_num);
                pdcc->bwait_keyframe = 1;
            }
            
            if(pdcc->bwait_keyframe)
            {
                if(is_idr_frame((AV_CODEC_ID_H264 == codec_id ? 4:5), pdata, data_len))
                {
                    sv_trace("key frame come, stop drop frame\n");
                    pdcc->bwait_keyframe = 0;
                    break;
                }
                else
                {
                    ret = 1;
                    break;
                }
            }
        }while(0);
        
        if(ret)
        {
            sv_trace("waitting for keyframe, drop current frame, codec_id:%d, pdata:%p, data_len:%d, pts:%" PRId64 ", framenum:%d\n", codec_id, pdata, data_len, pts, framenum);
        }
        pdcc->lvideo_frame_num = framenum;
        pdcc->lvideo_pts = pts;
        pdcc->lvideo_packet_size = data_len;
    }
    
    return ret;
}

int64_t fix_timestamp(struct avdeliver_control_context* pdcc, int codec_id, int64_t pts)
{
    int ret = 0;
    if(NULL == pdcc)
    {
        return 0;
    }
    if(INT64_MIN != pdcc->lfix_start_pts && abs(pts - pdcc->lfix_start_pts - pdcc->lfix_last_vpts) > pdcc->lfix_min_duration && abs(pts - pdcc->lfix_start_pts - pdcc->lfix_last_apts) > pdcc->lfix_min_duration)
    {
        int64_t last_pts = pdcc->lfix_last_vpts > pdcc->lfix_last_apts ? pdcc->lfix_last_vpts : pdcc->lfix_last_apts;
        int64_t new_start_pts = pts - last_pts - 67;
        lbtrace("fix timestamp, org start time:%" PRId64 ", new_start_pts:%" PRId64 ", pts:%" PRId64 " - last_pts:%" PRId64 " - 67, lfix_last_vpts:%" PRId64 ", lfix_last_apts:%" PRId64 "",pdcc->lfix_start_pts, new_start_pts, pts, last_pts, pdcc->lfix_last_vpts, pdcc->lfix_last_apts);
        pdcc->lfix_start_pts = new_start_pts;
    }
    if(INT64_MIN == pdcc->lfix_start_pts && INT64_MIN != pts)
    {
        pdcc->lfix_start_pts = pts;
        pts = 0;
        pdcc->lfix_last_vpts = 0;
        pdcc->lfix_last_apts = 0;
    }
    else
    {
        pts -= pdcc->lfix_start_pts;
        if(codec_id >= 10000)
        {
            // audio pts
            pdcc->lfix_last_vpts = pts;
        }
        else
        {
            pdcc->lfix_last_apts = pts;
        }
    }
    
    return pts;
}

int get_estimate_frame_rate(struct avdeliver_control_context* pdcc)
{
    if(NULL == pdcc)
    {
        return -1;
    }
    int64_t totoal_duration = 0;
    int64_t frame_num = 0;
    for(int i = 0; i < sizeof(pdcc->duration_list)/sizeof(int); i++)
    {
        if(pdcc->duration_list[i] != 0)
        {
            totoal_duration += pdcc->duration_list[i];
            frame_num++;
        }
    }

    if(frame_num <= 0)
    {
        // return default framerate
        return 15;
    }
    int64_t frame_rate = frame_num*1000000/totoal_duration;
    lbtrace("estimate frame_rate:%" PRId64 " = frame_num:%" PRId64 "*1000000/totoal_duration:%" PRId64 "\n", frame_rate, frame_num, totoal_duration);
    return (int)frame_rate;
}

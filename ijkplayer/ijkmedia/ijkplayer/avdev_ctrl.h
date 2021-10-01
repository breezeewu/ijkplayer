/*
 * avmuxer.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#pragma once
#include <stdint.h>
#include <libavcodec/avcodec.h>
#define ENABLE_WAITING_FOR_KEYFRAME
struct avdeliver_control_context* dev_ctrl_open_context();

void dev_ctrl_close_contextp(struct avdeliver_control_context** ppdcc);

int is_packet_dropable(struct avdeliver_control_context* pdcc, int codec_id, char* pdata, int data_len, int64_t pts, int framenum);

int64_t fix_timestamp(struct avdeliver_control_context* pdcc, int codec_id, int64_t pts);

int get_estimate_frame_rate(struct avdeliver_control_context* pdcc);

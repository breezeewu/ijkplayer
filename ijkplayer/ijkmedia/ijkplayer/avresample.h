/*
 * avmuxer.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#pragma once
#include <stdint.h>
#include <libavcodec/avcodec.h>

struct avresample_context* avresample_alloc_context();

void avresample_free_contextp(struct avresample_context** ppresample);

//struct avresample_context* avresample_init(struct avresample_context* presample, int src_channel, int src_samplerate, int src_format, int dst_channel, int dst_samplerate, int dst_format);
struct avresample_context* avresample_init(struct avresample_context* presample, int dst_channel, int dst_samplerate, int dst_format);

int avresample_resample(struct avresample_context* presample, AVFrame* pdstframe, AVFrame* psrcframe);

int avresample_resample_frame(struct avresample_context* presample, AVFrame* pframe);

int confirm_swr_avaiable(struct avresample_context* presample, AVFrame* psrcframe);
//int resample(AVFrame* pdstframe, AVFrame* psrcframe);

struct sample_buffer* lbsample_buffer_open(int channel, int samplerate, int format, int max_size);

void lbsample_buffer_close(struct sample_buffer ** ppsb);

int lbsample_buffer_deliver(struct sample_buffer * psb, const char* pbuf, int len, int64_t pts);

int lbsample_buffer_fetch(struct sample_buffer* psb, char* pbuf, int len, int64_t* ppts);

int lbsample_buffer_remain(struct sample_buffer* psb);

void lbsample_buffer_align(struct sample_buffer* psb);

int lbsample_buffer_size(struct sample_buffer* psb);

void lbsample_buffer_reset(struct sample_buffer* psb);

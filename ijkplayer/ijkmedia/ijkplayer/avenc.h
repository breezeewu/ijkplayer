/*
 * avmuxer.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#pragma once
#include <stdint.h>
#include <libavcodec/avcodec.h>
#define ENABLE_WRITE_INPUT_AND_OUTPUT
struct avencoder_context* avencoder_alloc_context();

void avencoder_free_contextp(struct avencoder_context** ppenc_ctx);

int avencoder_init(struct avencoder_context* penc_ctx, int codec_type, int codec_id, int param1, int param2, int param3, int format, int bitrate);

int avencoder_context_open(struct avencoder_context** ppenc_ctx, int codec_type, int codec_id, int param1, int param2, int praram3, int format, int bitrate);

int avencoder_encode_frame(struct avencoder_context* penc_ctx, AVFrame* pframe, char* pout_buf, int out_len);

int avencoder_encoder_data(struct avencoder_context* penc_ctx, char* pdata, int datalen, char* pout_buf, int out_len, long pts);

int avencoder_get_packet(struct avencoder_context* penc_ctx, char* pdata, int* datalen, long* pts);

int encoder_frame(int codecid, AVFrame* pframe, char* pout_buf, int out_len);

AVFrame* decoder_packet(int mediatype, int codecid, char* ph264, int h264len);

int convert_h26x_to_jpg(int codecid, char* penc_data, int data_len, char* pjpg, int jpglen);

int h26x_keyframe_to_jpg_file(int codecid, char* penc_data, int data_len, char* pjpgfile);
#ifdef ENABLE_WRITE_INPUT_AND_OUTPUT
int set_enable_write_test_data(struct avencoder_context* penc_ctx, int bwrite_input, int bwrite_output);
#endif

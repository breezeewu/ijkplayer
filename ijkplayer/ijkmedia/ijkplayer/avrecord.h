/*
 * avrecorder.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#ifndef AV_RECORD_H_
#define AV_RECORD_H_
#include <stdint.h>
#include "avmuxer.h"

//#define ENABLE_RECORD_MUXER_MEDIA

struct avrecord_context* avrecord_open_context(const char* psource, const char* psink_url, const char* pformat, const char* ptmp_url);
struct avrecord_context* avrecord_open_contextex(struct avrecord_context** pprc, const char* psrc_url, const char* psink_url, const char* pformat, const char* ptmp_url);
//long avrecord_open_contextex(struct avrecord_context** pprc, const char* psource, const char* psink_url, const char* pformat, const char* ptmp_url);

int avrecord_start(struct avrecord_context* prc, int64_t pts);

void avrecord_stop(struct avrecord_context* prc, int bcancel);

int avrecord_is_running(struct avrecord_context* prc);

void avrecord_close_contextp(struct avrecord_context** pprc);

int avrecord_get_percent(struct avrecord_context* prc);

int avrecord_deliver_packet(struct avrecord_context* prc, int codec_id, char* pdata, int len, int64_t pts, long framenum);

int avrecord_eof(struct avrecord_context* prc);

void* avrecord_set_callback(struct avrecord_context* prc, avmuxer_callback cbfunc, void* powner);
#endif

/*
 * avdemuxer.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#ifndef AV_DEMUXER_H_
#define AV_DEMUXER_H_
#include <stdint.h>

typedef int (*deliver_callback)(struct avmuxer_context* pmc, int codec_id, char* pdata, int len, int64_t pts);
struct avdemux_context* avdemux_open_context(const char* psource);
int avdemux_seek(struct avdemux_context* pdc, int64_t seek_pts);

int avdemux_set_deliver_callback(struct avdemux_context* pdc, deliver_callback pdev_cb, struct avmuxer_context* pdev_mc);

int avdemux_start(struct avdemux_context* pdc);

void avdemux_stop(struct avdemux_context* pdc);

int avdemux_is_running(struct avdemux_context* pdc);

int64_t avdemux_get_duration(struct avdemux_context* pdc);

void avdemux_close_contextp(struct avdemux_context** ppdc);
#endif

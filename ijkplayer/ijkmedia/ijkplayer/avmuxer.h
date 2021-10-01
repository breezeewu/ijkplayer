/*
 * avmuxer.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#ifndef AV_MUXER_H_
#define AV_MUXER_H_
#include <stdint.h>

#define AVMUXER_MSG_START                   2001
#define AVMUXER_MSG_PROGRESS                2002    // arg1 = mediatype, arg2 = pts
#define AVMUXER_MSG_COMPLETE                2003    // arg1 = interrupt falg, arg2 = duration
#define AVMUXER_MSG_ERROR                   2004    // arg1 = 0, arg2 = error code
#define AVMUXER_MSG_INTERRUPT               2005
#define MAX_PACKET_BUFFER_COUNT             45

#define INTERRUPT_REASON_RES_CHANGE                 0x1
#define INTERRUPT_REASON_VIDEO_PKT_DATA_ERROR       0x2
#define INTERRUPT_REASON_AUDIO_PKT_DATA_ERROR       0x4
#define INTERRUPT_REASON_SPS_PPS_CHANGE             0x8
//#define ENABLE_FFMPEG_BSF_FILTER

typedef void (*avmuxer_callback)(void* pvoid, int msgid, int wparam, int lparam);

struct avmuxer_context* avmuxer_open_context();

int avmuxer_set_sink(struct avmuxer_context* pmuxctx, const char* pdst_path, const char* ptmp_url, char* pformat);

void* avmuxer_set_callback(struct avmuxer_context* pmuxctx, avmuxer_callback cbfunc, void* powner);

int avmuxer_add_stream(struct avmuxer_context* pmuxctx, int mediatype, int codec_id, int param1, int param2, int format, int bitrate, char* pextra_data, int extra_data_size);

int avmuxer_deliver_packet(struct avmuxer_context* pmuxctx, int codec_id, char* pdata, int data_len, int64_t pts);

int avmuxer_start(struct avmuxer_context* pmuxctx);

int avmuxer_start_context(struct avmuxer_context** ppmuxctx, const char* pdst_path, const char* ptmp_url, char* pformat);

void avmuxer_stop(struct avmuxer_context* pmuxctx, int bcancel);

int avmuxer_is_running(struct avmuxer_context* pmuxctx);

void avmuxer_close_contextp(struct avmuxer_context** ppmuxctx);

int avmuxer_create_stream(struct avmuxer_context* pmuxctx, int mediatype, int codec_id, int param1, int param2, int format, int bitrate, char* pextra_data, int extra_data_size);

int avmuxer_open(struct avmuxer_context* pmuxctx);

void avmuxer_close(struct avmuxer_context* pmuxctx);

int64_t avmuxer_get_duration(struct avmuxer_context* pmuxctx);

void move_file(const char* psrc, const char* pdst, int copy);

int avmuxer_callback_notifiy(struct avmuxer_context* pmuxctx, int msgid, int wparam, int lparam);

int avmuxer_open_write_raw_data_file(struct avmuxer_context* pmuxctx, int codec_id, const char* pfile_path);

int avmuxer_write_raw_data_hook(struct avmuxer_context* pmuxctx, int codec_id, char* pdata, int data_len, int64_t pts);

int avmuxer_is_eof(struct avmuxer_context* pmuxctx);
#endif

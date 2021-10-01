/*
 * avdemuxer.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#ifndef LBWAV_MUXER_H_
#define LBWAV_MUXER_H_
#include <stdint.h>

struct lbwav_mux_context* lbwav_open_context(const char* pwav_url, int channel, int samplerate, int bitspersample);

int lbwav_write_data(struct lbwav_mux_context* pwmc, const char* pdata, int len);

void lbwav_close_contextp(struct lbwav_mux_context** ppwmc);
#endif

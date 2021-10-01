/*
 * IJKMediaMuxer.m
 *
 * Copyright (c) 2013 Bilibili
 * Copyright (c) 2013 Zhang Rui <bbcallen@gmail.com>
 *
 * This file is part of ijkPlayer.
 *
 * ijkPlayer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * ijkPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ijkPlayer; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#import "IJKMediaRecorder.h"
#include "avrecord.h"
#include "lbwavmux.h"
#include "lbsc_util_conf.h"
#include "lazylog.h"
@implementation IJKMediaRecorder : NSObject  {
//IMediaRecordCallback* precord_callback;
id<IMediaRecordCallback> rec_cb;
struct avdemux_context* prc;
NSString* psrcurl;
NSString* psinkurl;
NSString* ptmpurl;
int64_t m_lpts;
}
int event_callback(void* pvoid, int msgid, int wparam, int lparam)
{
    id<IMediaRecordCallback> prcb = (__bridge id<IMediaRecordCallback>)pvoid;//(__bridge_transfer id<IMediaRecordCallback>)pvoid;
    return [prcb eventNotify:msgid wparam:(int)wparam lparam:(int)lparam];
    /*IJKMediaRecorder* prec =(__bridge_transfer IJKMediaRecorder *)(pvoid);
    //IJKMediaRecorder* prec = (IJKMediaRecorder*)pvoid;
    if(NULL != prec->rec_cb)
    {
        return [prec->rec_cb eventNotify:msgid wparam:(int)wparam lparam:(int)lparam];
    }
    
    return -1;*/
}

- (id)initRecord:(NSString*)psrc_url sinkurl:(NSString*)sink_url tmpurl:(NSString*)tmp_url
{
    self = [super init];
    const char* psrc = [psrc_url UTF8String];
    const char* pdst = [sink_url UTF8String];
    const char* ptmp = [tmp_url UTF8String];
    prc = avrecord_open_context(psrc, pdst, "mov", ptmp);
    psrcurl = psrc_url;
    psinkurl = sink_url;
    ptmpurl = tmp_url;
    
    //rec_cb = NULL;
    return self;
}

-(void)setEventNotify:(id<IMediaRecordCallback>)reccb;
{
    //rec_cb = reccb;
    avrecord_set_callback(prc, event_callback, (__bridge_retained void *)reccb);
}

-(void)seek:(int64_t)pts
{
    m_lpts = pts;
}

-(void)start
{
    avrecord_start(prc, m_lpts);
}

-(void)stop
{
    avrecord_stop(prc, 0);
}

/*-(int)sendData:(int)codec_id data:(char*)pdata data_size:(int)size pts:(int64_t)pts
{
    return avrecord_deliver_packet(prc, codec_id, pdata, size, pts*1000);
}*/
/*-(int)deliver_data:(int)codec_id data:(char*)pdata data_size:(int)size pts:(int64_t)pts frame_num:(long)frame_num
{
    return avrecord_deliver_packet(prc, codec_id, pdata, size, pts, frame_num);
}*/

-(int)deliverData:(int)codecid data:(char*)pdata size:(int)size pts:(int64_t)pts framenum:(long)framenum
{
    return avrecord_deliver_packet(prc, codecid, pdata, size, pts*1000, framenum);
}

-(int)getPercent
{
    return avrecord_get_percent(prc);
}
@end

@implementation IWavMuxer
{
    NSString* psink_url;
    int channel;
    int samplerate;
    int bitspersample;
    struct lbwav_mux_context* pwmc;
}

- (id) initWithSinkUrl:(NSString *)sinkUrl channcel:(int)channel samplerate:(int)samplerate bitspersample:(int)bitspersample
{
    
    self = [super init];
    self->psink_url = sinkUrl;
    self->channel = channel;
    self->samplerate = samplerate;
    self->bitspersample =  bitspersample;

    self->pwmc = lbwav_open_context([self->psink_url UTF8String], self->channel, self->samplerate, self->bitspersample);
    
    return self;
}

- (id) initWithConfig:(NSString *)pconf_tag channcel:(int)channel samplerate:(int)samplerate bitspersample:(int)bitspersample
{
    const char* ptag = get_conf_value_by_tag(g_pconf_ctx, [pconf_tag UTF8String]);
    const char* logpath = gen_url_by_log_path(ptag);
    if(NULL == logpath)
    {
        return nil;
    }
    
    self = [super init];

    self->psink_url = [[NSString alloc] initWithCString:(const char*)logpath encoding:NSASCIIStringEncoding];
    self->channel = channel;
    self->samplerate = samplerate;
    self->bitspersample =  bitspersample;
    
    self->pwmc = lbwav_open_context(logpath, self->channel, self->samplerate, self->bitspersample);
    
    return self;
}

- (void) deliverData:(char*)pdata dataLen:(int)dataLen
{
    lbwav_write_data(pwmc, pdata, dataLen);
}

- (void) close
{
    if(pwmc)
    {
        lbwav_close_contextp(&pwmc);
    }
}

- (void)dealloc
{
    [self close];
}
@end

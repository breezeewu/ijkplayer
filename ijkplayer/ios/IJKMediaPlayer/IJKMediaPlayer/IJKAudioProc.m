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

#import "IJKAudioProc.h"
#include "audio_proc.h"
#include "lazylog.h"

@implementation IJKAudioProc {
    struct audio_proc_ctx* papc;
};

-(id)init:(int)channel samplerate:(int)samplerate sampleformat:(int)sampleformat nbsamples:(int)nbsamples
{
    self = [super init];
    papc = lbaudio_proc_open_context(channel, samplerate, sampleformat, nbsamples);
    
    return self;
}

-(void)initlog:(char*)plogpath loglevel:(int)level logmode:(int)mode
{
    sv_init_log(plogpath, level, mode, 0, 0, 0, 1);
}

-(int)add_aec_filter:(int)usdelay
{
    return laddd_aec_filterex(papc, usdelay/1000);
}

-(int)add_ns_filter:(int)ns_mode
{
    return lbadd_noise_reduce_filter(papc, 1, gen_url_by_log_path("nr.pcm"));
}

-(int)add_agc_filter:(int)agc_mode
{
    return lbadd_agc_filter(papc, 3, gen_url_by_log_path("agc.pcm"));
}
-(int)audio_process:(char*)pfar near:(char*)pnear outbuf:(char*)pout size:(int)size
//-(int)audio_process:(char*)pfar near:(char*)pnear outbuf:(char*)pout size:(int)size
{
    return lbaudio_proc_process(papc, pfar, pnear, pout, size);
}

-(void)deliveFarData:(char*)pdata datalen:(int)len timestamp:(long long)timestamp
{
    lbfar_audio_callback(papc, pdata, len, timestamp, 1, 8000, 1);
}
-(int)deliverData:(char*)pdata  outbuffer:(char*)poutbuf data_size:(int)size timestamp:(long long)timestamp_in_us
{
    return lbaudio_proc_process(papc, pdata, poutbuf, size, timestamp_in_us);
}

-(int)audio_Aec_Proc:(char*)pfar nearbuffer:(char*)pnear output:(char*)pout length:(int)len
{
    return lbaec_proc(papc, pfar, pnear, pout, len);
}

-(void)close
{
    lbaudio_proc_close_contextp(&papc);
}

+ (long long) get_timestamp_in_us
{
    return lbget_system_timestamp_in_us();
}
@end

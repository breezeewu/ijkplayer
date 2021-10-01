/*
 * IJKMediaMuxer.h
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

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface IJKAudioProc:NSObject
-(id)init:(int)channel samplerate:(int)samplerate sampleformat:(int)sampleformat nbsamples:(int)nbsamples;
-(void)initlog:(char*)plogpath loglevel:(int)level logmode:(int)mode;
-(int)add_aec_filter:(int)msdelay;
-(int)add_ns_filter:(int)ns_mode;
-(int)add_agc_filter:(int)agc_mode;
-(int)audio_process:(char*)pfar near:(char*)pnear outbuf:(char*)pout size:(int)size;
//-(int)audio_process:(char*)pdata outbuf:(char*)pout size:(int)size;

//-(int)add_aec_filter:(int)far_channel far_samplerate:(int)far_samplerate far_sampleformat:(int)far_sampleformat msdelay:(int)msdelay;
-(void)deliveFarData:(char*)pdata datalen:(int)len timestamp:(long long)timestamp_in_us;
-(int)deliverData:(char*)pdata  outbuffer:(char*)poutbuf data_size:(int)size timestamp:(long long)timestamp_in_us;
-(int)audio_Aec_Proc:(char*)pfar nearbuffer:(char*)pnear output:(char*)pout length:(int)len;
-(void)close;
+(long long)get_timestamp_in_us;
@end

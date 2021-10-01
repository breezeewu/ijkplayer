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
#import "MediaConst.h"
@protocol IMediaRecordCallback;
#pragma mark IMediaRecordCallback
@protocol IMediaRecordCallback <NSObject>
-(int)eventNotify:(int)msg wparam:(int)wparam lparam:(int)lparam;
@end
static const int VAVA_CODEC_ID_H264     =   28;
static const int VAVA_CODEC_ID_H265     =   174;
static const int VAVA_CODEC_ID_AAC      = 86018;

@interface IJKMediaRecorder:NSObject
/*
 函数描述：初始化record
 参数：
 id<IMediaRecordCallback>)reccb:
 */
-(id)initRecord:(NSString*)psrc_url sinkurl:(NSString*)sink_url tmpurl:(NSString*)tmpurl;
-(void)setEventNotify:(id<IMediaRecordCallback>)reccb;
-(void)seek:(int64_t)pts;
-(void)start;
-(void)stop;
//-(int)sendData:(int)codec_id data:(char*)pdata data_size:(int)size pts:(int64_t)pts;
//-(int)deliverData:(int)codec_id data:(char*)pdata data_size:(int)size pts:(int64_t)pts frame_num:(long)frame_num;
-(int)deliverData:(int)codecid data:(char*)pdata size:(int)size pts:(int64_t)pts framenum:(long)framenum;
-(int)getPercent;


@end


@interface IWavMuxer:NSObject

- (id) initWithSinkUrl:(NSString *)sinkUrl channcel:(int)channel samplerate:(int)samplerate bitspersample:(int)bitspersample;

- (id) initWithConfig:(NSString *)pconf_tag channcel:(int)channel samplerate:(int)samplerate bitspersample:(int)bitspersample;

- (void) deliverData:(char*)pdata dataLen:(int)dataLen;

- (void) close;

@end

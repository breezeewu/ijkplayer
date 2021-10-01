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

#import "IJKMediaEncoder.h"
#include "avenc.h"
@implementation IJKMediaEncoder {
    struct avencoder_context* encctx;
    
struct avmuxer_context* pmuxctx;
}

- (id)init:(int)mt codecID:(int)codecid Param1:(int)param1 Param2:(int)param2 Param3:(int)param3  format:(int)format bitrate:(int)bitrate
{
    self = [super init];
    int ret = avencoder_context_open(&encctx, mt, codecid, param1, param2, param3, format, bitrate);
    
    return self;
}

- (int)deliverData:(char*)rawdata rawdatalen:(int)rawdatalen encdata:(char*)encdata encdatalen:(int)encdatalen present_time:(long long)pts_in_ms
{
    int ret = avencoder_encoder_data(encctx, rawdata, rawdatalen, encdata, encdatalen, pts_in_ms*1000);
    return ret;
}

- (int)getEncData:(char*)pdata datalen:(int*)datalen timeStamp:(long long*) ppts_in_ms
{
    int64_t pts = 0;
    int ret = avencoder_get_packet(encctx, pdata, datalen, &pts);
    if(ppts_in_ms)
    {
        *ppts_in_ms = pts /1000;
    }
    return ret;
}
+ (int)Video_KeyFrame_To_JPG:(int)codecid penc_data:(char*)pdata datalen:(int)datalen jpgdata:(char*)pjpg jpglen:(int)jpglen;
//+ (int)Video_KeyFrame_To_JPG:(int)codecid penc_data:(char*)ph264 h264len:(int)h264len jpgdata:(char*)pjpg jpglen:(int)jpglen
{
    int ret = convert_h26x_to_jpg(codecid, pdata, datalen, pjpg, jpglen);
    return ret;
}
+ (int)Video_KeyFrame_To_JPG_File:(int)codecid penc_data:(char*)pdata datalen:(int)datalen jpgfile:(NSString*)jpgfile;
//+ (int)Video_KeyFrame_To_JPG_File:(int)codecid penc_data:(char*)ph264 h264len:(int)h264len jpgfile:(NSString*)jpgfile
{
    int ret = h26x_keyframe_to_jpg_file(codecid, pdata, datalen, [jpgfile UTF8String]);
    return ret;
}

- (void)deinit
{
    avencoder_free_contextp(&encctx);
}
@end

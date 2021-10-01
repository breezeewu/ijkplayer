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

@interface IJKMediaEncoder: NSObject
- (id)init:(int)mt codecID:(int)codecid Param1:(int)param1 Param2:(int)param2 Param3:(int)param3 format:(int)format bitrate:(int)bitrate;

- (int)deliverData:(char*)rawdata rawdatalen:(int)rawdatalen encdata:(char*)encdata encdatalen:(int)encdatalen present_time:(long long)pts_in_ms;

- (int)getEncData:(char*)pdata datalen:(int*)datalen timeStamp:(long long*) ppts_in_ms;

// success return jpg data len, failed return minus
+ (int)H264KeyFrame_To_JPG:(char*)ph264 h264len:(int)h264len jpgdata:(char*)pjpg jpglen:(int)jpglen;

// success return 0, failed return minus
+ (int)H264KeyFrame_To_JPG_File:(char*)ph264 h264len:(int)h264len jpgfile:(NSString*)jpgfile;
- (void)deinit;
@end

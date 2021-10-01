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

#import "IJKMediaMuxer.h"
#include "avmuxer.h"


@implementation IJKMediaMuxer {
/*    IjkMediaPlayer *_mediaPlayer;
    IJKSDLGLView *_glView;
    IJKFFMoviePlayerMessagePool *_msgPool;
    NSString *_urlString;

    NSInteger _videoWidth;
    NSInteger _videoHeight;
    NSInteger _sampleAspectRatioNumerator;
    NSInteger _sampleAspectRatioDenominator;

    BOOL      _seeking;
    NSInteger _bufferingTime;
    NSInteger _bufferingPosition;

    BOOL _keepScreenOnWhilePlaying;
    BOOL _pauseInBackground;
    BOOL _isVideoToolboxOpen;
    BOOL _playingBeforeInterruption;

    IJKNotificationManager *_notificationManager;

    AVAppAsyncStatistic _asyncStat;
    IjkIOAppCacheStatistic _cacheStat;
    NSTimer *_hudTimer;
    IJKSDLHudViewController *_hudViewController;
}*/
struct avmuxer_context* pmuxctx;
}

@synthesize duration;

- (id)initWithSinkUrl:(NSString *)sinkUrl tmpUrl:(NSString*)tmpUrl
{
    self = [super init];
    pmuxctx = avmuxer_open_context();
    avmuxer_set_sink(pmuxctx, [sinkUrl UTF8String], [tmpUrl UTF8String], "mov");
    
    return self;
}

- (void)start
{
    avmuxer_start(pmuxctx);
}
- (void)deliverData:(int) codec_id mediadata:(char*) pdata data_size:(int) size present_time:(long long) pts_in_ms
{
    avmuxer_deliver_packet(pmuxctx, codec_id, pdata, size, pts_in_ms*1000);
}

- (void)stop:(BOOL)bcancel
{
    avmuxer_stop(pmuxctx, bcancel);
}

- (NSTimeInterval)duration
{
    if (!pmuxctx)
        return 0.0f;

    NSTimeInterval ret = (double)avmuxer_get_duration(pmuxctx)/1000000.0;
    if (isnan(ret) || isinf(ret))
        return -1;

    return ret / 1000;
}
@end

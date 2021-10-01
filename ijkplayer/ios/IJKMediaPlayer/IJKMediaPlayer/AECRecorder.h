//
//  XBEchoCancellation.h
//  iOSEchoCancellation
//
//  Created by xxb on 2017/8/25.
//  Copyright © 2017年 xxb. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
typedef void (^on_aec_record_block)(AudioBufferList *bufferList);
typedef void (^on_aec_render_block)(AudioBufferList *bufferList,UInt32 inNumberFrames);
@interface AECRecorder : NSObject

@property (nonatomic,copy) on_aec_record_block on_aec_record_callback;
@property (nonatomic,copy) on_aec_render_block on_aec_render_callback;

@property (nonatomic,assign,readonly) AudioStreamBasicDescription streamFormat;

- (instancetype)init;

+ (instancetype)getInstance;

- (instancetype)openAECRecord:(int)channel samplerate:(int)samplerate bitspersample:(int)bitspersample record_cb:(on_aec_record_block)record_cb;

- (void)closeAECRecord;

@end

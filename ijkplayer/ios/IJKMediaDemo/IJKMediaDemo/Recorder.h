//
//  Recorder.h
//  PPCS_Client
//
//  Created by Elson on 2018/2/9.
//  Copyright © 2018年 internet. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <IJKMediaFramework/IJKMediaFramework.h>

#define RECORDER_NOTIFICATION_CALLBACK_NAME @"recorderNotificationCallBackName"
#define kNumberAudioQueueBuffers 3 //缓冲区设定3个
#define kDefaultSampleRate 16000    //采样率

typedef void(^inputAudioDataCallBack)(uint8_t *data, size_t data_size, int inNumberFrames);

@interface Recorder : NSObject
{
    AudioQueueRef _audioQueue;
    AudioStreamBasicDescription audioFormat;
    AudioQueueBufferRef _audioBuffers[kNumberAudioQueueBuffers];
    char data[1024];
    char aac_data[1024];
    int offset;
    bool bechocancel;
    
}
@property (nonatomic, strong) IJKMediaEncoder* aac_enc;//指针不能用assign
@property (nonatomic, assign) id<IJKMediaPlayback> player;
@property (nonatomic, assign) BOOL isRecording;
@property (atomic, assign) NSUInteger sampleRate;
@property (nonatomic, copy) inputAudioDataCallBack callBack;
@property (nonatomic, strong) NSMutableData *audioData;//采集640容易崩溃，320转码后会变成160.所以用这个data来拼接
- (id)initRecord:(id<IJKMediaPlayback>)player;
-(void)start;
-(void)stop;


@end

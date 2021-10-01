//
//  XBEchoCancellation.m
//  iOSEchoCancellation
//
//  Created by dawson on 2021/01/18.
//  Copyright © 2021年 sunvalley.com.cn. All rights reserved.
//

#import "AECRecorder.h"
#include "IJKMediaRecorder.h"
typedef enum : NSUInteger {
    AECStatus_open,
    AECStatus_close
} AECStatus;

typedef struct AECAUGraph{
    AUGraph graph;
    AudioUnit remoteIOUnit;
} AECAUGraph;


@interface AECRecorder()
{
    AECAUGraph aecGraph;
    //AudioUnit remoteIOUnit;

    int channel;
    int samplerate;
    int bitsPerSample;

    AECStatus aecStatus;
    NSString* recordCategory;
    IWavMuxer*  pwav_mux;
}

@property (nonatomic,assign) BOOL isRunningService; //是否运行着声音服务
@property (nonatomic,assign) BOOL isNeedInputCallback; //需要录音回调(获取input即麦克风采集到的声音回调)
@property (nonatomic,assign) BOOL isNeedOutputCallback; //需要播放回调(output即向发声设备传递声音回调)
@end

@implementation AECRecorder
@synthesize streamFormat;

+ (instancetype)getInstance
{
    static AECRecorder *echo = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        echo = [[AECRecorder alloc] init];
    });
    
    return echo;
}
- (long)getSysTime
{
    NSDate *datenow = [NSDate date];
    return (long)([datenow timeIntervalSince1970] *1000);
}
///0 开启，1 关闭
-(void)openOrCloseEchoCancellation:(UInt32)newAECStatus
{
    if (self.isRunningService == NO)
    {
        return;
    }
    UInt32 echoCancellation;
    UInt32 size = sizeof(echoCancellation);
    CheckError(AudioUnitGetProperty(aecGraph.remoteIOUnit,
                                    kAUVoiceIOProperty_BypassVoiceProcessing,
                                    kAudioUnitScope_Global,
                                    0,
                                    &echoCancellation,
                                    &size),
               "kAUVoiceIOProperty_BypassVoiceProcessing failed");
    if (newAECStatus == echoCancellation)
    {
        return;
    }
    
    CheckError(AudioUnitSetProperty(aecGraph.remoteIOUnit,
                                    kAUVoiceIOProperty_BypassVoiceProcessing,
                                    kAudioUnitScope_Global,
                                    0,
                                    &newAECStatus,
                                    sizeof(newAECStatus)),
               "AudioUnitSetProperty kAUVoiceIOProperty_BypassVoiceProcessing failed");
    aecStatus = newAECStatus == 0 ? AECStatus_open : AECStatus_close;
}


#pragma mark - 初始化AUGraph和Audio Unit

-(void)startGraph:(AUGraph)graph
{
    CheckError(AUGraphInitialize(graph),
               "AUGraphInitialize failed");
    CheckError(AUGraphStart(graph),
               "AUGraphStart failed");
    aecStatus = AECStatus_open;
}

- (void)stopGraph:(AUGraph)graph
{
    if (self.isRunningService == NO)
    {
        return;
    }
    CheckError(AUGraphStop(graph),"AUGraphStop failed");
    CheckError(AUGraphUninitialize(graph),"AUGraphUninitialize failed");
    CheckError(DisposeAUGraph(graph), "AUGraphDispose failed");
    self.isRunningService = NO;
    aecStatus = AECStatus_close;
}


-(void)createAUGraph:(AECAUGraph*)augStruct{
    //Create graph
    CheckError(NewAUGraph(&augStruct->graph),
               "NewAUGraph failed");
    
    //Create nodes and add to the graph
    AudioComponentDescription inputcd = {0};
    // 组件类型
    inputcd.componentType = kAudioUnitType_Output;
    // 打开回音消除功能
    inputcd.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
    // 选择组件厂商
    inputcd.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    AUNode remoteIONode;
    //Add node to the graph/增加graph节点
    CheckError(AUGraphAddNode(augStruct->graph,
                              &inputcd,
                              &remoteIONode),
               "AUGraphAddNode failed");
    
    //Open the graph/打开augraph
    CheckError(AUGraphOpen(augStruct->graph),
               "AUGraphOpen failed");
    
    //Get reference to the node/获取graph节点信息
    CheckError(AUGraphNodeInfo(augStruct->graph,
                               remoteIONode,
                               &inputcd,
                               &augStruct->remoteIOUnit),
               "AUGraphNodeInfo failed");
}

#pragma mark - 回调函数
OSStatus InputCallback_xb(void *inRefCon,
                       AudioUnitRenderActionFlags *ioActionFlags,
                       const AudioTimeStamp *inTimeStamp,
                       UInt32 inBusNumber,
                       UInt32 inNumberFrames,
                       AudioBufferList *ioData){
    
    AECRecorder *aecrecord = (__bridge AECRecorder*)inRefCon;
    if (aecrecord.isNeedInputCallback == NO)
    {
//        NSLog(@"没有开启声音输入回调");
        return noErr;
    }
    AECAUGraph *aecGraph = &(aecrecord->aecGraph);
    
    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0].mData = NULL;
    bufferList.mBuffers[0].mDataByteSize = 0;

    AudioUnitRender(aecGraph->remoteIOUnit,
                                      ioActionFlags,
                                      inTimeStamp,
                                      1,
                                      inNumberFrames,
                                      &bufferList);
//    AudioBuffer buffer = bufferList.mBuffers[0];
    /*if(echoCancellation->pfar_pcm_file)
    {
        for(int i = 0; i < inBusNumber; i++)
        {
            fwrite(bufferList.mBuffers[0].mData, 1, bufferList.mBuffers[0].mDataByteSize, echoCancellation->pfar_pcm_file);
        }
    }*/
    
    if(aecrecord->pwav_mux)
    {
        for(int i = 0; i < bufferList.mNumberBuffers; i++)
        {
            [aecrecord->pwav_mux deliverData:bufferList.mBuffers[i].mData dataLen:bufferList.mBuffers[i].mDataByteSize];
            //fwrite(bufferList.mBuffers[i].mData, 1, bufferList.mBuffers[i].mDataByteSize, echoCancellation->pnear_pcm_file);
        }
    }

    if (aecrecord->_on_aec_record_callback)
    {
        aecrecord->_on_aec_record_callback(&bufferList);
    }
    return noErr;
}

OSStatus outputRenderTone_xb(
                          void *inRefCon,
                          AudioUnitRenderActionFlags     *ioActionFlags,
                          const AudioTimeStamp         *inTimeStamp,
                          UInt32                         inBusNumber,
                          UInt32                         inNumberFrames,
                          AudioBufferList             *ioData)

{
    //TODO: implement this function
    memset(ioData->mBuffers[0].mData, 0, ioData->mBuffers[0].mDataByteSize);
    
    AECRecorder *aecrecord = (__bridge AECRecorder*)inRefCon;
    if (aecrecord->_isNeedOutputCallback == NO)
    {
        //        NSLog(@"没有开启声音输出回调");
        return noErr;
    }
    


    if (aecrecord->_on_aec_render_callback)
    {
        aecrecord->_on_aec_render_callback(ioData,inNumberFrames);
        /*if(aecrecord->pfar_pcm_file && ioData->mNumberBuffers > 0)
        {
            for(int i = 0; i < ioData->mNumberBuffers; i++)
            {
                fwrite(ioData->mBuffers[i].mData, 1, ioData->mBuffers[i].mDataByteSize, echoCancellation->pfar_pcm_file);
            }
        }*/
    }
//    NSLog(@"outputRenderTone");
    return 0;
}
// 设置远端和近段io回调
-(void)setupRemoteIOUnit:(AECAUGraph*)augStruct{
    //Open input of the bus 1(input mic)
    UInt32 inputEnableFlag = 1;
    CheckError(AudioUnitSetProperty(augStruct->remoteIOUnit,
                                    kAudioOutputUnitProperty_EnableIO,
                                    kAudioUnitScope_Input,
                                    1,
                                    &inputEnableFlag,
                                    sizeof(inputEnableFlag)),
               "Open input of bus 1 failed");
    
    //Open output of bus 0(output speaker)
    UInt32 outputEnableFlag = 1;
    CheckError(AudioUnitSetProperty(augStruct->remoteIOUnit,
                                    kAudioOutputUnitProperty_EnableIO,
                                    kAudioUnitScope_Output,
                                    0,
                                    &outputEnableFlag,
                                    sizeof(outputEnableFlag)),
               "Open output of bus 0 failed");
    
    UInt32 mFramesPerPacket = 1;
    UInt32 mBytesPerFrame = channel * bitsPerSample / 8;
    //Set up stream format for input and output
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    streamFormat.mSampleRate = samplerate;
    streamFormat.mFramesPerPacket = mFramesPerPacket;
    streamFormat.mBytesPerFrame = mBytesPerFrame;
    streamFormat.mBytesPerPacket = mBytesPerFrame * mFramesPerPacket;
    streamFormat.mBitsPerChannel = bitsPerSample;
    streamFormat.mChannelsPerFrame = channel;
    
    CheckError(AudioUnitSetProperty(augStruct->remoteIOUnit,
                                    kAudioUnitProperty_StreamFormat,
                                    kAudioUnitScope_Input,
                                    0,
                                    &streamFormat,
                                    sizeof(streamFormat)),
               "kAudioUnitProperty_StreamFormat of bus 0 failed");
    
    CheckError(AudioUnitSetProperty(augStruct->remoteIOUnit,
                                    kAudioUnitProperty_StreamFormat,
                                    kAudioUnitScope_Output,
                                    1,
                                    &streamFormat,
                                    sizeof(streamFormat)),
               "kAudioUnitProperty_StreamFormat of bus 1 failed");
    
    AURenderCallbackStruct input;
    input.inputProc = InputCallback_xb;
    input.inputProcRefCon = (__bridge void *)(self);
    CheckError(AudioUnitSetProperty(augStruct->remoteIOUnit,
                                    kAudioOutputUnitProperty_SetInputCallback,
                                    kAudioUnitScope_Output,
                                    1,
                                    &input,
                                    sizeof(input)),
               "couldnt set remote i/o render callback for output");
    
    AURenderCallbackStruct output;
    output.inputProc = outputRenderTone_xb;
    output.inputProcRefCon = (__bridge void *)(self);
    CheckError(AudioUnitSetProperty(augStruct->remoteIOUnit,
                                    kAudioUnitProperty_SetRenderCallback,
                                    kAudioUnitScope_Input,
                                    0,
                                    &output,
                                    sizeof(output)),
               "kAudioUnitProperty_SetRenderCallback failed");
}
// 设置录音和播放功能
-(void)setupSession
{
    NSError *error = nil;
    AVAudioSession* session = [AVAudioSession sharedInstance];
    recordCategory = [session category];
    [session setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:AVAudioSessionCategoryOptionDefaultToSpeaker error:&error];
    [session setActive:YES error:nil];
}


#pragma mark - 检查错误的方法
static void CheckError(OSStatus error, const char *operation)
{
    if (error == noErr) return;
    char errorString[20];
    // See if it appears to be a 4-char-code
    *(UInt32 *)(errorString + 1) = CFSwapInt32HostToBig(error);
    if (isprint(errorString[1]) && isprint(errorString[2]) &&
        isprint(errorString[3]) && isprint(errorString[4])) {
        errorString[0] = errorString[5] = '\'';
        errorString[6] = '\0';
    } else
        // No, format it as an integer
        sprintf(errorString, "%d", (int)error);
    fprintf(stderr, "Error: %s (%s)\n", operation, errorString);
    exit(1);
}



#pragma mark - 开启、停止服务
- (void)startService
{
    if (self.isRunningService == YES)
    {
        return;
    }
    
    long begin = [self getSysTime];
    //[self open_pcm_file:@"far.pcm" nearfile:@"near.pcm" aecoutfile:@"aecout.pcm"];
    // 设置声卡权限，同时支持录音和播放
    [self setupSession];
    
    NSLog(@"set record and play, spend time:%ld\n", [self getSysTime] - begin);
    // 创建AUgraph
    [self createAUGraph:&aecGraph];
    NSLog(@"createAUGraph, spend time:%ld\n", [self getSysTime] - begin);
    [self setupRemoteIOUnit:&aecGraph];
    NSLog(@"setupRemoteIOUnit, spend time:%ld\n", [self getSysTime] - begin);
    [self startGraph:aecGraph.graph];
    NSLog(@"startGraph, spend time:%ld\n", [self getSysTime] - begin);
    CheckError(AudioOutputUnitStart(aecGraph.remoteIOUnit), "AudioOutputUnitStart failed");
    NSLog(@"AudioOutputUnitStart, spend time:%ld\n", [self getSysTime] - begin);
    self.isRunningService = YES;
    NSLog(@"startService完成");
}

-(void)stopService
{
    self.on_aec_record_callback = nil;
    self.on_aec_render_callback = nil;

    [self stopGraph:aecGraph.graph];
    NSError *error = nil;
    AVAudioSession* session = [AVAudioSession sharedInstance];

    if(nil != recordCategory)
    {
        [session setCategory:recordCategory error:&error];
        recordCategory = NULL;
    }
    //[self close_pcm_file];
}

- (instancetype)init
{
    self = [super init];
    return self;
}

- (void)openAECRecord:(int)channel samplerate:(int)samplerate bitspersample:(int)bitspersample record_cb:(on_aec_record_block)record_cb
{
    self->channel = channel;
    self->samplerate = samplerate;
    self->bitsPerSample = bitspersample;
    self.isRunningService = NO;
    self.isNeedInputCallback = YES;
    self.on_aec_record_callback = record_cb;
    [self startService];
    
    //(NSString *)pconf_tag channcel:(int)channel samplerate:(int)samplerate bitspersample:(int)bitspersample
    pwav_mux = [[IWavMuxer alloc] initWithConfig:@"log_aec_output_path" channcel:channel samplerate:samplerate bitspersample:bitspersample];
}

- (void)closeAECRecord
{
    [self stopService];
    if(pwav_mux)
    {
        [pwav_mux close];
    }
}
@end

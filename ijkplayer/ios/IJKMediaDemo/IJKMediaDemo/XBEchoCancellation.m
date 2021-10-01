//
//  XBEchoCancellation.m
//  iOSEchoCancellation
//
//  Created by xxb on 2017/8/25.
//  Copyright © 2017年 xxb. All rights reserved.
//

#import "XBEchoCancellation.h"

typedef struct MyAUGraphStruct{
    AUGraph graph;
    AudioUnit remoteIOUnit;
} MyAUGraphStruct;


@interface XBEchoCancellation ()
{
    MyAUGraphStruct myStruct;
    int _rate;
    int _bit;
    int _channel;
    NSString* category;
    FILE* pfar_pcm_file;
    FILE* pnear_pcm_file;
    FILE* paec_out_pcm_file;
}
@property (nonatomic,assign) BOOL isRunningService; //是否运行着声音服务
@property (nonatomic,assign) BOOL isNeedInputCallback; //需要录音回调(获取input即麦克风采集到的声音回调)
@property (nonatomic,assign) BOOL isNeedOutputCallback; //需要播放回调(output即向发声设备传递声音回调)

@end

@implementation XBEchoCancellation

@synthesize streamFormat;

+ (instancetype)shared
{
    static XBEchoCancellation *echo = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        echo = [[XBEchoCancellation alloc] init];
    });
    return echo;
}

- (instancetype)init
{
    if (self = [super init])
    {
        _rate = 8000;
        _bit = 16;
        _channel = 1;
        _echoCancellationStatus = XBEchoCancellationStatus_close;
        self.isRunningService = NO;
        [self startService];
    }
    return self;
}

- (instancetype)initWithRate:(int)rate bit:(int)bit channel:(int)channel
{
    if (self = [super init])
    {
        _rate = rate;
        _bit = bit;
        _channel = channel;
        _echoCancellationStatus = XBEchoCancellationStatus_close;
        self.isRunningService = NO;
        [self startService];
    }
    return self;
}

- (void)dealloc
{
    NSLog(@"XBEchoCancellation销毁");
    [self stop];
}


#pragma mark - 开启或者停止音频输入、输出回调
- (void)startInput
{
    [self startService];
    self.isNeedInputCallback = YES;
}
- (void)stopInput
{
    self.isNeedInputCallback = NO;
}
- (void)startOutput
{
    [self startService];
    self.isNeedOutputCallback = YES;
}
- (void)stopOutput
{
    self.isNeedOutputCallback = NO;
}
NSString* get_doc_path_with_name(NSString* pname)
{
    NSString *homePath = NSHomeDirectory();
    NSLog(@"home根目录:%@", homePath);
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSLog(@"documens路径:%@", documentPath);
    NSString* path = [NSString stringWithFormat:@"%@/%@", documentPath, pname];
    return path;
}
- (void) open_pcm_file:(NSString*)farname nearfile:(NSString*)nearname aecoutfile:(NSString*)aecoutname
{
    if(farname)
    {
        pfar_pcm_file = fopen([get_doc_path_with_name(farname) UTF8String], "wb");
    }
    
    if(nearname)
    {
        pnear_pcm_file = fopen([get_doc_path_with_name(nearname) UTF8String], "wb");
    }
    
    if(aecoutname)
    {
        paec_out_pcm_file = fopen([get_doc_path_with_name(aecoutname) UTF8String], "wb");
    }
}
///pcm转WAV
- (void)pcm2WAV
{
    NSString *pcmPath = [NSHomeDirectory() stringByAppendingString:@"/Documents/near.pcm"];
    
    NSString *wavPath = [NSHomeDirectory() stringByAppendingString:@"/Documents/near.wav"];
    char *pcmPath_c = (char *)[pcmPath UTF8String];
    char *wavPath_c = (char *)[wavPath UTF8String];
    convertPcm2Wav(pcmPath_c, wavPath_c, 1, 16000);
    //进入沙盒找到xbMedia.wav即可
}



// pcm 转wav

//wav头的结构如下所示：

typedef  struct  {
    
    char        fccID[4];
    
    int32_t      dwSize;
    
    char        fccType[4];
    
} HEADER;

typedef  struct  {
    
    char        fccID[4];
    
    int32_t      dwSize;
    
    int16_t      wFormatTag;
    
    int16_t      wChannels;
    
    int32_t      dwSamplesPerSec;
    
    int32_t      dwAvgBytesPerSec;
    
    int16_t      wBlockAlign;
    
    int16_t      uiBitsPerSample;
    
}FMT;

typedef  struct  {
    
    char        fccID[4];
    
    int32_t      dwSize;
    
}DATA;

/*
 int convertPcm2Wav(char *src_file, char *dst_file, int channels, int sample_rate)
 请问这个方法怎么用?参数都是什么意思啊
 
 赞  回复
 code书童： @不吃鸡爪 pcm文件路径，wav文件路径，channels为通道数，手机设备一般是单身道，传1即可，sample_rate为pcm文件的采样率，有44100，16000，8000，具体传什么看你录音时候设置的采样率。
 */

int convertPcm2Wav(char *src_file, char *dst_file, int channels, int sample_rate)

{
    int bits = 16;
    
    //以下是为了建立.wav头而准备的变量
    
    HEADER  pcmHEADER;
    
    FMT  pcmFMT;
    
    DATA  pcmDATA;
    
    unsigned  short  m_pcmData;
    
    FILE  *fp,*fpCpy;
    
    if((fp=fopen(src_file,  "rb"))  ==  NULL) //读取文件
        
    {
        
        printf("open pcm file %s error\n", src_file);
        
        return -1;
        
    }
    
    if((fpCpy=fopen(dst_file,  "wb+"))  ==  NULL) //为转换建立一个新文件
        
    {
        
        printf("create wav file error\n");
        
        return -1;
        
    }
    
    //以下是创建wav头的HEADER;但.dwsize未定，因为不知道Data的长度。
    
    strncpy(pcmHEADER.fccID,"RIFF",4);
    
    strncpy(pcmHEADER.fccType,"WAVE",4);
    
    fseek(fpCpy,sizeof(HEADER),1); //跳过HEADER的长度，以便下面继续写入wav文件的数据;
    
    //以上是创建wav头的HEADER;
    
    if(ferror(fpCpy))
        
    {
        
        printf("error\n");
        
    }
    
    //以下是创建wav头的FMT;
    
    pcmFMT.dwSamplesPerSec=sample_rate;
    
    pcmFMT.dwAvgBytesPerSec=pcmFMT.dwSamplesPerSec*sizeof(m_pcmData);
    
    pcmFMT.uiBitsPerSample=bits;
    
    strncpy(pcmFMT.fccID,"fmt  ", 4);
    
    pcmFMT.dwSize=16;
    
    pcmFMT.wBlockAlign=2;
    
    pcmFMT.wChannels=channels;
    
    pcmFMT.wFormatTag=1;
    
    //以上是创建wav头的FMT;
    
    fwrite(&pcmFMT,sizeof(FMT),1,fpCpy); //将FMT写入.wav文件;
    
    //以下是创建wav头的DATA;  但由于DATA.dwsize未知所以不能写入.wav文件
    
    strncpy(pcmDATA.fccID,"data", 4);
    
    pcmDATA.dwSize=0; //给pcmDATA.dwsize  0以便于下面给它赋值
    
    fseek(fpCpy,sizeof(DATA),1); //跳过DATA的长度，以便以后再写入wav头的DATA;
    
    fread(&m_pcmData,sizeof(int16_t),1,fp); //从.pcm中读入数据
    
    while(!feof(fp)) //在.pcm文件结束前将他的数据转化并赋给.wav;
        
    {
        
        pcmDATA.dwSize+=2; //计算数据的长度；每读入一个数据，长度就加一；
        
        fwrite(&m_pcmData,sizeof(int16_t),1,fpCpy); //将数据写入.wav文件;
        
        fread(&m_pcmData,sizeof(int16_t),1,fp); //从.pcm中读入数据
        
    }
    
    fclose(fp); //关闭文件
    
    pcmHEADER.dwSize = 0;  //根据pcmDATA.dwsize得出pcmHEADER.dwsize的值
    
    rewind(fpCpy); //将fpCpy变为.wav的头，以便于写入HEADER和DATA;
    
    fwrite(&pcmHEADER,sizeof(HEADER),1,fpCpy); //写入HEADER
    
    fseek(fpCpy,sizeof(FMT),1); //跳过FMT,因为FMT已经写入
    
    fwrite(&pcmDATA,sizeof(DATA),1,fpCpy);  //写入DATA;
    
    fclose(fpCpy);  //关闭文件
    
    return 0;
    
}

- (void) close_pcm_file
{
    if(pfar_pcm_file)
    {
        fclose(pfar_pcm_file);
        pfar_pcm_file = NULL;
    }
    
    if(pnear_pcm_file)
    {
        fclose(pnear_pcm_file);
        pnear_pcm_file = NULL;
        [self pcm2WAV];
    }
    
    if(paec_out_pcm_file)
    {
        fclose(paec_out_pcm_file);
        paec_out_pcm_file = NULL;
    }
}
#pragma mark - 开启、停止服务
- (void)startService
{
    if (self.isRunningService == YES)
    {
        return;
    }
    [self open_pcm_file:@"far.pcm" nearfile:@"near.pcm" aecoutfile:@"aecout.pcm"];
    // 设置声卡权限，同时支持录音和播放
    [self setupSession];
    // 创建AUgraph
    [self createAUGraph:&myStruct];
    
    [self setupRemoteIOUnit:&myStruct];
    
    [self startGraph:myStruct.graph];
    
    CheckError(AudioOutputUnitStart(myStruct.remoteIOUnit), "AudioOutputUnitStart failed");
    
    self.isRunningService = YES;
    NSLog(@"startService完成");
}

#pragma mark - 开启或关闭回声消除
- (void)openEchoCancellation
{
    if (self.isRunningService == NO)
    {
        return;
    }
    [self openOrCloseEchoCancellation:0];
}
- (void)closeEchoCancellation
{
    if (self.isRunningService == NO)
    {
        return;
    }
    [self openOrCloseEchoCancellation:1];
}
///0 开启，1 关闭
-(void)openOrCloseEchoCancellation:(UInt32)newEchoCancellationStatus
{
    if (self.isRunningService == NO)
    {
        return;
    }
    UInt32 echoCancellation;
    UInt32 size = sizeof(echoCancellation);
    CheckError(AudioUnitGetProperty(myStruct.remoteIOUnit,
                                    kAUVoiceIOProperty_BypassVoiceProcessing,
                                    kAudioUnitScope_Global,
                                    0,
                                    &echoCancellation,
                                    &size),
               "kAUVoiceIOProperty_BypassVoiceProcessing failed");
    if (newEchoCancellationStatus == echoCancellation)
    {
        return;
    }
    
    CheckError(AudioUnitSetProperty(myStruct.remoteIOUnit,
                                    kAUVoiceIOProperty_BypassVoiceProcessing,
                                    kAudioUnitScope_Global,
                                    0,
                                    &newEchoCancellationStatus,
                                    sizeof(newEchoCancellationStatus)),
               "AudioUnitSetProperty kAUVoiceIOProperty_BypassVoiceProcessing failed");
    _echoCancellationStatus = newEchoCancellationStatus == 0 ? XBEchoCancellationStatus_open : XBEchoCancellationStatus_close;
}


#pragma mark - 初始化AUGraph和Audio Unit

-(void)startGraph:(AUGraph)graph
{
    CheckError(AUGraphInitialize(graph),
               "AUGraphInitialize failed");
    CheckError(AUGraphStart(graph),
               "AUGraphStart failed");
    _echoCancellationStatus = XBEchoCancellationStatus_open;
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
    _echoCancellationStatus = XBEchoCancellationStatus_close;
}


-(void)createAUGraph:(MyAUGraphStruct*)augStruct{
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
// 设置远端和近段io回调
-(void)setupRemoteIOUnit:(MyAUGraphStruct*)augStruct{
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
    UInt32 mBytesPerFrame = _channel * _bit / 8;
    //Set up stream format for input and output
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    streamFormat.mSampleRate = _rate;
    streamFormat.mFramesPerPacket = mFramesPerPacket;
    streamFormat.mBytesPerFrame = mBytesPerFrame;
    streamFormat.mBytesPerPacket = mBytesPerFrame * mFramesPerPacket;
    streamFormat.mBitsPerChannel = _bit;
    streamFormat.mChannelsPerFrame = _channel;
    
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
    category = [session category];
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

#pragma mark - 回调函数
OSStatus InputCallback_xb(void *inRefCon,
                       AudioUnitRenderActionFlags *ioActionFlags,
                       const AudioTimeStamp *inTimeStamp,
                       UInt32 inBusNumber,
                       UInt32 inNumberFrames,
                       AudioBufferList *ioData){
    
    XBEchoCancellation *echoCancellation = (__bridge XBEchoCancellation*)inRefCon;
    if (echoCancellation.isNeedInputCallback == NO)
    {
//        NSLog(@"没有开启声音输入回调");
        return noErr;
    }
    MyAUGraphStruct *myStruct = &(echoCancellation->myStruct);
    
    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0].mData = NULL;
    bufferList.mBuffers[0].mDataByteSize = 0;

    AudioUnitRender(myStruct->remoteIOUnit,
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
    
    if(echoCancellation->pnear_pcm_file && bufferList.mNumberBuffers > 0)
    {
        for(int i = 0; i < bufferList.mNumberBuffers; i++)
        {
            fwrite(bufferList.mBuffers[i].mData, 1, bufferList.mBuffers[i].mDataByteSize, echoCancellation->pnear_pcm_file);
        }
    }

    if (echoCancellation.bl_input)
    {
        echoCancellation.bl_input(&bufferList);
    }
    /*if(echoCancellation->paec_out_pcm_file && ioData->mNumberBuffers > 0)
    {
        for(int i = 0; i < ioData->mNumberBuffers; i++)
        {
            fwrite(ioData->mBuffers[i].mData, 1, ioData->mBuffers[i].mDataByteSize, echoCancellation->paec_out_pcm_file);
        }
    }*/
//    NSLog(@"InputCallback");
    return noErr;
}
OSStatus outputRenderTone_xb(
                          void *inRefCon,
                          AudioUnitRenderActionFlags 	*ioActionFlags,
                          const AudioTimeStamp 		*inTimeStamp,
                          UInt32 						inBusNumber,
                          UInt32 						inNumberFrames,
                          AudioBufferList 			*ioData)

{
    //TODO: implement this function
    memset(ioData->mBuffers[0].mData, 0, ioData->mBuffers[0].mDataByteSize);
    
    XBEchoCancellation *echoCancellation = (__bridge XBEchoCancellation*)inRefCon;
    if (echoCancellation.isNeedOutputCallback == NO)
    {
        //        NSLog(@"没有开启声音输出回调");
        return noErr;
    }
    


    if (echoCancellation.bl_output)
    {
        echoCancellation.bl_output(ioData,inNumberFrames);
        if(echoCancellation->pfar_pcm_file && ioData->mNumberBuffers > 0)
        {
            for(int i = 0; i < ioData->mNumberBuffers; i++)
            {
                fwrite(ioData->mBuffers[i].mData, 1, ioData->mBuffers[i].mDataByteSize, echoCancellation->pfar_pcm_file);
            }
        }
    }
//    NSLog(@"outputRenderTone");
    return 0;
}


#pragma mark - 其他方法

+ (void)volume_controlOut_buf:(short *)out_buf in_buf:(short *)in_buf in_len:(int)in_len in_vol:(float)in_vol
{
    volume_control(out_buf, in_buf, in_len, in_vol);
}

// 音量控制
// output: para1 输出数据
// input : para2 输入数据
//         para3 输入长度
//         para4 音量控制参数,有效控制范围[0,100]
// 超过100，则为倍数，倍数为in_vol减去98的数值
int volume_control(short* out_buf,short* in_buf,int in_len, float in_vol)
{
    int i,tmp;

    // in_vol[0,100]
    float vol = in_vol - 98;

    if(-98 < vol  &&  vol <0 )
    {
        vol = 1/(vol*(-1));
    }
    else if(0 <= vol && vol <= 1)
    {
        vol = 1;
    }
    /*else if(1 < vol && vol <= 2)
     {
     vol = vol;
     }*/
    else if(vol <= -98)
    {
        vol = 0;
    }
    //    else if(2 = vol)
    //    {
    //        vol = 2;
    //    }

    for(i=0; i<in_len/2; i++)
    {
        tmp = in_buf[i]*vol;
        if(tmp > 32767)
        {
            tmp = 32767;
        }
        else if( tmp < -32768)
        {
            tmp = -32768;
        }
        out_buf[i] = tmp;
    }

    return 0;
}

- (void)stop
{
    self.bl_input = nil;
    self.bl_output = nil;
    [self stopGraph:myStruct.graph];
    NSError *error = nil;
    AVAudioSession* session = [AVAudioSession sharedInstance];
    //category = [session category];
    if(nil != category)
    {
        [session setCategory:category error:&error];
        category = NULL;
    }
    [self close_pcm_file];
}

@end

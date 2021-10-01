//
//  Recorder.m
//  PPCS_Client
//
//  Created by Elson on 2018/2/9.
//  Copyright © 2018年 internet. All rights reserved.
//

#import "Recorder.h"
//#import "g711.h"
//#import "ZBLog.h"

#define audioByte 640
FILE* pfile = NULL;
@interface Recorder()

@property (nonatomic) int frameNum;

@end

@implementation Recorder
char* paec_data;
int aec_data_len;
char* paac_data;
int aac_data_len;

//IJKMediaEncoder*    mediaenc;

static Recorder *cSelf;

- (id)initRecord:(id<IJKMediaPlayback>)player
//- (id)initRecord:(IJKMediaPlayback*)player
{
    self = [super init];
    if (self) {
        self.sampleRate = kDefaultSampleRate;
        
        //设置录音 初始化录音参数
        [self setupAudioFormat:kAudioFormatLinearPCM SampleRate:(int)self.sampleRate];
        cSelf = self;
        self.audioData = [NSMutableData new];
    }
    self->_player = player;
    aec_data_len = 1024;
    paec_data = (char*)malloc(aec_data_len);
    aac_data_len = 1024;
    paac_data = (char*)malloc(aac_data_len);
    bechocancel = false;
    NSLog(@"initRecord begin\n");
    return self;
}
- (id)init
{
    self = [super init];
    if (self) {
        self.sampleRate = kDefaultSampleRate;
        
        //设置录音 初始化录音参数
        [self setupAudioFormat:kAudioFormatLinearPCM SampleRate:(int)self.sampleRate];
        cSelf = self;
        self.audioData = [NSMutableData new];
    }
    return self;
}
//设置录音 初始化录音参数
- (void)setupAudioFormat:(UInt32)inFormatID SampleRate:(int)sampeleRate
{
    memset(&audioFormat, 0, sizeof(audioFormat));
    audioFormat.mSampleRate = sampeleRate;//采样率
    audioFormat.mChannelsPerFrame = 1;//单声道
    audioFormat.mFormatID = inFormatID;//采集pcm 格式
    audioFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    audioFormat.mBitsPerChannel = 16;//每个通道采集2个byte
    audioFormat.mBytesPerPacket = audioFormat.mBytesPerFrame = (audioFormat.mBitsPerChannel / 8) * audioFormat.mChannelsPerFrame;
    audioFormat.mFramesPerPacket = 1;
    //init:(int)mt codecID:(int)codecid Param1:(int)param1 Param2:(int)param2 Param3:(int)param3 format:(int)format bitrate:(int)bitrate;
    _aac_enc = [[IJKMediaEncoder alloc] init:1 codecID:86018 Param1:audioFormat.mChannelsPerFrame Param2:audioFormat.mSampleRate Param3:1 format:1  bitrate:64000];
    NSLog(@"create aac enc self->_aac_enc:%p\n", self->_aac_enc);
    //self->_aac_enc = [[IJKMediaEncoder alloc]init:1 codecID:86018 Param1:1 Param2:16000 Param3:1 format:1 bitrate:64000];
}

NSString* get_doc_pathex(NSString* pname)
{
    NSString *homePath = NSHomeDirectory();
    NSLog(@"home根目录:%@", homePath);
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSLog(@"documens路径:%@", documentPath);
    NSString* path = [NSString stringWithFormat:@"%@/%@", documentPath, pname];
    return path;
}

//回调函数 不断采集声音。
void inputHandler(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer, const AudioTimeStamp *inStartTime,UInt32 inNumPackets, const AudioStreamPacketDescription *inPacketDesc)
{
    int ret = 0;
    Recorder *recorder = (__bridge Recorder*)inUserData;
    
    if(!recorder->bechocancel)
    {
        recorder->bechocancel = true;
        [recorder->_player startEchoCancel:recorder->audioFormat.mChannelsPerFrame samplerate:recorder->audioFormat.mSampleRate sampleformat:1 nbsamples:160 delay:210000];
    }
    NSLog(@"inBuffer->mAudioData:%p, inBuffer->mAudioDataByteSize%d\n", inBuffer->mAudioData, inBuffer->mAudioDataByteSize);
   // NSData *SpeechData = [[NSData alloc]initWithBytes:recorder->data+i*audioByte length:audioByte]
    int out_len = [recorder->_player echoCancelDeliverData:inBuffer->mAudioData outdata:recorder->aac_data data_len:inBuffer->mAudioDataByteSize];
    //NSLog(@"after echoCancelDeliverData, recorder->_aac_enc:%p, out_len:%d\n", recorder->_aac_enc, out_len);
    /*if(recorder->_aac_enc && out_len > 0)
    {
        // - (int)deliverData:(char*)rawdata rawdatalen:(int)rawdatalen encdata:(char*)encdata encdatalen:(int)encdatalen present_time:(long long)pts_in_ms
        ret = [recorder->_aac_enc deliverData:recorder->aac_data rawdatalen:out_len encdata:recorder->aac_data encdatalen:1024 present_time:0];
        if(ret < 0)
        {
            //assert(0);
        }
        
        //[recorder->_aac_enc  getEncData:];
    }*/
    /*if(NULL == pfile)
    {
        NSString* pcmpath = get_doc_pathex(@"raw_44.1k.pcm");
        pfile = fopen([pcmpath UTF8String], "wb");
    }
    if(pfile)
    {
        fwrite(inBuffer->mAudioData,1, inBuffer->mAudioDataByteSize, pfile);
    }
    int k = 0;
    if (inBuffer->mAudioDataByteSize > 0) {
        memcpy(recorder->data+recorder->offset,inBuffer->mAudioData,inBuffer->mAudioDataByteSize);//通过recorder->offset 偏移把语音数据存入recorder->data
        recorder->offset+=inBuffer->mAudioDataByteSize;//记录语音数据的大小
        k = recorder->offset/audioByte;//计算语音数据有多个audioByte个字节语音
        
        for(int i = 0; i <k; i++)
        {
            
            NSData *SpeechData = [[NSData alloc]initWithBytes:recorder->data+i*audioByte length:audioByte];//把recorder->data 数据以audioByte个字节分切放入 传出的数组中。
            //[[NSNotificationCenter defaultCenter] postNotificationName:RECORDER_NOTIFICATION_CALLBACK_NAME object:SpeechData];
            [cSelf.audioData appendData:SpeechData];
            if ((cSelf.audioData.length % 160)==0) {
                if (cSelf.audioData.length == 640) {
                    if (cSelf.callBack) {
                        
                        Byte *byteData = (Byte *)[cSelf.audioData bytes];
                        short *pPcm = (short *)byteData;
                        int outlen = 0;
                        int len =(int)cSelf.audioData.length / 2;
                        Byte * G711Buff = (Byte *)malloc(len);
                        memset(G711Buff,0,len);
                        int i;
                        for (i=0; i<len; i++) {
                            //此处修改转换格式（a-law或u-law）
                            G711Buff[i] = linear2alaw(pPcm[i]);
                        }
                        outlen = i;
                        Byte *sendbuff = (Byte *)G711Buff;
                        cSelf.callBack((uint8_t *)sendbuff, outlen, cSelf.frameNum++);
                        
//                        NSLog(@"data.leng = %lu",(unsigned long)cSelf.audioData.length);
//                        cSelf.callBack((uint8_t *)[cSelf.audioData bytes],cSelf.audioData.length,0);
                    }
                    cSelf.audioData = [NSMutableData new];
                }
            }else{
                cSelf.audioData = [NSMutableData new];
            }
        }
        
        memcpy(recorder->data,recorder->data+k*audioByte,recorder->offset-(k*audioByte));//把剩下的语音数据放入原来的数组中
        
        recorder->offset-=(k*audioByte);//计算剩下的语音数据大小
        
        
    }*/
    
    if (recorder.isRecording) {
        AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
    }
    
}

long getCurTime()
{
    NSDate *datenow = [NSDate date];
    return (long)([datenow timeIntervalSince1970] *1000);
}
//开始录音
-(void)start
{
    NSError *error = nil;
    long begintime = getCurTime();
    NSLog(@"record start begin\n");
    self.frameNum = 0;
    //录音的设置和初始化
    //BOOL ret = [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord error:&error];
    /*AVAudioSession *session = [AVAudioSession sharedInstance];
    NSString* cat = [session category];

    [session setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:AVAudioSessionCategoryOptionDefaultToSpeaker | AVAudioSessionCategoryOptionMixWithOthers error:nil];//AVAudioSessionCategoryMultiRoute:585ms/573, AVAudioSessionCategoryPlayAndRecord:651/729
    NSLog(@"record create session, spend time:%ld ms\n", getCurTime() - begintime);
    BOOL ret = [session setActive:YES error:&error];*/
    BOOL ret = [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:AVAudioSessionCategoryOptionDefaultToSpeaker | AVAudioSessionCategoryOptionMixWithOthers error:nil];
    
    
    //启用audio session
    ret = [[AVAudioSession sharedInstance] setActive:YES error:&error];
    if (!ret)
    {
        NSLog(@"声音开启失败");
        return;
    }
    
    
    //return ;
    NSLog(@"record start before AudioQueueNewInput, spend time:%ld ms\n", getCurTime() - begintime);
    //初始化缓冲语音数据数组
    memset(data,0,1024);
    offset = 0;
    audioFormat.mSampleRate = self.sampleRate;
    //初始化音频输入队列
    AudioQueueNewInput(&audioFormat, inputHandler, (__bridge void *)(self), NULL, NULL, 0, &_audioQueue);
    NSLog(@"record start after AudioQueueNewInput, spend time:%ld ms\n", getCurTime() - begintime);
    int bufferByteSize = audioByte; //设定采集一帧audioByte个字节
    
    //创建缓冲器
    for (int i = 0; i < kNumberAudioQueueBuffers; ++i){
        AudioQueueAllocateBuffer(_audioQueue, bufferByteSize, &_audioBuffers[i]);
        AudioQueueEnqueueBuffer(_audioQueue, _audioBuffers[i], 0, NULL);
    }
    NSLog(@"record start before AudioQueueStart(_audioQueue, NULL), spend time:%ld ms\n", getCurTime() - begintime);
    
    //开始录音
    AudioQueueStart(_audioQueue, NULL);
    self.isRecording = YES;
    NSLog(@"record start end, spend time:%ld ms\n", getCurTime() - begintime);
    /*if(!bechocancel)
    {
        bechocancel = true;
        [_player startEchoCancel:audioFormat.mChannelsPerFrame samplerate:audioFormat.mSampleRate sampleformat:1 nbsamples:160 delay:450000];// delay = near_pts - far_pts
    }*/
}

//结束录音
-(void)stop
{
    NSLog(@"record stop begin\n");
    //if (self.isRecording) { //这个判断不准，去掉
        self.isRecording = NO;
        AudioQueueStop(_audioQueue, true);
        AudioQueueDispose(_audioQueue, true);
        [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:nil];
        //[[AVAudioSession sharedInstance] setActive:NO error:nil]; //设置成no将不会有声音
        self.audioData = [NSMutableData new];
        self.callBack = nil;
        //logDebug(@"==========> 停止录音方法");
    self.frameNum = 0;
    [self.player stopEchoCancel];
    NSLog(@"record stop end\n");
    //}
}

@end


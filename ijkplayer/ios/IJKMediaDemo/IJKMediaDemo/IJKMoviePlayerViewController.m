/*
 * Copyright (C) 2013-2015 Bilibili
 * Copyright (C) 2013-2015 Zhang Rui <bbcallen@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import "IJKMoviePlayerViewController.h"
#import "IJKMediaControl.h"
#import "IJKCommon.h"
#import "IJKDemoHistory.h"
#import "Recorder.h"
#import "XBEchoCancellation.h"
//#import "IJKMediaMuxer.h"
//#import <Photos/Photos.h>
#import "PHPhotoLibrary/PHPhotoLibrary+CustomPhotoAlbum.h"
//#import "AECRecorder.h"
//#import "PHPhotoLibrary.h"

@interface IJKVideoViewController()

@end
#define REC_CODEC_ID_H264 28
#define REC_CODEC_ID_HEVC 174
#define REC_CODEC_ID_AAC  86018
@implementation IJKVideoViewController
NSThread*   deliver_thread;
NSCondition* _condition;
NSString* videoPath;
IJKMediaRecorder* prec;
XBEchoCancellation* pxbaec;
AECRecorder*    paecrecord;
int     brecord;
bool    bloop;
bool    bcomplete;
bool    baec;
Recorder* paudio_record;

IWavMuxer* pwav_mux;
IJKMediaEncoder* pmedia_enc;
FILE* paac_file;
- (void)dealloc
{
}

-(int)eventNotify:(int)msg wparam:(int)wparam lparam:(int)lparam
{
    if(2003 == msg)
    {
        NSLog(@"record end of stream\n");
        if(prec)
        {
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [prec stop];
                prec = NULL;
            });
        }
    }
    else
    {
        NSLog(@"msg:%d, wparam:%d, lparam:%d, percent:%d\n", msg, wparam, lparam, [prec getPercent]);
    }
}

- (IBAction)onDownload:(id)sender {
    if(NULL == prec)
    {
        NSString *homePath = NSHomeDirectory();
        NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
        NSString* sink = [NSString stringWithFormat:@"%@/%@.mov", documentPath, @"out"];
        NSString* tmp = NULL;//[NSString stringWithFormat:@"%@/%@.mov", documentPath, @"tmp"];
        prec = [[IJKMediaRecorder alloc] initRecord:[self.url absoluteString]  sinkurl:sink tmpurl:tmp];
        [prec setEventNotify:self];
        [prec start];
    }
}
- (IBAction)oncapture:(id)sender {
    /*NSString *homePath = NSHomeDirectory();
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];

    formatter.dateFormat = @"yyyyMMdd-HHmmss";

    NSString *time = [formatter stringFromDate:[NSDate date]];
    NSString* path = [NSString stringWithFormat:@"%@/%@.jpg", documentPath, time];
    [self.player saveImageJpg:path];
     NSURL *fileUrl = [NSURL fileURLWithPath:path];*/
    char* pjpgbuf = malloc(1024*1024);
    int len = 1024*1024;
    
    int jpglen = [self.player getImageJpg:pjpgbuf dataLen:len];
    if(jpglen > 0)
    {
    NSData* pdata = [[NSData alloc] initWithBytes:pjpgbuf length:jpglen];
    
    //[[PHPhotoLibrary sharedPhotoLibrary] saveImageData:
    /*[[PHPhotoLibrary sharedPhotoLibrary] saveImageWithImageUrl:fileUrl ToAlbum:@"test" completion:completion:^(PHAsset *videoAsset) {
        NSLog(@"save jpg success");
    } failure:^(NSError *error) {
        NSLog(@"save jpg failed:%@", [error localizedDescription]);
    }];*/
    [[PHPhotoLibrary sharedPhotoLibrary]  saveImageData:pdata ToAlbum:@"ijkjpg" completion:^(PHAsset *videoAsset) {
        NSLog(@"save jpg success");
    } failure:^(NSError *error) {
        NSLog(@"save jpg failed:%@", [error localizedDescription]);
    }];
    }
    free(pjpgbuf);
    /*[[PHPhotoLibrary sharedPhotoLibrary]  saveImageWithImageUrl:fileUrl ToAlbum:@"test" completion:^(PHAsset *videoAsset) {
        
    } failure:^(NSError *error) {
        
    }];*/
    
    
   
    
    /*[[PHImageManager defaultManager] requestPlayerItemForVideo:videoAsset options:nil resultHandler:^(AVPlayerItem * _Nullable playerItem, NSDictionary * _Nullable info) {
         NSString *filePath = [info valueForKey:@"PHImageFileSandboxExtensionTokenKey"];
         if (filePath && filePath.length > 0) {
              NSArray *lyricArr = [filePath componentsSeparatedByString:@";"];
              NSString *privatePath = [lyricArr lastObject];
              if (privatePath.length > 8) {
                   NSString *videoPath = [privatePath substringFromIndex:8];
                   NSLog(@"videoPath = %@",videoPath);
                  NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
                  formatter.dateFormat = @"yyyyMMdd-HHmmss";

                  NSString *time = [formatter stringFromDate:[NSDate date]];
                  NSString* path = [NSString stringWithFormat:@"%@/%@.jpg", videoPath, time];
                  [self.player saveImageJpg:path];
                }
           }
    }];
    NSString *homePath = NSHomeDirectory();
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];

    formatter.dateFormat = @"yyyyMMdd-HHmmss";

    NSString *time = [formatter stringFromDate:[NSDate date]];
    NSString* path = [NSString stringWithFormat:@"%@/%@.jpg", documentPath, time];
    [self.player saveImageJpg:path];*/
}
+ (void)presentFromViewController:(UIViewController *)viewController withTitle:(NSString *)title URL:(NSURL *)url completion:(void (^)())completion {
    IJKDemoHistoryItem *historyItem = [[IJKDemoHistoryItem alloc] init];
    
    historyItem.title = title;
    historyItem.url = url;
    [[IJKDemoHistory instance] add:historyItem];
    
    //[viewController presentViewController:[[IJKVideoViewController alloc] initWithURL:url] animated:YES completion:completion];
    [viewController.navigationController pushViewController:[[IJKVideoViewController alloc] initWithURL:url] animated:YES];
    viewController.navigationController.hidesBarsOnSwipe = YES;
}

- (instancetype)initWithURL:(NSURL *)url {
    self = [self initWithNibName:@"IJKMoviePlayerViewController" bundle:nil];
    if (self) {
        self.url = url;
    }
    baec = false;
    paudio_record = NULL;
    pwav_mux = NULL;
    paac_file = NULL;
    return self;
}
- (BOOL) IsPlayRecord
{
    NSString* path = [self.url absoluteString];
    if([path containsString:@".data"])
    {
        return TRUE;
    }
    return FALSE;
}
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

#define EXPECTED_IJKPLAYER_VERSION (1 << 16) & 0xFF) | 
- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.

//    [[UIApplication sharedApplication] setStatusBarHidden:YES];
//    [[UIApplication sharedApplication] setStatusBarOrientation:UIInterfaceOrientationLandscapeLeft animated:NO];
    brecord = 0;
    bloop = false;
    _condition = nil;
    videoPath = nil;
    bcomplete = false;
    pxbaec = nil;
    pmedia_enc = nil;
#ifdef DEBUG
    NSString *homePath = NSHomeDirectory();
    //NSLog(@"home根目录:%@", homePath);
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    //NSLog(@"documens路径:%@", documentPath);
    /*NSDate* date = [NSDate date];
    NSDateFormatter* dateFormat = [[NSDateFormatter alloc] init];
    [dateFormat setDateFormat:@"HHmmss"];
    NSString* dateStr = [dateFormat stringFromDate:date];
    NSLog(@"当前时间%@",dateStr);*/
    NSString* logpath = [NSString stringWithFormat:@"%@/log/ijkplayer_", documentPath];
    //ijkmp_global_init_log([logpath UTF8String], 3, 3);
    //setLog:(NSString*) path loglevel:(int)logLevel outputflag:(int)outputflag;
    [IJKFFMoviePlayerController setLog:logpath loglevel:3 outputflag:3];
#else
    [IJKFFMoviePlayerController setLog:nil loglevel:4 outputflag:1];
                //[IJKFFMoviePlayerController setLog:nil ];
#endif

    [IJKFFMoviePlayerController checkIfFFmpegVersionMatch:YES];
    // [IJKFFMoviePlayerController checkIfPlayerVersionMatch:YES major:1 minor:0 micro:0];

    IJKFFOptions *options = [IJKFFOptions optionsByDefault];

    if([self IsPlayRecord])
    {
        NSURL* url = [[NSURL alloc] initWithString:@"live"] ;
        self.player = [[IJKFFMoviePlayerController alloc] initWithContentURL:url withOptions:options];
    }
    else
    {
    //self.player = [[IJKFFMoviePlayerController alloc] initWithContentURL:nil withOptions:options];
        self.player = [[IJKFFMoviePlayerController alloc] initWithContentURL:self.url withOptions:options];
    }
    self.player.view.autoresizingMask = UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
    self.player.view.frame = self.view.bounds;
    self.player.scalingMode = IJKMPMovieScalingModeAspectFit;
    self.player.shouldAutoplay = YES;

    self.view.autoresizesSubviews = YES;
    [self.view addSubview:self.player.view];
    [self.view addSubview:self.mediaControl];

    self.mediaControl.delegatePlayer = self.player;
}

-(void) viewDidUnload
{
    bloop = false;
    if(deliver_thread && _condition)
    {
        [deliver_thread cancel];
        [_condition wait];
    }
}
/*- (void) muxer_loop
{
    NSString *homePath = NSHomeDirectory();
       NSLog(@"home根目录:%@", homePath);
    //2.获取Documents路径

        NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
       NSLog(@"documens路径:%@", documentPath);
    NSString* path = [NSString stringWithFormat:@"%@%@", documentPath, @"/test.mov"];
    IJKMediaMuxer* muxer = [[IJKMediaMuxer alloc] initWithSinkUrl:path];
    void* precord_demux = record_open([purl UTF8String]);
    //NSData *byteData = [[NSData alloc] dataWithBytes:nil length:1024*1024];
    Byte buf[256000];
    //NSData* pdata = [[NSData alloc] initWithBytes:nil length:1024*1024];
    //Byte buf[1024*1024];
    char* pbuf = &buf[0];//[pdata bytes];//[byteData getBytes];
    int buf_len = 1024*1024;
    while(precord_demux)
    {
        int mt = -1;
        long pts = 0;
        int size = (precord_demux, &mt, pbuf, buf_len, &pts);
        if(size > 0)
        {
            [self deliver_data:mt mediadata:pbuf data_size:size present_time:pts];
            [muxer deliverData:mt mediadata:pbuf data_size:size present_time:pts];
            //usleep(20000);
        }
        else
        {
            break;
            record_close(precord_demux);
            precord_demux = record_open([purl UTF8String]);
            //break;
        }
    }
    
    [muxer stop];
    
}*/
NSString* get_doc_path(NSString* pname)
{
    NSString *homePath = NSHomeDirectory();
    NSLog(@"home根目录:%@", homePath);
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSLog(@"documens路径:%@", documentPath);
    NSString* path = [NSString stringWithFormat:@"%@/%@", documentPath, pname];
    return path;
}

- (void)taskrun
{
    NSString *homePath = NSHomeDirectory();
    NSLog(@"home根目录:%@", homePath);
    //2.获取Documents路径
       /*参数一：指定要搜索的文件夹枚举值
         参数二：指定搜索的域Domian: NSUserDomainMask
         参数三：是否需要绝对/全路径：NO：波浪线~标识数据容器的根目录; YES(一般制定): 全路径
        */
        NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
       NSLog(@"documens路径:%@", documentPath);
    NSString* path = [NSString stringWithFormat:@"%@%@", documentPath, @"/154632_1_0_d.data"];
}
-(long) get_cur_timestamp
{
    NSDate *datenow = [NSDate date];
    //NSDate* dat = [NSDate dateWithTimeIntervalSinceNow:0];
    NSTimeInterval curtime = [datenow timeIntervalSince1970];
    
    return (long)(curtime * 1000);
}
-(void) packet_loop
{
    NSString* path = [self.url isFileURL] ? [self.url path] : [self.url absoluteString];
    bloop = true;
    if(nil == _condition)
    {
        _condition = [[NSCondition alloc] init];
    }
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
       NSLog(@"documens路径:%@", documentPath);
    NSString* sink = [NSString stringWithFormat:@"%@%@", documentPath, @"/rec_out.mov"];
    NSString* tmp = [NSString stringWithFormat:@"%@%@", documentPath, @"/tmp2.mov"];
    prec = NULL;
    prec = [[IJKMediaRecorder alloc] initRecord:NULL sinkurl:sink tmpurl:nil];
    [prec start];
    //[_condition reset];
    //[_condition lock];
    long long start_time = -1;
    long start_timestamp = [self get_cur_timestamp];
    int bfirstframe = 1;
    int codec_id = -1;
    int64_t offset = 0;
    int len = sizeof(long);
    int eof = 0;
    long vframenum = 0, aframenum = 0, frame_num = 0;
    if(path)
    {
        [self.player openRecord:path];
        Byte buf[1024000];
        //[self.player startEchoCancel:1 samplerate:8000 sampleformat:1 nbsamples:160 delay:40000];
        while(![[NSThread currentThread] isCancelled])
        {
            int mt = -1;
            long long pts = -1;
            if(eof)
            {
                if(bcomplete)
                {
                    break;
                }
                usleep(40000);
                continue;
            }
            //int per = [prec getPercent];
            int pktlen = [self.player readFrame:buf frameSize:1024000 mediaType:&mt presentTime:&pts frame_num:&frame_num];
            if(pktlen <= 0)
            {
                //[self.player deliverData:REC_CODEC_ID_H264 mediadata:NULL data_size:0 present_time:0];
                //[self.player deliverData:REC_CODEC_ID_AAC mediadata:NULL data_size:0 present_time:0];
                //[prec deliverData:REC_CODEC_ID_H264 data:nil data_size:0 pts:0 frame_num:frame_num];
                //[prec deliverData:REC_CODEC_ID_AAC data:nil data_size:0 pts:0 frame_num:frame_num];
                /*eof = 0;
                offset +=  3600000;
                [self.player closeRecord];
                [self.player openRecord:path];
                //continue;*/
                break;
            }
            if(bfirstframe)
            {
                Byte  jpgdata[1024000];
                //(int)Video_KeyFrame_To_JPG:(int)codecid penc_data:(char*)ph264 h264len:(int)h264len jpgdata:(char*)pjpg jpglen:(int)jpglen;
                int jpglen =  [IJKMediaEncoder Video_KeyFrame_To_JPG:mt == 0 ? REC_CODEC_ID_H264 : REC_CODEC_ID_HEVC penc_data:buf datalen:pktlen jpgdata:jpgdata jpglen:1024000];
                if(jpglen > 0)
                {
                    NSString* jpgpath = get_doc_path(@"h264_to_jpg.jpg");
                    FILE* pfile = fopen([jpgpath UTF8String], "wb");
                    if(pfile)
                    {
                        fwrite(jpgdata, 1, jpglen, pfile);
                        fclose(pfile);
                    }
                }
                bfirstframe = 0;
            }
            if(-1 == start_time)
            {
                start_time = pts;
            }
            pts = pts - start_time;
            if(0 == mt)
            {
                //codec_id = 0;
                codec_id = REC_CODEC_ID_H264;
                //frame_num = vframenum;
                //vframenum++;
                //continue;
            }
            else if(1 == mt)
            {
                codec_id = REC_CODEC_ID_HEVC;
                //frame_num = vframenum;
                //vframenum++;
            }
            else if(3 == mt)
            {
                //codec_id = 1;
                codec_id = REC_CODEC_ID_AAC;
                //frame_num = aframenum;
                //aframenum++;
                //continue;
            }
            /*if(77 == vframenum++%78)
            {
                continue;
            }*/
            //pts = 0;
            NSLog(@"mt:%d, pts:%lld, size:%d", mt, pts + offset, pktlen);
            [self.player deliverData:codec_id mediadata:buf data_size:pktlen present_time:pts+offset frame_num:-1];
            //devData:(int)codecid data:(char*)pdata size:(int)size pts:(int64_t)pts framenum:(long)framenum;
            //[prec deliverData:(int)codec_id data:(char*)buf data_size:(int)pktlen pts:(int64_t)pts frame_num:(long)frame_num];
            [prec deliverData:codec_id data:(char*)buf size:pktlen pts:pts framenum:-1];
            long cur_ts = self.player.currentVideoTimeStamp;
            //[prec sendData:codec_id data:NULL data_size:0 pts:0];
#if 0
            usleep(35000);
#else
            while([self get_cur_timestamp] - start_timestamp < pts+offset - 20)
            {
                usleep(20000);
            };
#endif
        }
        [self.player deliverData:REC_CODEC_ID_H264 mediadata:nil data_size:0 present_time:0  frame_num:0];
        [self.player deliverData:REC_CODEC_ID_AAC mediadata:nil data_size:0 present_time:0   frame_num:0];
        [prec deliverData:REC_CODEC_ID_H264 data:nil size:0 pts:0 framenum:0];
        [prec deliverData:REC_CODEC_ID_AAC data:nil size:0 pts:0 framenum:0];
        [prec stop];
        [self.player closeRecord];
        NSLog(@"before sleep 1\n");
        //usleep(2000000);
        NSLog(@"after sleep 1\n");
        [_condition signal];
        [NSThread exit];
    }
}

- (void)onAecData:(AudioBufferList*)bufferList
{
    AudioBuffer buffer = bufferList->mBuffers[0];
                NSData *pcmBlock = [NSData dataWithBytes:buffer.mData length:buffer.mDataByteSize];
                //deliverData:pcmData:(char*)pdata dataLen:(int)dataLen;
                [pwav_mux deliverData:(char*)buffer.mData dataLen:0];
                //[pwav_mux deliverData:(char*)buffer.mData dataLen:buffer.mDataByteSize];
                /*NSString *savePath = stroePath;
                if ([[NSFileManager defaultManager] fileExistsAtPath:savePath] == false)
                {
                    [[NSFileManager defaultManager] createFileAtPath:savePath contents:nil attributes:nil];
                }
                NSFileHandle * handle = [NSFileHandle fileHandleForWritingAtPath:savePath];
                [handle seekToEndOfFile];
                [handle writeData:pcmBlock];*/
}

long getSysTime()
{
    NSDate *datenow = [NSDate date];
    return (long)([datenow timeIntervalSince1970] *1000);
}

- (void)startpAECRecord
{
    if(nil == paecrecord)
    {
        //init:mediatype codecID:codecid Param1:channel Param2:samplerate Param3:0 format:foramt bitrate:bitrate];
        //- (id)init:(int)mt codecID:(int)codecid Param1:(int)param1 Param2:(int)param2 Param3:(int)param3 format:(int)format bitrate:(int)bitrate;
        pmedia_enc = [[IJKMediaEncoder alloc] init:(int)0 codecID:(int)0x15002 Param1:(int)1 Param2:(int)16000 Param3:(int)0 format:(int)1 bitrate:(int)64000];
                                              //init:0 codecID:0x15002 channel:1 samplerate:16000 Param3:0 format:1 bitrate:64000];
        paecrecord = [[AECRecorder alloc] init];
        [paecrecord openAECRecord:1 samplerate:16000 bitspersample:16 record_cb:^(AudioBufferList *bufferList) {
                    AudioBuffer buffer = bufferList->mBuffers[0];
                    // ... AAC encoder and send
            if(pmedia_enc)
            {
                char buf[1024];
                int len = 1024;
                int64_t pts = 0;
                [pmedia_enc deliverData:(char*)buffer.mData rawdatalen:(int)buffer.mDataByteSize encdata:(char*)NULL encdatalen:(int)0 present_time:(long long)0];
                len = [pmedia_enc getEncData:buf datalen:1024 timeStamp:&pts];
                if(len > 0)
                {
                    if(NULL == paac_file)
                    {
                        NSString *homePath = NSHomeDirectory();
                        NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
                        NSString* aacpath = [NSString stringWithFormat:@"%@/aec_out.aac", documentPath];
                        paac_file = fopen([aacpath UTF8String], "wb");
                    }
                    
                    if(paac_file)
                    {
                        fwrite(buf, 1, len, paac_file);
                    }
                }
            }
                }];
        NSLog(@"[paecrecord openAECRecord]");
    }
}

-(void)stopAECRecord
{
    if(paecrecord)
    {
        NSLog(@"before [paecrecord closeAECRecord]");
        [paecrecord closeAECRecord];
        NSLog(@"[paecrecord closeAECRecord]");
    }
    
    if(paac_file)
    {
        fclose(paac_file);
        paac_file = NULL;
    }
}
- (void)startXBAec
{
    if(nil == pxbaec)
    {
        pxbaec = [[XBEchoCancellation alloc] initWithRate:16000 bit:16 channel:1];
        [pxbaec startInput];
        //[pxbaec startOutput];
        pxbaec.bl_input = ^(AudioBufferList *bufferList)
        {
            AudioBuffer buffer = bufferList->mBuffers[0];
            //NSData *pcmBlock = [NSData dataWithBytes:buffer.mData length:buffer.mDataByteSize];
            [pwav_mux deliverData:(char*)buffer.mData dataLen:buffer.mDataByteSize];
        };
    }
    
    if(nil == pwav_mux)
    {
        pwav_mux = [[IWavMuxer alloc] initWithSinkUrl:get_doc_path(@"aec_out.wav") channcel:1 samplerate:16000 bitspersample:16];
    }
}

- (void)stopXBAec
{
    [pxbaec stop];
    [pxbaec stopInput];
    //[pxbaec stopOutput];
    pxbaec = NULL;
    
    if(nil != pwav_mux)
    {
        [pwav_mux close];
    }
}

- (void)startEchoCancel
{
    ////startEchoCancel:(int)channel samplerate:(int)samplerate sampleformat:(int)format nbsamples:(int)nbsamples delay:(int)usdelay
    long begin = getSysTime();
    NSLog(@"startEchoCancel begin\n");
    //[self.player startEchoCancel:1 samplerate:16000 sampleformat:1 nbsamples:160 delay:375000];
    NSLog(@"startEchoCancel spend time:%ld ms\n", getSysTime() - begin);
    if(NULL == paudio_record)
    {
        NSLog(@"initRecord spend time:%ld ms\n", getSysTime() - begin);
        paudio_record = [[Recorder alloc] initRecord:_player];
        NSLog(@"initRecord spend time:%ld ms\n", getSysTime() - begin);
        [paudio_record start];
        NSLog(@"start Record spend time:%ld ms\n", getSysTime() - begin);
    }
}

- (void)stopEchoCancel
{
    NSLog(@"stopEchoCancel begin\n");
    if(paudio_record)
    {
        [paudio_record stop];
    }
    NSLog(@"stopEchoCancel 1\n");
    //[self.player stopEchoCancel];
    NSLog(@"stopEchoCancel end\n");
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [self installMovieNotificationObservers];

    [self.player prepareToPlay];
    //NSString
    //NSString *UrlString = [self.url isFileURL] ? [self.url path] : [self.url absoluteString];
    if([self IsPlayRecord])
    {
        deliver_thread = [[NSThread alloc]initWithTarget:self selector:@selector(packet_loop) object:nil];
        //为线程设置一个名称
        deliver_thread.name=@"deliver thread";
         //开启线程
        [deliver_thread start];
    }
}
-(void)stopthread
{
    [self.player stopRecord:TRUE];
}
- (void)viewDidDisappear:(BOOL)animated {
    bloop = false;
    if(deliver_thread && [deliver_thread isExecuting] && _condition)
    {
        [deliver_thread cancel];
        [_condition wait];
        //[self performSelector:@selector(stopthread) onThread:deliver_thread withObject:nil waitUntilDone:NO];
    }
    [super viewDidDisappear:animated];
    [self.player shutdown];
    [self removeMovieNotificationObservers];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation{
    return UIInterfaceOrientationIsLandscape(toInterfaceOrientation);
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscape;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark IBAction

- (IBAction)onClickMediaControl:(id)sender
{
    [self.mediaControl showAndFade];
}

- (IBAction)onClickOverlay:(id)sender
{
    [self.mediaControl hide];
}

- (IBAction)onClickDone:(UIBarButtonItem *)sender
{
    NSString *homePath = NSHomeDirectory();
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];

    formatter.dateFormat = @"yyyyMMdd-HHmmss";

    NSString *time = [formatter stringFromDate:[NSDate date]];
    //NSString* path = [NSString stringWithFormat:@"%@%@", documentPath, time];
    if(!brecord)
    {
        NSString* tmppath = [NSString stringWithFormat:@"%@/%@.tmp", documentPath, time];
        videoPath = [NSString stringWithFormat:@"%@/%@.mov", documentPath, time];
        
        [self.player startRecord:videoPath tmpUrl:tmppath];
        brecord = 1;
    }
    else
    {
        [self.player stopRecord:FALSE];
        brecord = 0;
        NSURL* videourl = [NSURL fileURLWithPath:videoPath];
        [[PHPhotoLibrary sharedPhotoLibrary]  saveVideoWithUrl:videourl ToAlbum:@"ijkvideo" completion:^(PHAsset *videoAsset) {
            NSLog(@"save jpg success"); 
            remove([videoPath UTF8String]);
            videoPath = nil;
        } failure:^(NSError *error) {
            NSLog(@"save jpg failed:%@", [error localizedDescription]);
            videoPath = nil;
        }];
        
        
    }
    sender.title = (brecord ? @"stop record" : @"record");
    /*NSString *homePath = NSHomeDirectory();
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSString* path = [NSString stringWithFormat:@"%@%@", documentPath, @"/test2.jpg"];
    [self.player saveImageJpg:path];
    sender.title = (player.shouldShowHudView ? @"HUD On" : @"HUD Off");*/
    //[self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
}

- (IBAction)onClickHUD:(UIBarButtonItem *)sender
{
    NSString *homePath = NSHomeDirectory();
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];

    formatter.dateFormat = @"yyyyMMdd-HHmmss";
    BOOL muted = self.player.playbackMuted;
    muted = !muted;
    sender.title = (muted ? @"Not Mute" : @"Mute");
    self.player.playbackMuted = muted;
    /*NSString *time = [formatter stringFromDate:[NSDate date]];
    if ([self.player isKindOfClass:[IJKFFMoviePlayerController class]]) {
        IJKFFMoviePlayerController *player = self.player;
        player.shouldShowHudView = !player.shouldShowHudView;
        
        sender.title = (player.shouldShowHudView ? @"Not Mute" : @"Mute");
    }*/
}

- (IBAction)onClickPlay:(id)sender
{
    [self.player play];
    [self.mediaControl refreshMediaControl];
}

- (IBAction)onClickPause:(id)sender
{
    [self.player pause];
    [self.mediaControl refreshMediaControl];
}

-(IBAction)onClickEchoCancel:(UIBarButtonItem *)sender
{
    if(!baec)
    {
        [self startpAECRecord];
        //[self startEchoCancel];
        sender.title = @"stop aec";
        baec = true;
    }
    else
    {
        [self stopAECRecord];
        //[self stopEchoCancel];
        sender.title = @"start aec";
        baec = false;
    }
}
- (IBAction)didSliderTouchDown
{
    [self.mediaControl beginDragMediaSlider];
}

- (IBAction)didSliderTouchCancel
{
    [self.mediaControl endDragMediaSlider];
}

- (IBAction)didSliderTouchUpOutside
{
    [self.mediaControl endDragMediaSlider];
}

- (IBAction)didSliderTouchUpInside
{
    self.player.currentPlaybackTime = self.mediaControl.mediaProgressSlider.value;
    [self.mediaControl endDragMediaSlider];
}

- (IBAction)didSliderValueChanged
{
    [self.mediaControl continueDragMediaSlider];
}

- (void)loadStateDidChange:(NSNotification*)notification
{
    //    MPMovieLoadStateUnknown        = 0,
    //    MPMovieLoadStatePlayable       = 1 << 0,
    //    MPMovieLoadStatePlaythroughOK  = 1 << 1, // Playback will be automatically started in this state when shouldAutoplay is YES
    //    MPMovieLoadStateStalled        = 1 << 2, // Playback will be automatically paused in this state, if started

    IJKMPMovieLoadState loadState = _player.loadState;

    if ((loadState & IJKMPMovieLoadStatePlaythroughOK) != 0) {
        NSLog(@"loadStateDidChange: IJKMPMovieLoadStatePlaythroughOK: %d\n", (int)loadState);
    } else if ((loadState & IJKMPMovieLoadStateStalled) != 0) {
        NSLog(@"loadStateDidChange: IJKMPMovieLoadStateStalled: %d\n", (int)loadState);
    } else {
        NSLog(@"loadStateDidChange: ???: %d\n", (int)loadState);
    }
}
- (void)MuxerStartNotification:(NSNotification*)notification
{
    NSLog(@"Muxer start");
}

- (void)MuxerProgressNotification:(NSNotification*)notification
{
    int pts = [[[notification userInfo] valueForKey:IJKMediaMuxerProgressNotificationPTS] intValue];
    
    NSLog(@"current muxer pts:%d", pts);
}

- (void)MuxerCompleteNotification:(NSNotification*)notification
{
    int duration = [[[notification userInfo] valueForKey:IJKMediaMuxerCompleteNotificationDuration] intValue];
    
    NSLog(@"Muxer Complete, duration:%d", duration);
}

- (void)MuxerInterruptNotification:(NSNotification*)notification
{
    int interrupt_reason = [[[notification userInfo] valueForKey:IJKMediaMuxerCompleteNotificationInterruptReason] intValue];
    NSLog(@"interrupt reason:%d", interrupt_reason);
}

- (void)MuxerErrorNotification:(NSNotification*)notification
{
    int reason = [[[notification userInfo] valueForKey:IJKMediaMuxerErrorNotificationReason] intValue];
    NSLog(@"muxer error reason:%d", reason);
}
- (void)moviePlayBackDidFinish:(NSNotification*)notification
{
    //    MPMovieFinishReasonPlaybackEnded,
    //    MPMovieFinishReasonPlaybackError,
    //    MPMovieFinishReasonUserExited
    int reason = [[[notification userInfo] valueForKey:IJKMPMoviePlayerPlaybackDidFinishReasonUserInfoKey] intValue];
    bcomplete = 1;
    switch (reason)
    {
        case IJKMPMovieFinishReasonPlaybackEnded:
            NSLog(@"playbackStateDidChange: IJKMPMovieFinishReasonPlaybackEnded: %d\n", reason);
            break;

        case IJKMPMovieFinishReasonUserExited:
            NSLog(@"playbackStateDidChange: IJKMPMovieFinishReasonUserExited: %d\n", reason);
            break;

        case IJKMPMovieFinishReasonPlaybackError:
            NSLog(@"playbackStateDidChange: IJKMPMovieFinishReasonPlaybackError: %d\n", reason);
            break;

        default:
            NSLog(@"playbackPlayBackDidFinish: ???: %d\n", reason);
            break;
    }
}

- (void)mediaIsPreparedToPlayDidChange:(NSNotification*)notification
{
    NSLog(@"mediaIsPreparedToPlayDidChange\n");
}

- (void)moviePlayBackStateDidChange:(NSNotification*)notification
{
    //    MPMoviePlaybackStateStopped,
    //    MPMoviePlaybackStatePlaying,
    //    MPMoviePlaybackStatePaused,
    //    MPMoviePlaybackStateInterrupted,
    //    MPMoviePlaybackStateSeekingForward,
    //    MPMoviePlaybackStateSeekingBackward

    switch (_player.playbackState)
    {
        case IJKMPMoviePlaybackStateStopped: {
            NSLog(@"IJKMPMoviePlayBackStateDidChange %d: stoped", (int)_player.playbackState);
            break;
        }
        case IJKMPMoviePlaybackStatePlaying: {
            NSLog(@"IJKMPMoviePlayBackStateDidChange %d: playing", (int)_player.playbackState);
            break;
        }
        case IJKMPMoviePlaybackStatePaused: {
            NSLog(@"IJKMPMoviePlayBackStateDidChange %d: paused", (int)_player.playbackState);
            break;
        }
        case IJKMPMoviePlaybackStateInterrupted: {
            NSLog(@"IJKMPMoviePlayBackStateDidChange %d: interrupted", (int)_player.playbackState);
            break;
        }
        case IJKMPMoviePlaybackStateSeekingForward:
        case IJKMPMoviePlaybackStateSeekingBackward: {
            NSLog(@"IJKMPMoviePlayBackStateDidChange %d: seeking", (int)_player.playbackState);
            break;
        }
        default: {
            NSLog(@"IJKMPMoviePlayBackStateDidChange %d: unknown", (int)_player.playbackState);
            break;
        }
    }
}

#pragma mark Install Movie Notifications

/* Register observers for the various movie object notifications. */
-(void)installMovieNotificationObservers
{
	[[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(loadStateDidChange:)
                                                 name:IJKMPMoviePlayerLoadStateDidChangeNotification
                                               object:_player];

	[[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(moviePlayBackDidFinish:)
                                                 name:IJKMPMoviePlayerPlaybackDidFinishNotification
                                               object:_player];

	[[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(mediaIsPreparedToPlayDidChange:)
                                                 name:IJKMPMediaPlaybackIsPreparedToPlayDidChangeNotification
                                               object:_player];

	[[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(moviePlayBackStateDidChange:)
                                                name:IJKMPMoviePlayerPlaybackStateDidChangeNotification
                                               object:_player];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
    selector:@selector(MuxerStartNotification:)
        name:IJKMediaMuxerStartNotification
      object:_player];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
    selector:@selector(MuxerProgressNotification:)
        name:IJKMediaMuxerProgressNotification
      object:_player];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
    selector:@selector(MuxerCompleteNotification:)
        name:IJKMediaMuxerCompleteNotification
      object:_player];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
    selector:@selector(MuxerErrorNotification:)
        name:IJKMediaMuxerErrorNotification
      object:_player];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
    selector:@selector(MuxerInterruptNotification:)
        name:IJKMediaMuxerInterruptNotification
      object:_player];
}

#pragma mark Remove Movie Notification Handlers

/* Remove the movie notification observers from the movie object. */
-(void)removeMovieNotificationObservers
{
    [[NSNotificationCenter defaultCenter]removeObserver:self name:IJKMPMoviePlayerLoadStateDidChangeNotification object:_player];
    [[NSNotificationCenter defaultCenter]removeObserver:self name:IJKMPMoviePlayerPlaybackDidFinishNotification object:_player];
    [[NSNotificationCenter defaultCenter]removeObserver:self name:IJKMPMediaPlaybackIsPreparedToPlayDidChangeNotification object:_player];
    [[NSNotificationCenter defaultCenter]removeObserver:self name:IJKMPMoviePlayerPlaybackStateDidChangeNotification object:_player];
    [[NSNotificationCenter defaultCenter]removeObserver:self name:IJKMediaMuxerInterruptNotification object:_player];
}

@end

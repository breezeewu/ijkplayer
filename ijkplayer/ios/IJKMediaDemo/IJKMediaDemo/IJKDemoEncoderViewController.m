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

#import "IJKDemoEncoderViewController.h"

#import "IJKCommon.h"
#import "IJKMoviePlayerViewController.h"

/*@interface IJKDemoEncoderViewController () <UITableViewDataSource, UITableViewDelegate>

@property(nonatomic,strong) IBOutlet UITableView *tableView;
@property(nonatomic,strong) NSArray *sampleList;

@end

@implementation IJKDemoEncoderViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    NSString *homePath = NSHomeDirectory();
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSString* path = [NSString stringWithFormat:@"%@%@", documentPath, @"/test.pcm"];
    self.title = @"pcm data";
    NSMutableArray *sampleList = [[NSMutableArray alloc] init];
    [sampleList addObject:@[@"pcm data", path]];
    self.title = @"M3U8";

    NSMutableArray *sampleList = [[NSMutableArray alloc] init];

    [sampleList addObject:@[@"bipbop basic master playlist",
                            @"http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/bipbop_4x3_variant.m3u8"]];
    [sampleList addObject:@[@"bipbop basic 400x300 @ 232 kbps",
                            @"http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear1/prog_index.m3u8"]];
    [sampleList addObject:@[@"bipbop basic 640x480 @ 650 kbps",
                            @"http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear2/prog_index.m3u8"]];
    [sampleList addObject:@[@"bipbop basic 640x480 @ 1 Mbps",
                            @"http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear3/prog_index.m3u8"]];
    [sampleList addObject:@[@"bipbop basic 960x720 @ 2 Mbps",
                            @"http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear4/prog_index.m3u8"]];
    [sampleList addObject:@[@"bipbop basic 22.050Hz stereo @ 40 kbps",
                            @"http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear0/prog_index.m3u8"]];

    [sampleList addObject:@[@"bipbop advanced master playlist",
                            @"http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/bipbop_16x9_variant.m3u8"]];
    [sampleList addObject:@[@"bipbop advanced 416x234 @ 265 kbps",
                            @"http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/gear1/prog_index.m3u8"]];
    [sampleList addObject:@[@"bipbop advanced 640x360 @ 580 kbps",
                            @"http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/gear2/prog_index.m3u8"]];
    [sampleList addObject:@[@"bipbop advanced 960x540 @ 910 kbps",
                            @"http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/gear3/prog_index.m3u8"]];
    [sampleList addObject:@[@"bipbop advanced 1280x720 @ 1 Mbps",
                            @"http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/gear4/prog_index.m3u8"]];
    [sampleList addObject:@[@"bipbop advanced 1920x1080 @ 2 Mbps",
                            @"http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/gear5/prog_index.m3u8"]];
    [sampleList addObject:@[@"bipbop advanced 22.050Hz stereo @ 40 kbps",
                            @"http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/gear0/prog_index.m3u8"]];
     

    self.sampleList = sampleList;
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];

    [self.tableView reloadData];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return @"Samples";
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (IOS_NEWER_OR_EQUAL_TO_7) {
        return self.sampleList.count;
    } else {
        return self.sampleList.count - 1;
    }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"abc"];
    if (nil == cell) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"abc"];
        cell.textLabel.lineBreakMode = NSLineBreakByTruncatingMiddle;
    }

    cell.textLabel.text = self.sampleList[indexPath.row][0];

    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];

    NSArray *item = self.sampleList[indexPath.row];
    NSURL   *url  = [NSURL URLWithString:item[1]];

    [self.navigationController presentViewController:[[IJKVideoViewController alloc] initWithURL:url] animated:YES completion:^{}];
}
*/

@interface IJKDemoEncoderViewController ()

@end

@implementation IJKDemoEncoderViewController {
    NSString *_folderPath;
    NSMutableArray *_subpaths;
    NSMutableArray *_files;
    NSThread* deliver_thread;
    IJKMediaEncoder*    aac_enc;
    IJKMediaEncoder*    mediaenc;
    NSString* path;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        /*folderPath = [folderPath stringByStandardizingPath];
        self.title = [folderPath lastPathComponent];
        
        _folderPath = folderPath;*/
        _subpaths = [NSMutableArray array];
        _files = [NSMutableArray array];
    }
    deliver_thread = NULL;
    aac_enc = NULL;
    path = NULL;
    return self;
}
-(void) aec_loop
{
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSString* near_pcm_path = [NSString stringWithFormat:@"%@/%@.pcm", documentPath, @"micin"];
    NSString* far_pcm_path = [NSString stringWithFormat:@"%@/%@.pcm", documentPath, @"speaker"];
    NSString* out_pcm_path = [NSString stringWithFormat:@"%@/%@.pcm", documentPath, @"out"];
    NSString* logpath = [NSString stringWithFormat:@"%@/log/", documentPath];
    
    FILE* pnear_file = fopen([near_pcm_path UTF8String], "rb");
    FILE* pfar_file = fopen([far_pcm_path UTF8String], "rb");
    FILE* poutfile = fopen([out_pcm_path UTF8String], "wb");
    //channel samplerate:(int)samplerate samplebytes:(int)samplebytes nbsamples:(int)nbsamples
    IJKAudioProc* pap = [[IJKAudioProc alloc] init:1 samplerate:8000 sampleformat:1 nbsamples:160];
    //add_aec_filter:(int)far_channel far_samplerate:(int)far_samplerate far_sample_bytes:(int)far_sample_bytes msdelay:(int)msdelay
    [pap initlog:[logpath UTF8String] loglevel:3 logmode:3];
    [pap add_aec_filter:40000];
    /*fseek(pnear_file, 0, SEEK_END);
    fseek(pfar_file, 0, SEEK_END);
    int near_size = ftell(pnear_file);
    int far_size = ftell(pfar_file);*/
    const int smbbytes = 540;
    char near_buffer[smbbytes];
    char far_buffer[smbbytes];
    char out_buffer[1024];
    while(1)
    {
        long near_len = fread(near_buffer, 1, smbbytes, pnear_file);
        long far_len = fread(far_buffer, 1, smbbytes, pfar_file);
        if(near_len <= 0 || far_len <= 0)
        {
            NSLog(@"near_len:%d <= 0 || far_len:%d <= 0", near_len, far_len);
            break;
        }

        int64_t ts = [IJKAudioProc get_timestamp_in_us];
        //[pap audio_Aec_Proc:far_buffer nearbuffer:near_buffer output:out_buffer length:smbbytes];
        [pap deliveFarData:far_buffer datalen:far_len timestamp:ts];
        //ts = [IJKAudioProc get_timestamp_in_us];
        //- (void)deliverData:(char*) pdata  outbuffer:poutbuf data_size:(int)size timestamp:(long long)timestamp_in_us
        int len = [pap deliverData:near_buffer outbuffer:out_buffer data_size:far_len timestamp:ts];
        fwrite(out_buffer, 1, len, poutfile);
    }
    
    if(poutfile)
    {
        fclose(poutfile);
        NSLog(@"out.pcm");
    }
    
    if(pnear_file)
    {
        fclose(pnear_file);
    }
    
    if(pfar_file)
    {
        fclose(pfar_file);
    }
    
    [pap close];
}

-(void) packet_loop
{
    NSString* path = self->path;//[self.url isFileURL] ? [self.url path] : [self.url absoluteString];
    int mediatype = 1;
    int codecid = 0x15002;
    int channel = 1;
    int samplerate = 16000;
    int foramt = 1;
    int bitrate = 128;
    FILE* paacfile = NULL;
    if([path containsString:@"8k"] || [path containsString:@"8K"])
    {
        samplerate = 8000;
    }
    NSString *homePath = NSHomeDirectory();
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSString* aacpath = [NSString stringWithFormat:@"%@%@%d%@", documentPath, @"/test_", samplerate/1000, @"k.aac"];
    mediaenc = [[IJKMediaEncoder alloc] init:mediatype codecID:codecid Param1:channel Param2:samplerate Param3:0 format:foramt bitrate:bitrate];
    paacfile = fopen([aacpath UTF8String], "wb");
    if(path)
    {
        //[self.player openRecord:path];
        FILE* pfile = fopen([path UTF8String], "rb");
        int readlen = 2048;
        Byte buf[2048];
        Byte pcmbuf[2048];
        while(pfile)
        {
            int mt = -1;
            long pts = -1;
            int pcmlen = fread(pcmbuf, 1, readlen, pfile);
            if(pcmlen <= 0)
            {
                break;
            }
            //deliverData:rawdata:(char*) rawdata rawdatalen:(int)rawdatalen encdata:(char*)encdata encdatalen:(int)encdatalen present_time:(long)pts;
            int enclen = [mediaenc deliverData:pcmbuf rawdatalen:pcmlen encdata:nil encdatalen:0 present_time:0];
            if(enclen > 0)
            {
                //-(int)getEncData:(char*)pdata, datalen:(int*)datalen, timeStamp:(long*) ppts
                [mediaenc getEncData:buf datalen:&enclen timeStamp:NULL];
            }
            if(paacfile && enclen > 0)
            {
                fwrite(buf, 1, enclen, paacfile);
            }
            //[self.player deliverData:mt mediadata:buf data_size:pktlen present_time:pts];
            //usleep(30000);
        }
        
        //[mediaenc deinit];
        if(paacfile)
        {
            fclose(paacfile);
            paacfile = NULL;
        }
        UIAlertController *alertVC = [UIAlertController alertControllerWithTitle:@"提示" message:@"编码完成！" preferredStyle:UIAlertControllerStyleAlert];
        UIAlertAction * cancelAc = [UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                    //点击取消要执行的代码
                }];
                UIAlertAction *comfirmAc = [UIAlertAction actionWithTitle:@"确定" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                    //点击确定要执行的代码
                }];
                [alertVC addAction:cancelAc];
                [alertVC addAction:comfirmAc];
                [self presentViewController:alertVC animated:YES completion:nil];
        //[self.player closeRecord];
    }
}

-(void) pcm_proc_loop
{
    NSString* path = self->path;//[self.url isFileURL] ? [self.url path] : [self.url absoluteString];
    int mediatype = 1;
    int codecid = 0x15002;
    int channel = 1;
    int samplerate = 16000;
    int foramt = 1;
    int bitrate = 128;
    FILE* poutfile = NULL;
    if([path containsString:@"8k"] || [path containsString:@"8K"])
    {
        samplerate = 8000;
    }
    NSString *homePath = NSHomeDirectory();
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSString* outpath = [NSString stringWithFormat:@"%@%@%d%@", documentPath, @"/pc_out_", samplerate/1000, @"k.pcm"];
    mediaenc = [[IJKMediaEncoder alloc] init:mediatype codecID:codecid Param1:channel Param2:samplerate Param3:0 format:foramt bitrate:bitrate];
    IJKAudioProc* apc = [[IJKAudioProc alloc] init:1 samplerate:16000 sampleformat:1 nbsamples:320];
    [apc add_ns_filter:1];
    //[apc add_agc_filter:3];
    poutfile = fopen([outpath UTF8String], "wb");
    if(path)
    {
        //[self.player openRecord:path];
        FILE* pfile = fopen([path UTF8String], "rb");
        int readlen = 640;
        Byte buf[2048];
        Byte pcmbuf[2048];
        while(pfile)
        {
            int mt = -1;
            long pts = -1;
            int pcmlen = fread(pcmbuf, 1, readlen, pfile);
            if(pcmlen <= 0)
            {
                break;
            }
            int outlen = [apc audio_process:NULL near:pcmbuf outbuf:buf size:pcmlen];
            //int outlen = [apc audio_process:NULL near:pcmbuf outbuffer:buf size:pcmlen];
            //deliverData:rawdata:(char*) rawdata rawdatalen:(int)rawdatalen encdata:(char*)encdata encdatalen:(int)encdatalen present_time:(long)pts;
            //int enclen = [mediaenc deliverData:pcmbuf rawdatalen:pcmlen encdata:nil encdatalen:0 present_time:0];
            /*if(outlen > 0)
            {
                //-(int)getEncData:(char*)pdata, datalen:(int*)datalen, timeStamp:(long*) ppts
                [mediaenc getEncData:buf datalen:&enclen timeStamp:NULL];
            }*/
            if(poutfile && outlen > 0)
            {
                fwrite(buf, 1, outlen, poutfile);
            }
            //[self.player deliverData:mt mediadata:buf data_size:pktlen present_time:pts];
            //usleep(30000);
        }
        
        //[mediaenc deinit];
        if(poutfile)
        {
            fclose(poutfile);
            poutfile = NULL;
        }
        UIAlertController *alertVC = [UIAlertController alertControllerWithTitle:@"提示" message:@"编码完成！" preferredStyle:UIAlertControllerStyleAlert];
        UIAlertAction * cancelAc = [UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                    //点击取消要执行的代码
                }];
                UIAlertAction *comfirmAc = [UIAlertAction actionWithTitle:@"确定" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                    //点击确定要执行的代码
                }];
                [alertVC addAction:cancelAc];
                [alertVC addAction:comfirmAc];
                [self presentViewController:alertVC animated:YES completion:nil];
        //[self.player closeRecord];
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    NSError *error = nil;
    BOOL isDirectory = NO;
    _folderPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSArray *files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:_folderPath error:&error];
    NSString *homePath = NSHomeDirectory();
    
    //NSString* path = [NSString stringWithFormat:@"%@%@", documentPath, @"/test.pcm"];
    [_subpaths addObject:@".."];
    //[_files addObject:path];
    //NSLog(path);
    for (NSString *fileName in files) {
        NSString *fullFileName = [_folderPath stringByAppendingPathComponent:fileName];
        if(fullFileName && [fullFileName containsString:@".pcm"])
        {
            [_files addObject:fileName];
        }
        /*[[NSFileManager defaultManager] fileExistsAtPath:fullFileName isDirectory:&isDirectory];
        if (isDirectory) {
            [_subpaths addObject:fileName];
        } else {
            [_files addObject:fileName];
        }*/
    }
    /*[_subpaths addObject:path];

    /*for (NSString *fileName in files) {
        NSString *fullFileName = [_folderPath stringByAppendingPathComponent:fileName];
        if(fullFileName && [fullFileName containsString:@".data"])
        {
            [_files addObject:fileName];
        }
    }*/
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 2;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    switch (section) {
        case 0:
            return _subpaths.count;
            
        case 1:
            return _files.count;
            
        default:
            break;
    }

    return 0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"abc"];
    if (nil == cell) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"abc"];
        cell.textLabel.lineBreakMode = NSLineBreakByTruncatingMiddle;
    }
    
    switch (indexPath.section) {
        case 0: {
            cell.textLabel.text = [NSString stringWithFormat:@"[%@]", _subpaths[indexPath.row]];
            cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        } break;
        case 1: {
            cell.textLabel.text = _files[indexPath.row];
            cell.accessoryType = UITableViewCellAccessoryNone;
        } break;
        default:
            break;
    }
    
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    
    switch (indexPath.section) {
        case 0: {
            NSString *fileName = [_folderPath stringByAppendingPathComponent:_files[indexPath.row]];
            NSLog(@"index:0 %@\n", fileName);
            deliver_thread = [[NSThread alloc]initWithTarget:self selector:@selector(pcm_proc_loop) object:fileName];
            //为线程设置一个名称
            deliver_thread.name=@"deliver thread";
             //开启线程
            [deliver_thread start];
            /*IJKDemoEncoderViewController *viewController = [[IJKDemoEncoderViewController alloc] initWithFolderPath:fileName];
            
            [self.navigationController pushViewController:viewController animated:YES];*/
        } break;
        case 1: {
            NSLog(@"index:1 %@", _files[indexPath.row]);
            //NSString *fileName = _files[indexPath.row];
            //self->path = _files[indexPath.row];
            NSString *fileName = [_folderPath stringByAppendingPathComponent:_files[indexPath.row]];
            self->path = fileName;
            //NSLog(@"index:1 %@\n", fileName);
            //fileName = [fileName stringByStandardizingPath];
            deliver_thread = [[NSThread alloc]initWithTarget:self selector:@selector(pcm_proc_loop) object:nil];
            //为线程设置一个名称
            deliver_thread.name=@"deliver thread";
             //开启线程
            [deliver_thread start];
            /*[IJKVideoViewController presentFromViewController:self withTitle:[NSString stringWithFormat:@"File: %@", fileName] URL:[NSURL fileURLWithPath:fileName] completion:^{
            }];*/
            
        } break;
        default:
            break;
    }
}

/*
// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the specified item to be editable.
    return YES;
}
*/

/*
// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }
}
*/

/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end

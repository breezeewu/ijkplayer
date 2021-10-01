/*
 * Copyright (C) 2015 Gdier
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

#import "IJKDemoInputURLViewController.h"
#import "IJKMoviePlayerViewController.h"

@interface IJKDemoInputURLViewController () <UITextViewDelegate>

@property(nonatomic,strong) IBOutlet UITextView *textView;

@end

@implementation IJKDemoInputURLViewController

- (instancetype)init {
    self = [super init];
    if (self) {
        self.title = @"Input URL";
        
        [self.navigationItem setRightBarButtonItem:[[UIBarButtonItem alloc] initWithTitle:@"Play" style:UIBarButtonItemStyleDone target:self action:@selector(onClickPlayButton)]];
    }
    
    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    NSString *homePath = NSHomeDirectory();
       NSLog(@"home根目录:%@", homePath);
    //2.获取Documents路径
       /*参数一：指定要搜索的文件夹枚举值
         参数二：指定搜索的域Domian: NSUserDomainMask
         参数三：是否需要绝对/全路径：NO：波浪线~标识数据容器的根目录; YES(一般制定): 全路径
        */
        NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
       NSLog(@"documens路径:%@", documentPath);
    NSString* path = [NSString stringWithFormat:@"%@%@", documentPath, @"/test2.mov"];
    self.textView.text = @"https://pd-vava-s3-test.s3-us-west-2.amazonaws.com/live/P020101000201190813000050/20190923-124534.m3u8";
    //self.textView.text =@"https://pd-vava-s3-test.s3-us-west-2.amazonaws.com/live/P020101000201190813000050/20190923-124534-0.mp4";//path;//
    //self.textView.text =@"https://pd-vava-s3-test.s3-us-west-2.amazonaws.com/live/P020101000407010000000496/20190911-180834-0.ts";
}

- (void)onClickPlayButton {
    
    NSURL *url = [NSURL URLWithString:self.textView.text];
    if(nil == self.textView.text || [self.textView.text length] == 0)
    {
    UIAlertController *alertVC = [UIAlertController alertControllerWithTitle:@"提示" message:@"URL不能为空" preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction * cancelAc = [UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                //点击取消要执行的代码
            }];
            UIAlertAction *comfirmAc = [UIAlertAction actionWithTitle:@"确定" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                //点击确定要执行的代码
            }];
            [alertVC addAction:cancelAc];
            [alertVC addAction:comfirmAc];
            [self presentViewController:alertVC animated:YES completion:nil];
    }
    NSString *scheme = [[url scheme] lowercaseString];
    [IJKVideoViewController presentFromViewController:self withTitle:[NSString stringWithFormat:@"URL: %@", url] URL:url completion:^{
    //            [self.navigationController popViewControllerAnimated:NO];
            }];
    /*if ([scheme isEqualToString:@"http"]
        || [scheme isEqualToString:@"https"]
        || [scheme isEqualToString:@"rtmp"]) {
        [IJKVideoViewController presentFromViewController:self withTitle:[NSString stringWithFormat:@"URL: %@", url] URL:url completion:^{
//            [self.navigationController popViewControllerAnimated:NO];
        }];
    }*/
}

- (void)textViewDidEndEditing:(UITextView *)textView {
    //[self onClickPlayButton];
}

@end

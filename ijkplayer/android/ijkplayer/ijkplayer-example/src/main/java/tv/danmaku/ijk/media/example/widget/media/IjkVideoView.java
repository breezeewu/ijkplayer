/*
 * Copyright (C) 2015 Bilibili
 * Copyright (C) 2015 Zhang Rui <bbcallen@gmail.com>
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

package tv.danmaku.ijk.media.example.widget.media;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaRecorder;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.support.v7.app.AlertDialog;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.MediaController;
import android.widget.TableLayout;
import android.widget.TextView;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import tv.danmaku.ijk.media.exo.IjkExoMediaPlayer;
import tv.danmaku.ijk.media.player.AndroidMediaPlayer;
import tv.danmaku.ijk.media.player.IMediaPlayer;
import tv.danmaku.ijk.media.player.IjkMediaPlayer;
import tv.danmaku.ijk.media.player.IjkTimedText;
import tv.danmaku.ijk.media.player.TextureMediaPlayer;
import tv.danmaku.ijk.media.player.misc.IMediaDataSource;
import tv.danmaku.ijk.media.player.misc.IMediaFormat;
import tv.danmaku.ijk.media.player.misc.ITrackInfo;
import tv.danmaku.ijk.media.player.misc.IjkMediaFormat;
import tv.danmaku.ijk.media.example.R;
import tv.danmaku.ijk.media.example.application.Settings;
import tv.danmaku.ijk.media.example.services.MediaPlayerService;

import android.media.AudioRecord;
import tv.danmaku.ijk.media.player.TalkHelp;
import tv.danmaku.ijk.media.recorder.IRecordDemux;
import tv.danmaku.ijk.media.recorder.VavaRecorder;
import tv.danmaku.ijk.media.recorder.VavaRecordDemux;

public class IjkVideoView extends FrameLayout implements MediaController.MediaPlayerControl{
    private String TAG = "IjkVideoView";
    // settable by the client
    private Uri mUri;
    private Uri msrcUri = null;
    private boolean balreadyplay = false;
    private Map<String, String> mHeaders;


    // all possible internal states
    private static final int STATE_ERROR = -1;
    private static final int STATE_IDLE = 0;
    private static final int STATE_PREPARING = 1;
    private static final int STATE_PREPARED = 2;
    private static final int STATE_PLAYING = 3;
    private static final int STATE_PAUSED = 4;
    private static final int STATE_PLAYBACK_COMPLETED = 5;

    // mCurrentState is a VideoView object's current state.
    // mTargetState is the state that a method caller intends to reach.
    // For instance, regardless the VideoView object's current state,
    // calling pause() intends to bring the object to a target state
    // of STATE_PAUSED.
    private int mCurrentState = STATE_IDLE;
    private int mTargetState = STATE_IDLE;

    // All the stuff we need for playing and showing a video
    private IRenderView.ISurfaceHolder mSurfaceHolder = null;
    private IMediaPlayer mMediaPlayer = null;
    // private int         mAudioSession;
    private int mVideoWidth;
    private int mVideoHeight;
    private int mSurfaceWidth;
    private int mSurfaceHeight;
    private int mVideoRotationDegree;
    private IMediaController mMediaController;
    private IMediaPlayer.OnCompletionListener mOnCompletionListener;
    private IMediaPlayer.OnPreparedListener mOnPreparedListener;
    private int mCurrentBufferPercentage;
    private IMediaPlayer.OnErrorListener mOnErrorListener;
    private IMediaPlayer.OnInfoListener mOnInfoListener;
    private int mSeekWhenPrepared;  // recording the seek position while preparing
    private boolean mCanPause = true;
    private boolean mCanSeekBack = true;
    private boolean mCanSeekForward = true;

    /** Subtitle rendering widget overlaid on top of the video. */
    // private RenderingWidget mSubtitleWidget;

    /**
     * Listener for changes to subtitle data, used to redraw when needed.
     */
    // private RenderingWidget.OnChangedListener mSubtitlesChangedListener;

    private Context mAppContext;
    private Settings mSettings;
    private IRenderView mRenderView;
    private int mVideoSarNum;
    private int mVideoSarDen;

    private InfoHudViewHolder mHudViewHolder;

    private long mPrepareStartTime = 0;
    private long mPrepareEndTime = 0;

    private long mSeekStartTime = 0;
    private long mSeekEndTime = 0;
    private boolean mbRun = false;

    private TextView subtitleDisplay;
    private boolean mbRecord = false;
    private int bmute = 0;
    private long pcmencid = 0;
    private Handler handler = new Handler();
    public interface OnAudioRecordCallback{
        void onAudioRecordData(byte[] data, int len);
    }
    public class PCMEncoder extends Thread
    {
        String pcmUrl;
        String dstaacurl;
        boolean mbRun = false;
        Button pcmencbnt;
        datadeliver data_deliver = null;
        private AudioRecord mAudioRecord = null;
        public PCMEncoder(Button bnt, String pcmpath, String aacpath)
        {
            pcmUrl = pcmpath;
            pcmencbnt = bnt;
            dstaacurl = aacpath;
            //data_deliver = ideliver;
        }


        public void run()
        {
            boolean isfirst = true;
            DataInputStream pcmreader = null;
            DataOutputStream aacwriter = null;
            DataInputStream pcmreader2 = null;
            DataOutputStream ag711writer = null;
            DataOutputStream decwriter = null;
            g711codec g711enc = new g711codec();

            mbRun = true;
            long encid = 0;
            while (mbRun) {
                try {
                    if (isfirst) {

                        pcmreader = new DataInputStream(new FileInputStream(pcmUrl));
                        pcmreader2 = new DataInputStream(new FileInputStream("/storage/emulated/0/tmp/test1.pcm"));
                        int samplerate = 16000;
                        if(pcmUrl.contains("8k") || pcmUrl.contains("8K"))
                        {
                            samplerate = 8000;
                        }
                        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyyMMdd-HHmmss");
                        long curtime = System.currentTimeMillis();
                        Date date = new Date(curtime);
                        String aacpath = dstaacurl + dateFormat.format(date)+"_" + samplerate/1000 + "k.aac";
                        encid = IjkMediaPlayer.nativeEncoderOpen(1, 0x15002, 1, samplerate, 1,128);//mMediaPlayer.EncoderOpen(1, 0x15002, 1, 16000, 1,128);
                        System.out.print(encid+" = IjkMediaPlayer.nativeEncoderOpen");
                        String g711path = dstaacurl + dateFormat.format(date)+"_" + samplerate/1000 + "k.g711";
                        FileOutputStream aacfile = new FileOutputStream(aacpath);
                        aacwriter = new DataOutputStream(aacfile);
                        FileOutputStream g711file = new FileOutputStream(g711path);
                        ag711writer = new DataOutputStream(g711file);
                        String decpath = dstaacurl + dateFormat.format(date)+"_" + samplerate/1000 + "k_dec.pcm";
                        FileOutputStream decfile = new FileOutputStream(decpath);
                        decwriter = new DataOutputStream(decfile);
                        //muxid = IjkMediaPlayer.nativeMuxerOpen("/storage/emulated/0/tmp/muxer.mov", "/storage/emulated/0/tmp/muxer.tmp");
                        if(null == aacwriter)
                        {

                        }
                        isfirst = false;
                    }
                    //System.out.print(encid+" = IjkMediaPlayer.nativeEncoderOpen");
                    byte pcmdata[] = new byte[1024];
                    if(0 != encid) {

                        System.out.print("before pcm sread");
                        int readed = pcmreader.read(pcmdata);
                        if(readed <= 0)
                        {
                            break;
                        }
                        System.out.print("after pcm sread");
                        byte[] aacdata = IjkMediaPlayer.nativeEncoderDeliverData(encid, pcmdata, 0);//mMediaPlayer.EncoderDeliverData(encid, pcmdata,0);
                        System.out.print("after pcm nativeEncoderDeliverData");
                        if (null != aacdata) {
                            aacwriter.write(aacdata);
                        }
                    }

                    int readlen = pcmreader2.read(pcmdata);
                    if(readlen <= 0)
                    {
                        break;
                    }
                    byte enc[] = g711enc.encode(pcmdata);
                    ag711writer.write(enc);
                    byte dec[] = g711enc.decode(enc);
                    decwriter.write(dec);
                    /*try {
                        Thread.sleep(30);
                    }
                    catch ( InterruptedException e)
                    {
                        System.out.print(e.toString());
                    }*/
                } catch (IOException e) {
                    System.out.print(e.toString());
                    break;
                }
            }
            try {
                if(null != aacwriter) {
                    aacwriter.close();
                }
                if(null != ag711writer)
                {
                    ag711writer.close();
                }
                if(null != decwriter)
                {
                    decwriter.close();
                }
            }
            catch(IOException e)
            {
                System.out.print(e.toString());
            }
            if(0 != encid) {
                IjkMediaPlayer.nativeEncoderClose(encid);
            }
            //pcmencbnt.setText("Start Encoder");
            //Toast.makeText(getApplicationContext(),"提示内容",Toast.LENGTH_SHORT).show();
            System.out.print("pcm encoder complete");
            //data_deliver.showtips("pcm encoder complete");
            /*AlertDialog.Builder builder=new AlertDialog.Builder(null);
            builder.setTitle("pcm encoder success");
            builder.setMessage("您的可用预存款为："+100+"元");
            builder.setPositiveButton("确定",
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialogInterface, int i) {

                        }
                    });
            AlertDialog dialog=builder.create();
            dialog.show();*/
        //}
        }

        public void stopEncoder()
        {
            mbRun = false;
            try{
                super.join();
            }
            catch(InterruptedException e)
            {
                e.printStackTrace();
                System.out.print(e.toString());
            }

        }

    }

    public class EchoCancelRecorder implements OnAudioRecordCallback
    {
        private AudioRecord mAudioRecord = null;
        int m_nchannel = AudioFormat.CHANNEL_IN_MONO;
        int m_nsamplerate = 16000;
        int m_nsampbit = 2;
        int m_nmin_buf_size = 0;
        IMediaPlayer m_player = null;
        public EchoCancelRecorder(IMediaPlayer player) {
            m_player = player;
            m_nmin_buf_size = AudioRecord.getMinBufferSize(m_nsamplerate, m_nchannel, m_nsampbit);
            mAudioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, m_nsamplerate, m_nchannel, m_nsampbit, m_nmin_buf_size);
        }
        public void start()
        {
            if(null != m_player)
            {
                m_player.StartEchoCancel(m_nchannel, m_nsamplerate, 1, 80, 430000);
            }
            if(null != mAudioRecord)
            {
                mAudioRecord.startRecording();
            }
        }

        public void stop()
        {
            if(null != m_player)
            {
                m_player.stop();
            }
            if(null != mAudioRecord)
            {
                mAudioRecord.stop();
            }
        }
        public void onAudioRecordData(byte[] data, int len)
        {
            if(null != m_player)
            {
                byte[] data2 = m_player.EchoCancelDeliverData(data);
            }
        }
    }
    public class RecordDemuxer extends Thread
    {
        String sourceUrl;
        datadeliver ideliver;
        private IMediaPlayer mMediaPlayer = null;
        private EchoCancelRecorder ecrecord = null;
        private TalkHelp th = null;
        private VavaRecorder record = null;
        public RecordDemuxer(IMediaPlayer MediaPlayer, String srcurl, datadeliver ideliver)
        {
            sourceUrl = srcurl;
            mMediaPlayer = MediaPlayer;
            this.ideliver = ideliver;
            Log.d(TAG, "RecordDemuxer() begin\n");
            //ecrecord = new EchoCancelRecorder(mMediaPlayer);
            th = new TalkHelp(MediaPlayer);

        }

        public void startEchoCancel(int delay)
        {
            if(th != null) {
                th.stopRecord();
                th.startRecord(0, delay);
            }
        }

        public void stopEchoCancel()
        {
            th.stopRecord();
        }

        public class recheader
        {
            public int tag; // oxEB0000AA
            public int size;
            public int type;
            public int fps;
            public int time_sec;
            public int time_usec;
        };

        public int read_int(DataInputStream din)
        {
            if(null != din)
            {
                try {
                    int value = 0;
                    byte bval[] = new byte[4];
                    bval[3] = din.readByte();
                    bval[2] = din.readByte();
                    bval[1] = din.readByte();
                    bval[0] = din.readByte();
                    for(int i = 0; i < 4; i++)
                    {
                        value <<= 8;
                        value |= bval[i]&0xff;
                    }

                    return value;
                }
                catch(IOException e)
                {
                    System.out.print(e.toString());
                }
            }
            return -1;
        }

        public void run() {
            boolean isfirst = true;
            byte v_enc = 0;
            DataInputStream din = null;//new DataInputStream(new FileInputStream(sourceUrl));
            //FileReader recreader = new FileReader(sourceUrl)；
            /*if(Looper.myLooper() == Looper.getMainLooper())
            {
                System.out.print("main thread");
            }*/
            VavaRecordDemux recdemux = new VavaRecordDemux();
            System.out.print("demuxer run begin\n");
            DataInputStream pcmreader = null;
            DataOutputStream aacwriter = null;
            record = new VavaRecorder();
            int ret = record.open(/*"https://pd-vava-s3-test.s3-us-west-2.amazonaws.com/live/P020101000201190813000050/20190923-124534.m3u8"*/null, "/storage/emulated/0/tmp/out.mov", "/storage/emulated/0/tmp/tmp.mov");
            record.start(0);
            mbRun = true;
            int first = 1;
            long encid = 0;
            long muxid = 0;
            long vframenum = 0;
            long aframenum = 0;
            long framenum = 0;
            //IjkMediaPlayer.nativeMuxerOpen("/storage/emulated/0/tmp/muxer.mp4", "/storage/emulated/0/tmp/tmp.mp4");
            while (mbRun) {
                try {
                    if (isfirst) {
                        recdemux.open(sourceUrl);
                        /*din = new DataInputStream(new FileInputStream(sourceUrl));
                        //Byte buf[] = new Byte[16];
                        //byte[] buf = new byte[16];
                        //din.read(buf);
                        byte tag = din.readByte();
                        v_enc = din.readByte();
                        byte a_enc = din.readByte();
                        byte res = din.readByte();
                        byte fps = din.readByte();
                        byte enctype = din.readByte();
                        int vframe = din.readShort();
                        int size = read_int(din);//din.//din.readInt();
                        int time = read_int(din);//din.readInt();
                        System.out.println("tag:"+tag+"v_enc:"+v_enc+"a_enc:"+a_enc+"res:"+res+"fps:"+fps+"enctype:"+enctype+"vframe:"+vframe+"size:"+size+"time:"+time);
                        //pcmreader = new DataInputStream(new FileInputStream("/storage/emulated/0/tmp/test.pcm"));
                        //encid = IjkMediaPlayer.nativeEncoderOpen(1, 0x15002, 1, 16000, 1,128);//mMediaPlayer.EncoderOpen(1, 0x15002, 1, 16000, 1,128);
                        //FileOutputStream aacfile = new FileOutputStream("/storage/emulated/0/tmp/test.aac");
                        //aacwriter = new DataOutputStream(aacfile);
                        //muxid = IjkMediaPlayer.nativeMuxerOpen("/storage/emulated/0/tmp/muxer.mov", "/storage/emulated/0/tmp/muxer.tmp");*/
                        isfirst = false;
                    }
                    /*int tag = read_int(din);//din.readInt();
                    int size = read_int(din);//din.readInt();
                    int type = read_int(din);//din.readInt();
                    int fps = read_int(din);//din.readInt();
                    int time_sec = read_int(din);//din.readInt();
                    int time_usec = read_int(din);//din.readInt();
                    long pts = ((long)time_sec * 1000 + (long)time_usec);

                    //System.out.println("tag"+tag+"type:"+type+ "fps"+fps+"size:"+size+"pts:"+pts);
                    //DebugLog.dfmt("ijkplayer", "type:%d, size:%d, fps: pts:%ld\n", type, size, pts);
                    if(size <= 0)
                    {
                        break;
                    }
                    int codec_id = (type == 8 ? 86018 : (v_enc == 0 ? 28 : 174));
                    byte data[] = new byte[size];
                    din.read(data);
                    if(first == 1)
                    {
                        byte[] jpg = IjkMediaPlayer.nativeConvertKeyframeToJPG(codec_id, data);
                        ret = IjkMediaPlayer.nativeConvertKeyframeToJPGFile(28, data, "/storage/emulated/0/tmp/out.jpg");
                        if(null != jpg && jpg.length > 0)
                        {
                            FileOutputStream jpgfile = new FileOutputStream("/storage/emulated/0/tmp/h264_to_jpg.jpg");
                            DataOutputStream jpgwriter = new DataOutputStream(jpgfile);
                            jpgwriter.write(jpg);
                            jpgwriter.close();
                        }
                        first = 0;
                    }
                    if(codec_id > 10000)
                    {
                        framenum = aframenum;
                        aframenum++;
                    }
                    else
                    {
                        framenum = vframenum;

                        if(vframenum%99==1)
                        {
                            vframenum++;
                            continue;
                        }
                        vframenum++;
                    }
                    mMediaPlayer.DeliverPacket(codec_id, data, pts, framenum);
                    //record.deliverPacket(codec_id, data, pts, framenum);
                    //System.out.print("mMediaPlayer.DeliverPacket");
                    //Log.d(TAG, "mMediaPlayer.DeliverPacket("+codec_id+"data"+pts);
                    //ideliver.DeliverData(type == 8 ? 1 : 0, data, pts);*/

                    IRecordDemux.RecordPacket pkt = recdemux.read_packet();
                    if(null == pkt)
                    {
                        //recdemux.open(sourceUrl);
                        //continue;
                        break;
                    }
                    mMediaPlayer.DeliverPacket(pkt.codec_id, pkt.data, pkt.timestamp, /*pkt.framenum*/-1);
                    record.deliverPacket(pkt.codec_id, pkt.data, pkt.timestamp, pkt.framenum);
                    pkt.data = null;
                    if(0 != encid) {
                        byte pcmdata[] = new byte[1024];
                        pcmreader.read(pcmdata);
                        byte[] aacdata = IjkMediaPlayer.nativeEncoderDeliverData(encid, pcmdata, 0);//mMediaPlayer.EncoderDeliverData(encid, pcmdata,0);
                        if (null != aacdata) {
                            aacwriter.write(aacdata);
                        }
                    }
                    long curvts = mMediaPlayer.GetVideoTimeStamp();
                    Log.d("IjkMedia", "codec_id:" + pkt.codec_id + "pts:" + pkt.timestamp + "framenum:" + pkt.framenum + "curvts:" + curvts);
                    try  {
                        Thread.sleep(55);
                    }
                    catch ( InterruptedException e)
                    {
                        System.out.print(e.toString());
                    }
                } catch (IOException e) {
                    System.out.print(e.toString());
                    break;
                }
            }
            byte data[] = null;
            mMediaPlayer.DeliverPacket(0, data, 0, framenum);
            mMediaPlayer.DeliverPacket(1, data, 0, framenum);
            record.stop(0);
            /*if(null != record) {
                record.deliverPacket(v_enc == 0 ? 28 : 174, null, 0);
                record.deliverPacket(86018, null, 0);
            }*/
            try {
                if(null != din) {
                    din.close();
                }
                if(null != aacwriter) {
                    aacwriter.close();
                }
            }
            catch(IOException e)
            {
                System.out.print(e.toString());
            }
            if(0 != encid) {
                IjkMediaPlayer.nativeEncoderClose(encid);
            }
            //mMediaPlayer.EncoderClose(encid);
        }

        public void stopDemux()
        {
            mbRun = false;
            try{
                super.join();
            }
            catch(InterruptedException e)
            {
                e.printStackTrace();
                System.out.print(e.toString());
            }

        }

    }

    private RecordDemuxer demuxer = null;
    private PCMEncoder    encoder = null;
    public IjkVideoView(Context context) {
        super(context);
        initVideoView(context);
    }

    public IjkVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initVideoView(context);
    }

    public IjkVideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        initVideoView(context);
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public IjkVideoView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        initVideoView(context);
    }

    // REMOVED: onMeasure
    // REMOVED: onInitializeAccessibilityEvent
    // REMOVED: onInitializeAccessibilityNodeInfo
    // REMOVED: resolveAdjustedSize

    private void initVideoView(Context context) {
        mAppContext = context.getApplicationContext();
        mSettings = new Settings(mAppContext);

        initBackground();
        initRenders();

        mVideoWidth = 0;
        mVideoHeight = 0;
        // REMOVED: getHolder().addCallback(mSHCallback);
        // REMOVED: getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();
        // REMOVED: mPendingSubtitleTracks = new Vector<Pair<InputStream, MediaFormat>>();
        mCurrentState = STATE_IDLE;
        mTargetState = STATE_IDLE;

        subtitleDisplay = new TextView(context);
        subtitleDisplay.setTextSize(24);
        subtitleDisplay.setGravity(Gravity.CENTER);
        FrameLayout.LayoutParams layoutParams_txt = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.BOTTOM);
        addView(subtitleDisplay, layoutParams_txt);
    }

    public void setRenderView(IRenderView renderView) {
        if (mRenderView != null) {
            if (mMediaPlayer != null)
                mMediaPlayer.setDisplay(null);

            View renderUIView = mRenderView.getView();
            mRenderView.removeRenderCallback(mSHCallback);
            mRenderView = null;
            removeView(renderUIView);
        }

        if (renderView == null)
            return;

        mRenderView = renderView;
        renderView.setAspectRatio(mCurrentAspectRatio);
        if (mVideoWidth > 0 && mVideoHeight > 0)
            renderView.setVideoSize(mVideoWidth, mVideoHeight);
        if (mVideoSarNum > 0 && mVideoSarDen > 0)
            renderView.setVideoSampleAspectRatio(mVideoSarNum, mVideoSarDen);

        View renderUIView = mRenderView.getView();
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.CENTER);
        renderUIView.setLayoutParams(lp);
        addView(renderUIView);

        mRenderView.addRenderCallback(mSHCallback);
        mRenderView.setVideoRotation(mVideoRotationDegree);
    }

    public void setRender(int render) {
        switch (render) {
            case RENDER_NONE:
                setRenderView(null);
                break;
            case RENDER_TEXTURE_VIEW: {
                TextureRenderView renderView = new TextureRenderView(getContext());
                if (mMediaPlayer != null) {
                    renderView.getSurfaceHolder().bindToMediaPlayer(mMediaPlayer);
                    renderView.setVideoSize(mMediaPlayer.getVideoWidth(), mMediaPlayer.getVideoHeight());
                    renderView.setVideoSampleAspectRatio(mMediaPlayer.getVideoSarNum(), mMediaPlayer.getVideoSarDen());
                    renderView.setAspectRatio(mCurrentAspectRatio);
                }
                setRenderView(renderView);
                break;
            }
            case RENDER_SURFACE_VIEW: {
                SurfaceRenderView renderView = new SurfaceRenderView(getContext());
                setRenderView(renderView);
                break;
            }
            default:
                Log.e(TAG, String.format(Locale.getDefault(), "invalid render %d\n", render));
                break;
        }
    }

    public void setHudView(TableLayout tableLayout) {
        mHudViewHolder = new InfoHudViewHolder(getContext(), tableLayout);
    }

    /**
     * Sets video path.
     *
     * @param path the path of the video.
     */
    public void setVideoPath(String path) {
        setVideoURI(Uri.parse(path));
    }

    /**
     * Sets video URI.
     *
     * @param uri the URI of the video.
     */
    public void setVideoURI(Uri uri) {
        setVideoURI(uri, null);
    }

    /**
     * Sets video URI using specific headers.
     *
     * @param uri     the URI of the video.
     * @param headers the headers for the URI request.
     *                Note that the cross domain redirection is allowed by default, but that can be
     *                changed with key/value pairs through the headers parameter with
     *                "android-allow-cross-domain-redirect" as the key and "0" or "1" as the value
     *                to disallow or allow cross domain redirection.
     */
    private void setVideoURI(Uri uri, Map<String, String> headers) {
        mUri = uri;
        String urlstr = uri.toString();
        if(!urlstr.contains(".data") && !urlstr.contains(".rec") && urlstr.contains("."))
        {
            msrcUri = mUri;
        }

        mHeaders = headers;
        mSeekWhenPrepared = 0;
        openVideo();
        requestLayout();
        invalidate();
    }

    // REMOVED: addSubtitleSource
    // REMOVED: mPendingSubtitleTracks

    public void stopPlayback() {
        if (mMediaPlayer != null) {
            System.out.print("before mMediaPlayer.stop()");
            mMediaPlayer.stop();
            System.out.print("after mMediaPlayer.stop()");
            mMediaPlayer.release();
            System.out.print("after mMediaPlayer.release()");
            mMediaPlayer = null;
            if (mHudViewHolder != null)
                mHudViewHolder.setMediaPlayer(null);
            mCurrentState = STATE_IDLE;
            mTargetState = STATE_IDLE;
            System.out.print("after mHudViewHolder.setMediaPlayer");
            AudioManager am = (AudioManager) mAppContext.getSystemService(Context.AUDIO_SERVICE);
            am.abandonAudioFocus(null);
        }
    }

    @TargetApi(Build.VERSION_CODES.M)
    private void openVideo() {
        if (mUri == null || mSurfaceHolder == null) {
            // not ready for playback just yet, will try again later
            return;
        }
        // we shouldn't clear the target state, because somebody might have
        // called start() previously
        if(!balreadyplay) {
            release(false);
            AudioManager am = (AudioManager) mAppContext.getSystemService(Context.AUDIO_SERVICE);
            am.requestAudioFocus(null, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
            balreadyplay = true;
        }

        try {
            mMediaPlayer = createPlayer(mSettings.getPlayer());
            //IjkMediaPlayer.native_setLogPath("/storage/emulated/0/tmp/ijkplayer_", 3, 3);
            // TODO: create SubtitleController in MediaPlayer, but we need
            String version = IjkMediaPlayer.nativeGetVersion();
            System.out.print(version);
            // a context for the subtitle renderers
            final Context context = getContext();
            // REMOVED: SubtitleController

            // REMOVED: mAudioSession
            mMediaPlayer.setOnPreparedListener(mPreparedListener);
            mMediaPlayer.setOnVideoSizeChangedListener(mSizeChangedListener);
            mMediaPlayer.setOnCompletionListener(mCompletionListener);
            mMediaPlayer.setOnErrorListener(mErrorListener);
            mMediaPlayer.setOnInfoListener(mInfoListener);
            mMediaPlayer.setOnBufferingUpdateListener(mBufferingUpdateListener);
            mMediaPlayer.setOnSeekCompleteListener(mSeekCompleteListener);
            mMediaPlayer.setOnTimedTextListener(mOnTimedTextListener);
            mCurrentBufferPercentage = 0;
            String scheme = mUri.getScheme();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M &&
                    mSettings.getUsingMediaDataSource() &&
                    (TextUtils.isEmpty(scheme) || scheme.equalsIgnoreCase("file"))) {
                IMediaDataSource dataSource = new FileMediaDataSource(new File(mUri.toString()));
                mMediaPlayer.setDataSource(dataSource);
            }  else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
                mMediaPlayer.setDataSource(mAppContext, msrcUri, mHeaders);
            } else {
                mMediaPlayer.setDataSource(msrcUri == null ? msrcUri.toString() : null);
            }
            bindSurfaceHolder(mMediaPlayer, mSurfaceHolder);
            mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
            mMediaPlayer.setScreenOnWhilePlaying(true);
            mPrepareStartTime = System.currentTimeMillis();
            mMediaPlayer.prepareAsync();
            if (mHudViewHolder != null)
                mHudViewHolder.setMediaPlayer(mMediaPlayer);
            byte[] data = new byte[1024];
            long curpos = mMediaPlayer.getCurrentPosition();
            //mMediaPlayer.native_setLogLevel(3);
            //mMediaPlayer.GetCurrentImage("/storage/emulated/0/tmp/test2.jpg");
            //mMediaPlayer.DeliverPacket(0, data, 0);
            //mMediaPlayer.DeliverPacket(1, data, 2);
            String ver = IjkMediaPlayer.nativeGetVersion();
            if(msrcUri == null) {
                if(Looper.myLooper() == Looper.getMainLooper())
                {
                    System.out.print("main thread");
                }

                System.out.print("demuxer = new RecordDemuxer");
                demuxer = new RecordDemuxer(mMediaPlayer, mUri.toString(), new datadeliver() {
                    public int DeliverData(final int mt, final byte[] data, final long pts) {
                        handler.post(new Runnable() {
                            @Override
                            public void run() {
                                if(Looper.myLooper() == Looper.getMainLooper())
                                {
                                    System.out.print("main thread");
                                }
                                mMediaPlayer.DeliverPacket(mt, data, pts, -1);
                            }
                        });

                        return 0;
                    }
                });
                demuxer.start();

            }

            // REMOVED: mPendingSubtitleTracks

            // we don't set the target state here either, but preserve the
            // target state that was there before.
            mCurrentState = STATE_PREPARING;
            attachMediaController();
        } catch (IOException ex) {
            Log.w(TAG, "Unable to open content: " + mUri, ex);
            mCurrentState = STATE_ERROR;
            mTargetState = STATE_ERROR;
            mErrorListener.onError(mMediaPlayer, MediaPlayer.MEDIA_ERROR_UNKNOWN, 0);
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "Unable to open content: " + mUri, ex);
            mCurrentState = STATE_ERROR;
            mTargetState = STATE_ERROR;
            mErrorListener.onError(mMediaPlayer, MediaPlayer.MEDIA_ERROR_UNKNOWN, 0);
        } finally {
            // REMOVED: mPendingSubtitleTracks.clear();
        }
    }

    public void setMediaController(IMediaController controller) {
        if (mMediaController != null) {
            mMediaController.hide();
        }
        mMediaController = controller;
        attachMediaController();
    }

    private void attachMediaController() {
        if (mMediaPlayer != null && mMediaController != null) {
            mMediaController.setMediaPlayer(this);
            View anchorView = this.getParent() instanceof View ?
                    (View) this.getParent() : this;
            mMediaController.setAnchorView(anchorView);
            mMediaController.setEnabled(isInPlaybackState());
        }
    }

    IMediaPlayer.OnVideoSizeChangedListener mSizeChangedListener =
            new IMediaPlayer.OnVideoSizeChangedListener() {
                public void onVideoSizeChanged(IMediaPlayer mp, int width, int height, int sarNum, int sarDen) {
                    mVideoWidth = mp.getVideoWidth();
                    mVideoHeight = mp.getVideoHeight();
                    mVideoSarNum = mp.getVideoSarNum();
                    mVideoSarDen = mp.getVideoSarDen();
                    if (mVideoWidth != 0 && mVideoHeight != 0) {
                        if (mRenderView != null) {
                            mRenderView.setVideoSize(mVideoWidth, mVideoHeight);
                            mRenderView.setVideoSampleAspectRatio(mVideoSarNum, mVideoSarDen);
                        }
                        // REMOVED: getHolder().setFixedSize(mVideoWidth, mVideoHeight);
                        requestLayout();
                    }
                }
            };

    IMediaPlayer.OnPreparedListener mPreparedListener = new IMediaPlayer.OnPreparedListener() {
        public void onPrepared(IMediaPlayer mp) {
            mPrepareEndTime = System.currentTimeMillis();
            mHudViewHolder.updateLoadCost(mPrepareEndTime - mPrepareStartTime);
            mCurrentState = STATE_PREPARED;

            // Get the capabilities of the player for this stream
            // REMOVED: Metadata

            if (mOnPreparedListener != null) {
                mOnPreparedListener.onPrepared(mMediaPlayer);
            }
            if (mMediaController != null) {
                mMediaController.setEnabled(true);
            }
            mVideoWidth = mp.getVideoWidth();
            mVideoHeight = mp.getVideoHeight();

            int seekToPosition = mSeekWhenPrepared;  // mSeekWhenPrepared may be changed after seekTo() call
            if (seekToPosition != 0) {
                seekTo(seekToPosition);
            }
            System.out.print("mVideoWidth: " + mVideoWidth + " mVideoHeight: "+mVideoHeight);
            if (mVideoWidth != 0 && mVideoHeight != 0) {
                //Log.i("@@@@", "video size: " + mVideoWidth +"/"+ mVideoHeight);
                // REMOVED: getHolder().setFixedSize(mVideoWidth, mVideoHeight);
                if (mRenderView != null) {
                    mRenderView.setVideoSize(mVideoWidth, mVideoHeight);
                    mRenderView.setVideoSampleAspectRatio(mVideoSarNum, mVideoSarDen);
                    if (!mRenderView.shouldWaitForResize() || mSurfaceWidth == mVideoWidth && mSurfaceHeight == mVideoHeight) {
                        // We didn't actually change the size (it was already at the size
                        // we need), so we won't get a "surface changed" callback, so
                        // start the video here instead of in the callback.
                        if (mTargetState == STATE_PLAYING) {
                            start();
                            if (mMediaController != null) {
                                mMediaController.show();
                            }
                        } else if (!isPlaying() &&
                                (seekToPosition != 0 || getCurrentPosition() > 0)) {
                            if (mMediaController != null) {
                                // Show the media controls when we're paused into a video and make 'em stick.
                                mMediaController.show(0);
                            }
                        }
                    }
                }
            } else {
                // We don't know the video size yet, but should start anyway.
                // The video size might be reported to us later.
                if (mTargetState == STATE_PLAYING) {
                    start();
                }
            }
        }
    };

    private IMediaPlayer.OnCompletionListener mCompletionListener =
            new IMediaPlayer.OnCompletionListener() {
                public void onCompletion(IMediaPlayer mp) {
                    mCurrentState = STATE_PLAYBACK_COMPLETED;
                    mTargetState = STATE_PLAYBACK_COMPLETED;
                    if (mMediaController != null) {
                        mMediaController.hide();
                    }
                    if (mOnCompletionListener != null) {
                        mOnCompletionListener.onCompletion(mMediaPlayer);
                    }
                }
            };

    private IMediaPlayer.OnInfoListener mInfoListener =
            new IMediaPlayer.OnInfoListener() {
                public boolean onInfo(IMediaPlayer mp, int arg1, int arg2) {
                    if (mOnInfoListener != null) {
                        mOnInfoListener.onInfo(mp, arg1, arg2);
                    }
                    switch (arg1) {
                        case IMediaPlayer.MEDIA_INFO_VIDEO_TRACK_LAGGING:
                            Log.d(TAG, "MEDIA_INFO_VIDEO_TRACK_LAGGING:");
                            break;
                        case IMediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START:
                            Log.d(TAG, "MEDIA_INFO_VIDEO_RENDERING_START:");
                            break;
                        case IMediaPlayer.MEDIA_INFO_BUFFERING_START:
                            Log.d(TAG, "MEDIA_INFO_BUFFERING_START:");
                            break;
                        case IMediaPlayer.MEDIA_INFO_BUFFERING_END:
                            Log.d(TAG, "MEDIA_INFO_BUFFERING_END:");
                            break;
                        case IMediaPlayer.MEDIA_INFO_NETWORK_BANDWIDTH:
                            Log.d(TAG, "MEDIA_INFO_NETWORK_BANDWIDTH: " + arg2);
                            break;
                        case IMediaPlayer.MEDIA_INFO_BAD_INTERLEAVING:
                            Log.d(TAG, "MEDIA_INFO_BAD_INTERLEAVING:");
                            break;
                        case IMediaPlayer.MEDIA_INFO_NOT_SEEKABLE:
                            Log.d(TAG, "MEDIA_INFO_NOT_SEEKABLE:");
                            break;
                        case IMediaPlayer.MEDIA_INFO_METADATA_UPDATE:
                            Log.d(TAG, "MEDIA_INFO_METADATA_UPDATE:");
                            break;
                        case IMediaPlayer.MEDIA_INFO_UNSUPPORTED_SUBTITLE:
                            Log.d(TAG, "MEDIA_INFO_UNSUPPORTED_SUBTITLE:");
                            break;
                        case IMediaPlayer.MEDIA_INFO_SUBTITLE_TIMED_OUT:
                            Log.d(TAG, "MEDIA_INFO_SUBTITLE_TIMED_OUT:");
                            break;
                        case IMediaPlayer.MEDIA_INFO_VIDEO_ROTATION_CHANGED:
                            mVideoRotationDegree = arg2;
                            Log.d(TAG, "MEDIA_INFO_VIDEO_ROTATION_CHANGED: " + arg2);
                            if (mRenderView != null)
                                mRenderView.setVideoRotation(arg2);
                            break;
                        case IMediaPlayer.MEDIA_INFO_AUDIO_RENDERING_START:
                            Log.d(TAG, "MEDIA_INFO_AUDIO_RENDERING_START:");
                            break;
                        /*case MEDIA_MUXER_MSG:
                            switch(arg1)
                            {
                                case IMediaPlayer.MEDIA_MUXER_MSG_START:
                                    System.out.print("MEDIA_MUXER_MSG_START\n");
                                    break;
                                case IMediaPlayer.MEDIA_MUXERT_MSG_PROGRESS:
                                    System.out.print("MEDIA_MUXERT_MSG_PROGRESS:");
                                    System.out.print(arg2);
                                    break;
                                case IMediaPlayer.MEDIA_MUXERT_MSG_COMPLETE:
                                    System.out.print("MEDIA_MUXERT_MSG_COMPLETE:");
                                    System.out.print(arg2);
                                    break;
                                case IMediaPlayer.MEDIA_MUXERT_MSG_ERROR:
                                    System.out.print("MEDIA_MUXERT_MSG_ERROR:");
                                    System.out.print(arg2);
                                    break;
                                default:
                                    System.out.print("unknown muxer msg:");
                                    System.out.print(arg1);
                                    break;
                            }*/
                        default:
                            System.out.print("unknown muxer msg:");
                            System.out.print(arg1);
                            break;
                    }
                    return true;
                }
            };
    int MEDIA_MUXER_MSG                 = 2000;
    int MEDIA_MUXER_MSG_START           = 2001;
    int MEDIA_MUXERT_MSG_PROGRESS       = 2002;
    int MEDIA_MUXERT_MSG_COMPLETE       = 2003;
    int MEDIA_MUXERT_MSG_ERROR          = 2004;
    private IMediaPlayer.OnErrorListener mErrorListener =
            new IMediaPlayer.OnErrorListener() {
                public boolean onError(IMediaPlayer mp, int framework_err, int impl_err) {
                    Log.d(TAG, "Error: " + framework_err + "," + impl_err);
                    mCurrentState = STATE_ERROR;
                    mTargetState = STATE_ERROR;
                    if (mMediaController != null) {
                        mMediaController.hide();
                    }

                    /* If an error handler has been supplied, use it and finish. */
                    if (mOnErrorListener != null) {
                        if (mOnErrorListener.onError(mMediaPlayer, framework_err, impl_err)) {
                            return true;
                        }
                    }

                    /* Otherwise, pop up an error dialog so the user knows that
                     * something bad has happened. Only try and pop up the dialog
                     * if we're attached to a window. When we're going away and no
                     * longer have a window, don't bother showing the user an error.
                     */
                    if (getWindowToken() != null) {
                        Resources r = mAppContext.getResources();
                        int messageId;

                        if (framework_err == MediaPlayer.MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK) {
                            messageId = R.string.VideoView_error_text_invalid_progressive_playback;
                        } else {
                            messageId = R.string.VideoView_error_text_unknown;
                        }

                        new AlertDialog.Builder(getContext())
                                .setMessage(messageId)
                                .setPositiveButton(R.string.VideoView_error_button,
                                        new DialogInterface.OnClickListener() {
                                            public void onClick(DialogInterface dialog, int whichButton) {
                                            /* If we get here, there is no onError listener, so
                                             * at least inform them that the video is over.
                                             */
                                                if (mOnCompletionListener != null) {
                                                    mOnCompletionListener.onCompletion(mMediaPlayer);
                                                }
                                            }
                                        })
                                .setCancelable(false)
                                .show();
                    }
                    return true;
                }
            };

    private IMediaPlayer.OnBufferingUpdateListener mBufferingUpdateListener =
            new IMediaPlayer.OnBufferingUpdateListener() {
                public void onBufferingUpdate(IMediaPlayer mp, int percent) {
                    mCurrentBufferPercentage = percent;
                }
            };

    private IMediaPlayer.OnSeekCompleteListener mSeekCompleteListener = new IMediaPlayer.OnSeekCompleteListener() {

        @Override
        public void onSeekComplete(IMediaPlayer mp) {
            mSeekEndTime = System.currentTimeMillis();
            mHudViewHolder.updateSeekCost(mSeekEndTime - mSeekStartTime);
        }
    };

    private IMediaPlayer.OnTimedTextListener mOnTimedTextListener = new IMediaPlayer.OnTimedTextListener() {
        @Override
        public void onTimedText(IMediaPlayer mp, IjkTimedText text) {
            if (text != null) {
                subtitleDisplay.setText(text.getText());
            }
        }
    };

    /**
     * Register a callback to be invoked when the media file
     * is loaded and ready to go.
     *
     * @param l The callback that will be run
     */
    public void setOnPreparedListener(IMediaPlayer.OnPreparedListener l) {
        mOnPreparedListener = l;
    }

    /**
     * Register a callback to be invoked when the end of a media file
     * has been reached during playback.
     *
     * @param l The callback that will be run
     */
    public void setOnCompletionListener(IMediaPlayer.OnCompletionListener l) {
        mOnCompletionListener = l;
    }

    /**
     * Register a callback to be invoked when an error occurs
     * during playback or setup.  If no listener is specified,
     * or if the listener returned false, VideoView will inform
     * the user of any errors.
     *
     * @param l The callback that will be run
     */
    public void setOnErrorListener(IMediaPlayer.OnErrorListener l) {
        mOnErrorListener = l;
    }

    /**
     * Register a callback to be invoked when an informational event
     * occurs during playback or setup.
     *
     * @param l The callback that will be run
     */
    public void setOnInfoListener(IMediaPlayer.OnInfoListener l) {
        mOnInfoListener = l;
    }

    // REMOVED: mSHCallback
    private void bindSurfaceHolder(IMediaPlayer mp, IRenderView.ISurfaceHolder holder) {
        if (mp == null)
            return;

        if (holder == null) {
            mp.setDisplay(null);
            return;
        }

        holder.bindToMediaPlayer(mp);
    }

    IRenderView.IRenderCallback mSHCallback = new IRenderView.IRenderCallback() {
        @Override
        public void onSurfaceChanged(@NonNull IRenderView.ISurfaceHolder holder, int format, int w, int h) {
            if (holder.getRenderView() != mRenderView) {
                Log.e(TAG, "onSurfaceChanged: unmatched render callback\n");
                return;
            }

            mSurfaceWidth = w;
            mSurfaceHeight = h;
            boolean isValidState = (mTargetState == STATE_PLAYING);
            boolean hasValidSize = !mRenderView.shouldWaitForResize() || (mVideoWidth == w && mVideoHeight == h);
            if (mMediaPlayer != null && isValidState && hasValidSize) {
                if (mSeekWhenPrepared != 0) {
                    seekTo(mSeekWhenPrepared);
                }
                start();
            }
        }

        @Override
        public void onSurfaceCreated(@NonNull IRenderView.ISurfaceHolder holder, int width, int height) {
            if (holder.getRenderView() != mRenderView) {
                Log.e(TAG, "onSurfaceCreated: unmatched render callback\n");
                return;
            }

            mSurfaceHolder = holder;
            if (mMediaPlayer != null)
                bindSurfaceHolder(mMediaPlayer, holder);
            else
                openVideo();
        }

        @Override
        public void onSurfaceDestroyed(@NonNull IRenderView.ISurfaceHolder holder) {
            if(null != demuxer) {
                System.out.print("onSurfaceDestroyed");
                demuxer.stopDemux();
            }
            if(null != encoder)
            {
                encoder.stopEncoder();
            }
            if (holder.getRenderView() != mRenderView) {
                Log.e(TAG, "onSurfaceDestroyed: unmatched render callback\n");
                return;
            }

            // after we return from this we can't use the surface any more
            mSurfaceHolder = null;
            // REMOVED: if (mMediaController != null) mMediaController.hide();
            // REMOVED: release(true);
            releaseWithoutStop();
        }
    };

    public void releaseWithoutStop() {
        System.out.print("releaseWithoutStop");
        if (mMediaPlayer != null)
            mMediaPlayer.setDisplay(null);
    }

    /*
     * release the media player in any state
     */
    public void release(boolean cleartargetstate) {
        System.out.print("release player");
        if(null != demuxer) {
            System.out.print("onSurfaceDestroyed");
            demuxer.stopDemux();
        }
        if(null != encoder)
        {
            encoder.stopEncoder();
        }
        if (mMediaPlayer != null) {
            mMediaPlayer.reset();
            mMediaPlayer.release();
            mMediaPlayer = null;
            // REMOVED: mPendingSubtitleTracks.clear();
            mCurrentState = STATE_IDLE;
            if (cleartargetstate) {
                mTargetState = STATE_IDLE;
            }
            AudioManager am = (AudioManager) mAppContext.getSystemService(Context.AUDIO_SERVICE);
            am.abandonAudioFocus(null);
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        if (isInPlaybackState() && mMediaController != null) {
            toggleMediaControlsVisiblity();
        }
        return false;
    }

    @Override
    public boolean onTrackballEvent(MotionEvent ev) {
        if (isInPlaybackState() && mMediaController != null) {
            toggleMediaControlsVisiblity();
        }
        return false;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        boolean isKeyCodeSupported = keyCode != KeyEvent.KEYCODE_BACK &&
                keyCode != KeyEvent.KEYCODE_VOLUME_UP &&
                keyCode != KeyEvent.KEYCODE_VOLUME_DOWN &&
                keyCode != KeyEvent.KEYCODE_VOLUME_MUTE &&
                keyCode != KeyEvent.KEYCODE_MENU &&
                keyCode != KeyEvent.KEYCODE_CALL &&
                keyCode != KeyEvent.KEYCODE_ENDCALL;
        if (isInPlaybackState() && isKeyCodeSupported && mMediaController != null) {
            if (keyCode == KeyEvent.KEYCODE_HEADSETHOOK ||
                    keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE) {
                if (mMediaPlayer.isPlaying()) {
                    pause();
                    mMediaController.show();
                } else {
                    start();
                    mMediaController.hide();
                }
                return true;
            } else if (keyCode == KeyEvent.KEYCODE_MEDIA_PLAY) {
                if (!mMediaPlayer.isPlaying()) {
                    start();
                    mMediaController.hide();
                }
                return true;
            } else if (keyCode == KeyEvent.KEYCODE_MEDIA_STOP
                    || keyCode == KeyEvent.KEYCODE_MEDIA_PAUSE) {
                if (mMediaPlayer.isPlaying()) {
                    pause();
                    mMediaController.show();
                }
                return true;
            } else {
                toggleMediaControlsVisiblity();
            }
        }

        return super.onKeyDown(keyCode, event);
    }

    private void toggleMediaControlsVisiblity() {
        if (mMediaController.isShowing()) {
            mMediaController.hide();
        } else {
            mMediaController.show();
        }
    }

    @Override
    public void start() {
        if (isInPlaybackState()) {
            mMediaPlayer.start();
            mCurrentState = STATE_PLAYING;
        }
        mTargetState = STATE_PLAYING;
    }

    @Override
    public void pause() {
        if (isInPlaybackState()) {
            if (mMediaPlayer.isPlaying()) {
                mMediaPlayer.pause();
                mCurrentState = STATE_PAUSED;
            }
        }
        mTargetState = STATE_PAUSED;
    }

    public void suspend() {
        release(false);
    }

    public void resume() {
        openVideo();
    }

    @Override
    public int getDuration() {
        if (isInPlaybackState()) {
            return (int) mMediaPlayer.getDuration();
        }

        return -1;
    }

    @Override
    public int getCurrentPosition() {
        if (isInPlaybackState()) {
            return (int) mMediaPlayer.getCurrentPosition();
        }
        return 0;
    }

    @Override
    public void seekTo(int msec) {
        if (isInPlaybackState()) {
            mSeekStartTime = System.currentTimeMillis();
            mMediaPlayer.seekTo(msec);
            mSeekWhenPrepared = 0;
        } else {
            mSeekWhenPrepared = msec;
        }
    }

    @Override
    public boolean isPlaying() {
        return isInPlaybackState() && mMediaPlayer.isPlaying();
    }

    @Override
    public int getBufferPercentage() {
        if (mMediaPlayer != null) {
            return mCurrentBufferPercentage;
        }
        return 0;
    }

    private boolean isInPlaybackState() {
        return (mMediaPlayer != null &&
                mCurrentState != STATE_ERROR &&
                mCurrentState != STATE_IDLE &&
                mCurrentState != STATE_PREPARING);
    }

    @Override
    public boolean canPause() {
        return mCanPause;
    }

    @Override
    public boolean canSeekBackward() {
        return mCanSeekBack;
    }

    @Override
    public boolean canSeekForward() {
        return mCanSeekForward;
    }

    @Override
    public int getAudioSessionId() {
        return 0;
    }

    // REMOVED: getAudioSessionId();
    // REMOVED: onAttachedToWindow();
    // REMOVED: onDetachedFromWindow();
    // REMOVED: onLayout();
    // REMOVED: draw();
    // REMOVED: measureAndLayoutSubtitleWidget();
    // REMOVED: setSubtitleWidget();
    // REMOVED: getSubtitleLooper();

    //-------------------------
    // Extend: Aspect Ratio
    //-------------------------

    private static final int[] s_allAspectRatio = {
            IRenderView.AR_ASPECT_FIT_PARENT,
            IRenderView.AR_ASPECT_FILL_PARENT,
            IRenderView.AR_ASPECT_WRAP_CONTENT,
            // IRenderView.AR_MATCH_PARENT,
            IRenderView.AR_16_9_FIT_PARENT,
            IRenderView.AR_4_3_FIT_PARENT};
    private int mCurrentAspectRatioIndex = 0;
    private int mCurrentAspectRatio = s_allAspectRatio[0];

    public int toggleAspectRatio() {
        mCurrentAspectRatioIndex++;
        mCurrentAspectRatioIndex %= s_allAspectRatio.length;

        mCurrentAspectRatio = s_allAspectRatio[mCurrentAspectRatioIndex];
        if (mRenderView != null)
            mRenderView.setAspectRatio(mCurrentAspectRatio);
        return mCurrentAspectRatio;
    }

    //-------------------------
    // Extend: Render
    //-------------------------
    public static final int RENDER_NONE = 0;
    public static final int RENDER_SURFACE_VIEW = 1;
    public static final int RENDER_TEXTURE_VIEW = 2;

    private List<Integer> mAllRenders = new ArrayList<Integer>();
    private int mCurrentRenderIndex = 0;
    private int mCurrentRender = RENDER_NONE;

    private void initRenders() {
        mAllRenders.clear();

        if (mSettings.getEnableSurfaceView())
            mAllRenders.add(RENDER_SURFACE_VIEW);
        if (mSettings.getEnableTextureView() && Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH)
            mAllRenders.add(RENDER_TEXTURE_VIEW);
        if (mSettings.getEnableNoView())
            mAllRenders.add(RENDER_NONE);

        if (mAllRenders.isEmpty())
            mAllRenders.add(RENDER_SURFACE_VIEW);
        mCurrentRender = mAllRenders.get(mCurrentRenderIndex);
        setRender(mCurrentRender);
    }

    public int toggleRender() {
        mCurrentRenderIndex++;
        mCurrentRenderIndex %= mAllRenders.size();

        mCurrentRender = mAllRenders.get(mCurrentRenderIndex);
        setRender(mCurrentRender);
        return mCurrentRender;
    }

    @NonNull
    public static String getRenderText(Context context, int render) {
        String text;
        switch (render) {
            case RENDER_NONE:
                text = context.getString(R.string.VideoView_render_none);
                break;
            case RENDER_SURFACE_VIEW:
                text = context.getString(R.string.VideoView_render_surface_view);
                break;
            case RENDER_TEXTURE_VIEW:
                text = context.getString(R.string.VideoView_render_texture_view);
                break;
            default:
                text = context.getString(R.string.N_A);
                break;
        }
        return text;
    }

    //-------------------------
    // Extend: Player
    //-------------------------
    public int togglePlayer() {
        if (mMediaPlayer != null)
            mMediaPlayer.release();

        if (mRenderView != null)
            mRenderView.getView().invalidate();
        openVideo();
        return mSettings.getPlayer();
    }

    @NonNull
    public static String getPlayerText(Context context, int player) {
        String text;
        switch (player) {
            case Settings.PV_PLAYER__AndroidMediaPlayer:
                text = context.getString(R.string.VideoView_player_AndroidMediaPlayer);
                break;
            case Settings.PV_PLAYER__IjkMediaPlayer:
                text = context.getString(R.string.VideoView_player_IjkMediaPlayer);
                break;
            case Settings.PV_PLAYER__IjkExoMediaPlayer:
                text = context.getString(R.string.VideoView_player_IjkExoMediaPlayer);
                break;
            default:
                text = context.getString(R.string.N_A);
                break;
        }
        return text;
    }

    public IMediaPlayer createPlayer(int playerType) {
        IMediaPlayer mediaPlayer = null;

        switch (playerType) {
            case Settings.PV_PLAYER__IjkExoMediaPlayer: {
                IjkExoMediaPlayer IjkExoMediaPlayer = new IjkExoMediaPlayer(mAppContext);
                mediaPlayer = IjkExoMediaPlayer;
            }
            break;
            case Settings.PV_PLAYER__AndroidMediaPlayer: {
                AndroidMediaPlayer androidMediaPlayer = new AndroidMediaPlayer();
                mediaPlayer = androidMediaPlayer;
            }
            break;
            case Settings.PV_PLAYER__IjkMediaPlayer:
            default: {
                IjkMediaPlayer ijkMediaPlayer = null;
                if (mUri != null) {
                    ijkMediaPlayer = new IjkMediaPlayer();
                    IjkMediaPlayer.native_setLogPath("/storage/emulated/0/tmp/log/ijkplay_", 3, 3);
                    //ijkMediaPlayer.native_setLogLevel(IjkMediaPlayer.IJK_LOG_DEBUG);

                    if (mSettings.getUsingMediaCodec()) {
                        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec", 1);
                        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "dropframe", 1);
                        if (mSettings.getUsingMediaCodecAutoRotate()) {
                            ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-auto-rotate", 1);
                        } else {
                            ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-auto-rotate", 0);
                        }
                        if (mSettings.getMediaCodecHandleResolutionChange()) {
                            ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-handle-resolution-change", 1);
                        } else {
                            ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-handle-resolution-change", 0);
                        }
                    } else {
                        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec", 0);
                    }

                    if (mSettings.getUsingOpenSLES()) {
                        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "opensles", 1);
                    } else {
                        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "opensles", 0);
                    }

                    String pixelFormat = mSettings.getPixelFormat();
                    if (TextUtils.isEmpty(pixelFormat)) {
                        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "overlay-format", IjkMediaPlayer.SDL_FCC_RV32);
                    } else {
                        ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "overlay-format", pixelFormat);
                    }
                    ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "framedrop", 1);
                    ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "start-on-prepared", 0);

                    ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_FORMAT, "http-detect-range-support", 0);

                    ijkMediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_CODEC, "skip_loop_filter", 48);
                }
                mediaPlayer = ijkMediaPlayer;
            }
            break;
        }

        if (mSettings.getEnableDetachedSurfaceTextureView()) {
            mediaPlayer = new TextureMediaPlayer(mediaPlayer);
        }

        return mediaPlayer;
    }

    //-------------------------
    // Extend: Background
    //-------------------------

    private boolean mEnableBackgroundPlay = false;

    private void initBackground() {
        mEnableBackgroundPlay = mSettings.getEnableBackgroundPlay();
        if (mEnableBackgroundPlay) {
            MediaPlayerService.intentToStart(getContext());
            mMediaPlayer = MediaPlayerService.getMediaPlayer();
            if (mHudViewHolder != null)
                mHudViewHolder.setMediaPlayer(mMediaPlayer);
        }
    }

    public boolean isBackgroundPlayEnabled() {
        return mEnableBackgroundPlay;
    }

    public void enterBackground() {
        MediaPlayerService.setMediaPlayer(mMediaPlayer);
    }

    public void stopBackgroundPlay() {
        System.out.print("stopBackgroundPlay");
        MediaPlayerService.setMediaPlayer(null);
    }

    //-------------------------
    // Extend: Background
    //-------------------------
    public void showMediaInfo() {
        if (mMediaPlayer == null)
            return;

        int selectedVideoTrack = MediaPlayerCompat.getSelectedTrack(mMediaPlayer, ITrackInfo.MEDIA_TRACK_TYPE_VIDEO);
        int selectedAudioTrack = MediaPlayerCompat.getSelectedTrack(mMediaPlayer, ITrackInfo.MEDIA_TRACK_TYPE_AUDIO);
        int selectedSubtitleTrack = MediaPlayerCompat.getSelectedTrack(mMediaPlayer, ITrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT);

        TableLayoutBinder builder = new TableLayoutBinder(getContext());
        builder.appendSection(R.string.mi_player);
        builder.appendRow2(R.string.mi_player, MediaPlayerCompat.getName(mMediaPlayer));
        builder.appendSection(R.string.mi_media);
        builder.appendRow2(R.string.mi_resolution, buildResolution(mVideoWidth, mVideoHeight, mVideoSarNum, mVideoSarDen));
        builder.appendRow2(R.string.mi_length, buildTimeMilli(mMediaPlayer.getDuration()));

        ITrackInfo trackInfos[] = mMediaPlayer.getTrackInfo();
        if (trackInfos != null) {
            int index = -1;
            for (ITrackInfo trackInfo : trackInfos) {
                index++;

                int trackType = trackInfo.getTrackType();
                if (index == selectedVideoTrack) {
                    builder.appendSection(getContext().getString(R.string.mi_stream_fmt1, index) + " " + getContext().getString(R.string.mi__selected_video_track));
                } else if (index == selectedAudioTrack) {
                    builder.appendSection(getContext().getString(R.string.mi_stream_fmt1, index) + " " + getContext().getString(R.string.mi__selected_audio_track));
                } else if (index == selectedSubtitleTrack) {
                    builder.appendSection(getContext().getString(R.string.mi_stream_fmt1, index) + " " + getContext().getString(R.string.mi__selected_subtitle_track));
                } else {
                    builder.appendSection(getContext().getString(R.string.mi_stream_fmt1, index));
                }
                builder.appendRow2(R.string.mi_type, buildTrackType(trackType));
                builder.appendRow2(R.string.mi_language, buildLanguage(trackInfo.getLanguage()));

                IMediaFormat mediaFormat = trackInfo.getFormat();
                if (mediaFormat == null) {
                } else if (mediaFormat instanceof IjkMediaFormat) {
                    switch (trackType) {
                        case ITrackInfo.MEDIA_TRACK_TYPE_VIDEO:
                            builder.appendRow2(R.string.mi_codec, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_LONG_NAME_UI));
                            builder.appendRow2(R.string.mi_profile_level, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_PROFILE_LEVEL_UI));
                            builder.appendRow2(R.string.mi_pixel_format, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_PIXEL_FORMAT_UI));
                            builder.appendRow2(R.string.mi_resolution, mediaFormat.getString(IjkMediaFormat.KEY_IJK_RESOLUTION_UI));
                            builder.appendRow2(R.string.mi_frame_rate, mediaFormat.getString(IjkMediaFormat.KEY_IJK_FRAME_RATE_UI));
                            builder.appendRow2(R.string.mi_bit_rate, mediaFormat.getString(IjkMediaFormat.KEY_IJK_BIT_RATE_UI));
                            break;
                        case ITrackInfo.MEDIA_TRACK_TYPE_AUDIO:
                            builder.appendRow2(R.string.mi_codec, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_LONG_NAME_UI));
                            builder.appendRow2(R.string.mi_profile_level, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_PROFILE_LEVEL_UI));
                            builder.appendRow2(R.string.mi_sample_rate, mediaFormat.getString(IjkMediaFormat.KEY_IJK_SAMPLE_RATE_UI));
                            builder.appendRow2(R.string.mi_channels, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CHANNEL_UI));
                            builder.appendRow2(R.string.mi_bit_rate, mediaFormat.getString(IjkMediaFormat.KEY_IJK_BIT_RATE_UI));
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        AlertDialog.Builder adBuilder = builder.buildAlertDialogBuilder();
        adBuilder.setTitle(R.string.media_information);
        adBuilder.setNegativeButton(R.string.close, null);
        adBuilder.show();
    }

    private String buildResolution(int width, int height, int sarNum, int sarDen) {
        StringBuilder sb = new StringBuilder();
        sb.append(width);
        sb.append(" x ");
        sb.append(height);

        if (sarNum > 1 || sarDen > 1) {
            sb.append("[");
            sb.append(sarNum);
            sb.append(":");
            sb.append(sarDen);
            sb.append("]");
        }

        return sb.toString();
    }

    private String buildTimeMilli(long duration) {
        long total_seconds = duration / 1000;
        long hours = total_seconds / 3600;
        long minutes = (total_seconds % 3600) / 60;
        long seconds = total_seconds % 60;
        if (duration <= 0) {
            return "--:--";
        }
        if (hours >= 100) {
            return String.format(Locale.US, "%d:%02d:%02d", hours, minutes, seconds);
        } else if (hours > 0) {
            return String.format(Locale.US, "%02d:%02d:%02d", hours, minutes, seconds);
        } else {
            return String.format(Locale.US, "%02d:%02d", minutes, seconds);
        }
    }

    private String buildTrackType(int type) {
        Context context = getContext();
        switch (type) {
            case ITrackInfo.MEDIA_TRACK_TYPE_VIDEO:
                return context.getString(R.string.TrackType_video);
            case ITrackInfo.MEDIA_TRACK_TYPE_AUDIO:
                return context.getString(R.string.TrackType_audio);
            case ITrackInfo.MEDIA_TRACK_TYPE_SUBTITLE:
                return context.getString(R.string.TrackType_subtitle);
            case ITrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT:
                return context.getString(R.string.TrackType_timedtext);
            case ITrackInfo.MEDIA_TRACK_TYPE_METADATA:
                return context.getString(R.string.TrackType_metadata);
            case ITrackInfo.MEDIA_TRACK_TYPE_UNKNOWN:
            default:
                return context.getString(R.string.TrackType_unknown);
        }
    }

    private String buildLanguage(String language) {
        if (TextUtils.isEmpty(language))
            return "und";
        return language;
    }

    public ITrackInfo[] getTrackInfo() {
        if (mMediaPlayer == null)
            return null;

        return mMediaPlayer.getTrackInfo();
    }

    public void selectTrack(int stream) {
        MediaPlayerCompat.selectTrack(mMediaPlayer, stream);
    }

    public void deselectTrack(int stream) {
        MediaPlayerCompat.deselectTrack(mMediaPlayer, stream);
    }

    public int getSelectedTrack(int trackType) {
        return MediaPlayerCompat.getSelectedTrack(mMediaPlayer, trackType);
    }

    public void OnRecordDown(Button bnt)
    {
        System.out.println("OnRecordDown");
        if(!mbRecord)
        {
            System.out.println("OnRecordDown record");
            long curtime = System.currentTimeMillis();
            String galleryPath = Environment.getExternalStorageDirectory()
                    + File.separator + Environment.DIRECTORY_DCIM
                    +File.separator+"Camera"+File.separator;
            Date date = new Date(curtime);
            SimpleDateFormat dateFormat = new SimpleDateFormat("yyyyMMdd-HHmmss");
            String recpath = galleryPath + dateFormat.format(date)+".mov";
            String tmppath = "/storage/emulated/0/tmp/" + dateFormat.format(date)+".tmp";
            System.out.println("recpath:"+recpath);
            System.out.println("tmppath:"+tmppath);
            mMediaPlayer.StartRecord(recpath, tmppath);
            System.out.println("after StartRecord");
            bnt.setText("Stop Record");
            mbRecord = true;
        }
        else
        {
            System.out.println("before StopRecord");
            mMediaPlayer.StopRecord(0);
            System.out.println("after StopRecord");
            bnt.setText("Start Record");
            mbRecord = false;
        }
    }

    public void OnShotImage(Button bnt)
    {
        System.out.println("Shot image");
        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyyMMdd-HHmmss");
        long curtime = System.currentTimeMillis();
        Date date = new Date(curtime);
        String galleryPath = Environment.getExternalStorageDirectory()
                + File.separator + Environment.DIRECTORY_DCIM
                +File.separator+"Camera"+File.separator;
        String imgpath = galleryPath + dateFormat.format(date)+".jpg";
        System.out.println("imgpath:"+imgpath);
        mMediaPlayer.GetCurrentImage(imgpath);
    }

    public void OnMute(Button bnt)
    {
        System.out.println("Mute");
        int mute = mMediaPlayer.getPlaybackMute();
        mute = mute == 0 ? 1 : 0;
        if(mute == 1)
        {
            bnt.setText("not mute");
        }
        else
        {
            bnt.setText("mute");
        }
        mMediaPlayer.setPlaybackMute(mute);
    }

    public void OnPcmEncoder(Button bnt, String pcmurl)
    {
        System.out.println("OnPcmEncoder");
        if(encoder == null)
        {
            bnt.setText("Stop Pcm encoder");
            /*encoder = new PCMEncoder("/storage/emulated/0/tmp/test.pcm", new datadeliver() {
                public int DeliverData(final int mt, final byte[] data, final long pts) {
                    handler.post(new Runnable() {
                        @Override
                        public void run() {
                            if(Looper.myLooper() == Looper.getMainLooper())
                            {
                                System.out.print("main thread");
                            }
                            mMediaPlayer.DeliverPacket(mt, data, pts);
                        }
                    }

                    );

                    return 0;
                }

                public void showtips(final String tips) {
                    handler.post(new Runnable() {
                                     @Override
                                     public void run() {
                                         if (Looper.myLooper() == Looper.getMainLooper()) {
                                             System.out.print("main thread");
                                         }
                                         AlertDialog.Builder builder = new AlertDialog.Builder(this);
                                         builder.setTitle("tips");
                                         builder.setMessage(tips);
                                         builder.setPositiveButton("确定",
                                                 new DialogInterface.OnClickListener() {
                                                     @Override
                                                     public void onClick(DialogInterface dialogInterface, int i) {

                                                     }
                                                 });
                                         AlertDialog dialog = builder.create();
                                         dialog.show();
                                     }

                                     //mMediaPlayer.DeliverPacket(mt, data, pts);
                                 }
                    );

                }
            });*/
            String galleryPath = Environment.getExternalStorageDirectory()
                    + File.separator + Environment.DIRECTORY_DCIM
                    +File.separator+"Camera"+File.separator;
            /*SimpleDateFormat dateFormat = new SimpleDateFormat("yyyyMMdd-HHmmss");
            long curtime = System.currentTimeMillis();
            Date date = new Date(curtime);
            String galleryPath = Environment.getExternalStorageDirectory()
                    + File.separator + Environment.DIRECTORY_DCIM
                    +File.separator+"Camera"+File.separator;
            String aacpath = galleryPath + dateFormat.format(date)+".aac";*/
            encoder = new PCMEncoder(bnt, pcmurl, galleryPath);
            encoder.start();
        }
        else
        {
            bnt.setText("pcm encoder");
            encoder.stopEncoder();
            encoder = null;
        }


        //mMediaPlayer.setPlaybackMute(mute);
    }

    public void OnStartPlay(Button bnt, String playurl)
    {
        if (mMediaPlayer != null) {
            System.out.print("before mMediaPlayer.stop()");
            mMediaPlayer.stop();
            System.out.print("after mMediaPlayer.stop()");
            //mMediaPlayer.release();
            //System.out.print("after mMediaPlayer.release()");
            /*mMediaPlayer = null;
            if (mHudViewHolder != null)
                mHudViewHolder.setMediaPlayer(null);
            mCurrentState = STATE_IDLE;
            mTargetState = STATE_IDLE;
            System.out.print("after mHudViewHolder.setMediaPlayer");*/
            //AudioManager am = (AudioManager) mAppContext.getSystemService(Context.AUDIO_SERVICE);
            //am.abandonAudioFocus(null);
            //AudioManager am = (AudioManager) mAppContext.getSystemService(Context.AUDIO_SERVICE);
            //am.requestAudioFocus(null, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
        }
        //stopPlayback();
        setRender(RENDER_SURFACE_VIEW);
        setVideoPath(playurl);
        start();
    }

    public void OnEchoCancel(Button bnt, int delay)
    {
        String startec = "start echocancel";
        String text = bnt.getText().toString();
        if(text.equals(startec))
        {
            if(null != demuxer)
            {
                demuxer.startEchoCancel(delay);
            }
            bnt.setText("stop echocancel");
        }
        else
        {
            if(null != demuxer)
            {
                demuxer.stopEchoCancel();
            }
            bnt.setText(startec);
        }
    }
}

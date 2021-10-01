package tv.danmaku.ijk.media.player;

import android.media.AudioFormat;


/*import com.leachchen.commongroup.Utils.HandThread.ThreadPoolManager;
import com.leachchen.commongroup.Utils.LogWrite.LogModel;
import com.leachchen.commongroup.Utils.LogWrite.LogWrite;
import com.leachchen.provider.IPC.Device.Command.HandData;
import com.leachchen.provider.IPC.Device.Command.SendCommand;
import com.p2p.pppp_api.PPCS_APIs;*/
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.media.audiofx.AcousticEchoCanceler;
import android.util.Log;

import tv.danmaku.ijk.media.player.IMediaPlayer;
import tv.danmaku.ijk.media.player.IjkMediaPlayer;
//import AcousticEchoCanceler;
//import android.media.audiofx.AcousticEchoCanceler;
/**
 * ClassName:   TalkHelp.java
 * Description:
 * Author :     leach.chen
 * Date:        2017/7/14 16:37
 **/

public class TalkHelp implements DAudioRecord.OnAudioRecordCallback {

    //    private G711Coder mG711Coder;
//    private G711Coder.CoderResult mDecodeRes;
//    private G711Coder.CoderResult mEncodeRes;
    private DAudioRecord mAudioRecord;
    private int mSession;
    private int framenum;
    private int channel;
    private int frequency;
    private int format;
    private long enchandle;
    private int sampbit;
    private IMediaPlayer player = null;
    private AudioRecord mSDRecord = null;
    private byte[] sddata;
    private AcousticEchoCanceler aechdl = null;


    public TalkHelp(IMediaPlayer ownerPlayer) {
//        mG711Coder = new G711Coder();
//        mG711Coder.g711CoderInit();
//        mDecodeRes = new G711Coder.CoderResult();
//        mEncodeRes = new G711Coder.CoderResult();

        player = ownerPlayer;
        frequency = 16000;
        channel = AudioFormat.CHANNEL_IN_MONO;
        //sampbit = 2;
        sampbit = AudioFormat.ENCODING_PCM_16BIT;
        format = 1;

        int minBufferSize = AudioRecord.getMinBufferSize(frequency, channel,
                sampbit);
        /*mSDRecord = new AudioRecord(MediaRecorder.AudioSource.REMOTE_SUBMIX,
                frequency, channel, sampbit, minBufferSize * 2);
        mSDRecord.getAudioSessionId();*/
        sddata = new byte[minBufferSize];


    }

    public int startRecord(int session, int delay_ms) {
        setFramenum(0);
        mSession = session;
        String ms = android.os.Build.BRAND;
        if(null != aechdl)
        {
            aechdl.setEnabled(true);
        }
        if(AcousticEchoCanceler.isAvailable())
        {
            try
            {
                int sid = player.getAudioSessionId();
                aechdl = AcousticEchoCanceler.create(sid);
                if(AcousticEchoCanceler.isAvailable())
                {
                    Log.d("echocancel", "AEC is available");
                }
                else
                {
                    Log.d("echocancel", "AEC is not available");
                }
            }
            catch(Exception e)
            {
                Log.d("echocancel", e.toString());
                //LogWrite.writeMsg(e);
            }
        }
        //player.nativeStartEchoCancel(int channezzz cl, int samplerate, int sampleformat, int nbsamples, int usdelay)
        else if(null != player) {
            // retmi note3 -430000, huawei -280
            player.StartEchoCancel(1, frequency, format, 160, delay_ms * 1000);
        }
        enchandle = IjkMediaPlayer.nativeEncoderOpen(1, 86018, 1, frequency, format, 64);
        mAudioRecord = new DAudioRecord(frequency, channel, sampbit, this);
        int ret = mAudioRecord.startRecord() ? 1 : -1;
        if(0 != ret)
        {
            return -1;
        }

        if(null != mSDRecord) {
            mSDRecord.startRecording();
        }

        return ret;
    }

    public void stopRecord() {
        if(null != mAudioRecord) {
            mAudioRecord.stopRecord();
        }
        setFramenum(0);
        if(null != player) {
            player.StopEchoCancel();
        }

        if(null != mSDRecord) {
            mSDRecord.stop();
            mAudioRecord = null;
        }
        if(0 == enchandle) {
            IjkMediaPlayer.nativeEncoderClose(enchandle);
        }
        enchandle = 0;
    }

    public void release() {
        mAudioRecord.release();
    }

    public void setFramenum(int framenum) {
        this.framenum = framenum;
    }

    @Override
    public void onAudioRecordData(final byte[] dataArr, final int len) {
        //ThreadPoolManager.getInstance().execute(new Runnable() {
            //@Override
            //public void run() {
                byte[] pcmdata = null;
                int sdread_len = 0;
                Log.d("echocancel", "enter pcmdata = player.EchoCancelDeliverData(dataArr)");
                try {
                    if(null != aechdl)
                    {
                        //sdread_len = mSDRecord.read(sddata, 0, len);
                        pcmdata = player.EchoCancelDeliverData(dataArr);//player.EchoCancelDeliverDataEx(dataArr, sddata);
                        Log.d("echocancel", "pcmdata = player.EchoCancelDeliverData(dataArr):" + dataArr.length + "pcmdata: " + pcmdata.length);
                    }
                    else if(null != player)
                    {
                        Log.d("echocancel", "before pcmdata = player.EchoCancelDeliverData(dataArr)");
                        pcmdata = player.EchoCancelDeliverData(dataArr);
                        Log.d("echocancel", "after pcmdata = player.EchoCancelDeliverData(dataArr):" + dataArr.length + "pcmdata: " + pcmdata.length);
                    }
//                    mG711Coder.g711aEncode(dataArr, len, mEncodeRes);
                    /*if(null != player) {
                        pcmdata = player.EchoCancelDeliverDataEx(dataArr, sddata);
                    }*/
                    if(null == pcmdata)
                    {
                        pcmdata = dataArr;
                    }
                    Log.d("echocancel", "before IjkMediaPlayer.nativeEncoderDeliverData");
                    byte[] aacdata = IjkMediaPlayer.nativeEncoderDeliverData(enchandle, pcmdata, 0);
                    Log.d("echocancel", "after IjkMediaPlayer.nativeEncoderDeliverData");
                    //byte[] g711 = CMG711.encode(pcmdata);
                    //byte[] pac = HandData.getTalkAudio(g711, framenum++);//wait
                    if(null != aacdata)
                    {
                        ///byte[] pac = HandData.getTalkAudio(aacdata, framenum++);//wait
                        //PPCS_APIs.PPCS_Write(mSession, (byte) SendCommand.STREAM_VIDEO_CHANNEL, pac, pac.length);
                        //LogWrite.d("send speaker data length = " + pac.length + "   framenum = " + framenum, LogModel.MODEL_VIDEO);
                    }

                } catch (Exception e) {
                    System.out.print(e.toString());
                    //Log.d("ijkmedia", e);
                }
        Log.d("echocancel", "pcmdata = player.EchoCancelDeliverData(dataArr) end");
            //}
        //});
    }
}

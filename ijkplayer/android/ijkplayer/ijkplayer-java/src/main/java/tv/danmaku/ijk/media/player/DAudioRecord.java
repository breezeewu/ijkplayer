package tv.danmaku.ijk.media.player;

import android.media.AudioRecord;
import android.media.MediaRecorder;


public class DAudioRecord implements Runnable {
    private int mFrequency;//采样率
    private int mChannel;//声道
    private int mSampBit;//采样精度
    private AudioRecord mAudioRecord;
    private OnAudioRecordCallback mCallback;
    private boolean recordTag;
    private int minBufferSize;
    private Thread mThread;

    public DAudioRecord(int frequency, int channel, int sampbit, OnAudioRecordCallback callback) {
        this.mFrequency = frequency;
        this.mChannel = channel;
        this.mSampBit = sampbit;
        this.mCallback = callback;
        minBufferSize = AudioRecord.getMinBufferSize(mFrequency, mChannel, mSampBit);
        /**
         * AudioSource.VOICE_CALL 				打电话话筒的声音
         * AudioSource.MIC        				MIC 麦克风声音采集
         * AudioSource.VOICE_COMMUNICATION  	电话通讯,能过进行回音的消除功能
         * AudioSource.CAMCORDER				声音加工,具有方向性???
         */
        mAudioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, mFrequency, mChannel, mSampBit, minBufferSize);
        int status = mAudioRecord.getState();
        //mAudioRecord.startRecording();

    }

    public boolean startRecord() {
        if (mAudioRecord == null) {
            return false;
        }
        //LogWrite.d("TalkHelp startRecord state = " + mAudioRecord.getState() + "   RecordingState=" + mAudioRecord.getRecordingState(), LogModel.MODEL_VIDEO);
        int status = mAudioRecord.getState();
        if (mAudioRecord.getState() == AudioRecord.STATE_INITIALIZED && mAudioRecord.getRecordingState() == AudioRecord.RECORDSTATE_STOPPED) {
            if (mThread != null) {
                this.recordTag = false;
                destroyThread();
            }
            mAudioRecord.startRecording();
            try {
                Thread.sleep(50);
                byte[] audioData = new byte[640];
                int dataLength = 0;
                int size = mAudioRecord.read(audioData, 0, audioData.length);
                dataLength = size;
            }catch (Exception e)
            {

            }
            this.recordTag = true;
            mThread = new Thread(this);
            mThread.start();
            //mThread.resume();
            //mThread.run();
            Thread.State state = mThread.getState();
            try {
                Thread.sleep(100);
                state = mThread.getState();
                status = 0;
            }catch (Exception e)
            {

            }
            return true;
        }
        return false;
    }

    public boolean stopRecord() {
        if (mAudioRecord == null) {
            return false;
        }
        //LogWrite.d("TalkHelp stopRecord state = " + mAudioRecord.getState() + "   RecordingState=" + mAudioRecord.getRecordingState(), LogModel.MODEL_VIDEO);
        if (mAudioRecord.getState() == AudioRecord.STATE_INITIALIZED && mAudioRecord.getRecordingState() == AudioRecord.RECORDSTATE_RECORDING) {
            this.recordTag = false;
            mAudioRecord.stop();
            destroyThread();
            return true;
        }
        return false;
    }

    /**
     * 销毁线程方法
     */
    private void destroyThread() {
        try {
            android.util.Log.d("dawson", "destroyThread");
            if (null != mThread && Thread.State.RUNNABLE == mThread.getState()) {
                try {
                    Thread.sleep(50);
                    mThread.interrupt();
                } catch (Exception e) {
                    mThread = null;
                }
            }
            mThread = null;
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            mThread = null;
        }
    }

    public void release() {
        this.recordTag = false;
        destroyThread();
        if (mAudioRecord != null) {
            try {
                if (mAudioRecord.getState() == AudioRecord.STATE_INITIALIZED) {
                    mAudioRecord.stop();
                    mAudioRecord.release();
                    mAudioRecord = null;
                }
            } catch (Exception e) {
                //LogWrite.e("TalkHelp 关闭录音出错:" + toString(), LogModel.MODEL_VIDEO);
                //LogWrite.writeMsg(e);
            } finally {
                mAudioRecord = null;
            }
        }
    }

//	public int getBit(int maxInputBytes){
//		if(maxInputBytes <= minBufferSize){
//			return 1;
//		}else{
//			int d = maxInputBytes / minBufferSize;
//			for(int i = d; true; i++){
//				int m = maxInputBytes % i;
//				if(m == 0){
//					return i;
//				}
//			}
//		}
//	}

    @Override
    public void run() {
        try {
            byte[] audioData = new byte[640];
            int dataLength = 0;
            android.util.Log.d("dawson", "thread running ...");
            while (recordTag && mAudioRecord != null) {
                int size = mAudioRecord.read(audioData, 0, audioData.length);
                android.util.Log.d("dawson", size + "mAudioRecord.read");
                dataLength += size;
                if (dataLength > 5120 && size >= AudioRecord.SUCCESS) {
                    //LogWrite.d("TalkHelp recordSize======" + size, LogModel.MODEL_VIDEO);
                    if (mCallback != null) {
                        mCallback.onAudioRecordData(audioData, size);
                    }
                }
            }
        } catch (Exception e) {
            //LogWrite.e("TalkHelp record read error:" + toString(), LogModel.MODEL_VIDEO);
            //LogWrite.writeMsg(e);
            android.util.Log.d("dawson","thread running", e);
            release();
        }
    }

    public interface OnAudioRecordCallback {
        void onAudioRecordData(byte[] dataArr, int len);
    }
}

package tv.danmaku.ijk.media.recorder;
import tv.danmaku.ijk.media.player.IjkMediaPlayer;
import android.util.Log;

import java.lang.ref.WeakReference;

public final class VavaRecorder {
        public static final int vava_record_msg_start          = 2001;
        public static final int vava_record_msg_progress       = 2002;
        public static final int vava_record_msg_complete       = 2003;
        public static final int vava_record_msg_error          = 2004;
        public static final int vava_record_msg_interupt       = 2005;

        public static final int VAVA_CODEC_ID_H264             = 28;
        public static final int VAVA_CODEC_ID_H265             = 174;
        public static final int VAVA_CODEC_ID_AAC              = 86018;
        public VavaRecorder()
        {
            // load library and init native
            IjkMediaPlayer player = new IjkMediaPlayer();

        }

        public native int open(String srcurl, String sinkurl, String tmpurl);
        public native int start(long pts);
        public native int deliverPacket(int codec_id, byte[] data, long pts, long frame_num);
        public native void stop(int cancel);
        public native int getPercent();
        public native boolean recordIsEOF();
        public void dump()
        {
            Log.d("ijkmedia", "haha,  recordEventFromNative call success, mNativeMediaRecord:" + mNativeMediaRecord);
        }

        public void nativeEventNotify(int msgid, int wparam, int lparam)
        {
            Log.d("ijkmedia", "msg:" + msgid + ", wparam:" + wparam + ", lparam:" + lparam + "\n");
            switch(msgid)
            {
                case vava_record_msg_start:
                    break;
                case vava_record_msg_progress:
                    break;
                case vava_record_msg_complete:
                {
                    // 异步通知执行stop
                    //...
                }
                break;
                case vava_record_msg_error:
                    break;
                case vava_record_msg_interupt:
                    break;
                default:
                    break;
            }
        }

        private long mNativeMediaRecord;
}

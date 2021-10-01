package tv.danmaku.ijk.media.recorder;
import java.io.DataInputStream;


public interface IRecordDemux {
        public static final int vava_record_msg_start          = 2001;
        public static final int vava_record_msg_progress       = 2002;
        public static final int vava_record_msg_complete       = 2003;
        public static final int vava_record_msg_error          = 2004;
        public static final int vava_record_msg_interupt       = 2005;

        public static final int VAVA_CODEC_ID_H264             = 28;
        public static final int VAVA_CODEC_ID_H265             = 174;
        public static final int VAVA_CODEC_ID_AAC              = 86018;
        //public DataInputStream m_din;

        public class RecordHeader
        {
            public Byte tag;
            public Byte ver;
            public Byte venc;
            public Byte aenc;

            public Byte res;
            public Byte fps;
            public Byte enctype;
            public Byte alarmtype;

            public int reserve1;

            public short vframe;
            public short sample;

            public int size;
            public int time;
            public long reserve2;
        }

        public class RecordPacket
        {
            public int tag;
            public int size;
            public int type;
            public int fps;
            public int framenum;
            public long timestamp;
            public byte[]  data;
            // customer flag;
            public int codec_id;
            public int keyflag;


        }

        public class record_packet
        {
          public byte[] data = null;
          public long pts = -1;
          public int codec_id = -1;
          public int keyflag = 0;
          public int frame_num = 0;
        };

        public int open(String srcurl);

        public RecordPacket read_packet();

        public void close();
}

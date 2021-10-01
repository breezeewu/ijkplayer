package tv.danmaku.ijk.media.recorder;
import android.util.Log;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import tv.danmaku.ijk.media.recorder.IRecordDemux;

import java.io.RandomAccessFile;
import java.lang.ref.WeakReference;

public class VavaRecordDemux implements IRecordDemux {

    public final int rec_type_hs003 = 3;
    public final int rec_type_hs004 = 4;
    public final int rec_type_hs005 = 5;
    public final int HS_FRAME_SYNC_TAG = 0xEB0000AA;
    protected RandomAccessFile m_reader;
    public RecordHeader m_rechdr;
    public int vframenum;
    public int aframenum;

    public int rec_type;
    public long readByte(int nbytes)
    {
        if(null != m_reader)
        {
            try {
                int i = 0;
                long value = 0;
                byte bval[] = new byte[nbytes];
                for(i = 0; i < nbytes; i++)
                {
                    bval[nbytes - i - 1] = m_reader.readByte();
                }

                /*bval[2] = m_reader.readByte();
                bval[1] = m_reader.readByte();
                bval[0] = m_reader.readByte();*/
                for(i = 0; i < nbytes; i++)
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

    @Override
    public int open(String url)
    {
        if(0 == on_read_hs003_header(url))
        {
            rec_type = rec_type_hs003;
            return 0;
        }
        else if(0 == on_read_hs004_header(url))
        {
            rec_type = rec_type_hs004;
            return 0;
        }

        return -1;
    }


    public RecordPacket read_packet()
    {
        if(rec_type_hs003 == rec_type)
        {
            return on_read_hs003_packet();
        }
        else if(rec_type_hs004 == rec_type)
        {
            return on_read_hs004_packet();
        }

        return null;
    }

    @Override
    public void close()
    {
        if(null == m_reader)
        {
            return ;
        }
        try {
            m_reader.close();
        }
        catch(IOException e)
        {
            System.out.print(e.toString());
        }
    }

    // on read record header func
    public int on_read_hs003_header(String url)
    {
        try {
            final int HS003_HEADER_SIZE = 16;
            m_reader = new RandomAccessFile(new File(url), "r");
            m_reader.skipBytes(HS003_HEADER_SIZE);
            int sync_tag = (int)readByte(4);
            if(HS_FRAME_SYNC_TAG != sync_tag)
            {
                m_reader.close();
                return -1;
            }
            m_reader.seek(0);
            m_rechdr = new RecordHeader();
            m_rechdr.tag = m_reader.readByte();
            m_rechdr.venc = m_reader.readByte();
            m_rechdr.aenc = m_reader.readByte();
            m_rechdr.res = m_reader.readByte();
            m_rechdr.fps = m_reader.readByte();
            m_rechdr.enctype = m_reader.readByte();
            m_rechdr.vframe = (short)readByte(2);
            m_rechdr.size = (int)readByte(4);
            m_rechdr.time = (int)readByte(4);
            return 0;
        }
        catch(IOException e)
        {
            System.out.print(e.toString());
        }

        return -1;
    }

    public int on_read_hs004_header(String url)
    {
        try {
            final int HS004_HEADER_SIZE = 32;
            m_reader = new RandomAccessFile(new File(url), "r");
            //m_reader = new DataInputStream(new FileInputStream(url));
            m_reader.skipBytes(HS004_HEADER_SIZE);
            int sync_tag = (int)readByte(4);
            if(HS_FRAME_SYNC_TAG != sync_tag)
            {
                m_reader.close();
                return -1;
            }
            //m_reader.close();
            //m_reader = new DataInputStream(new FileInputStream(url));
            //m_reader.skip(0);
            m_reader.seek(0);
            m_rechdr = new RecordHeader();
            m_rechdr.tag = m_reader.readByte();
            m_rechdr.ver = m_reader.readByte();
            m_rechdr.venc = m_reader.readByte();
            m_rechdr.aenc = m_reader.readByte();
            m_rechdr.res = m_reader.readByte();
            m_rechdr.fps = m_reader.readByte();
            m_rechdr.enctype = m_reader.readByte();
            m_rechdr.alarmtype = m_reader.readByte();
            m_rechdr.reserve1 = (int)readByte(4);
            m_rechdr.vframe = (short)readByte(2);
            m_rechdr.sample = (short)readByte(2);
            m_rechdr.size = (int)readByte(4);
            m_rechdr.time = (int)readByte(4);
            m_rechdr.reserve2 = readByte(8);
            return 0;
        }
        catch(IOException e)
        {
            System.out.print(e.toString());
        }

        return -1;
    }

    // read record packet
    public RecordPacket on_read_hs003_packet()
    {
        try {
            RecordPacket pkt = new RecordPacket();
            pkt.tag = (int)readByte(4);
            if(HS_FRAME_SYNC_TAG != pkt.tag)
            {
                return null;
            }
            pkt.size = (int)readByte(4);
            pkt.type = (int)readByte(4);
            pkt.fps = (int)readByte(4);
            long time_sec = (int)readByte(4);
            long time_usec = (int)readByte(4);
            pkt.timestamp = time_sec * 1000 + time_usec/1000;
            pkt.data = new byte[pkt.size];

            m_reader.read(pkt.data);
            if(8 == pkt.type)
            {
                pkt.codec_id = VAVA_CODEC_ID_AAC;
                pkt.framenum = aframenum++;
            }
            else
            {
                pkt.codec_id = (m_rechdr.venc == 0) ? VAVA_CODEC_ID_H264 : VAVA_CODEC_ID_H265;
                pkt.framenum = vframenum++;
            }
            pkt.keyflag = pkt.type == 3 ? 1 : pkt.type;
            return pkt;
        }
        catch(IOException e)
        {
            System.out.print(e.toString());
        }

        return null;
    }

    public RecordPacket on_read_hs004_packet()
    {
        try {
            RecordPacket pkt = new RecordPacket();
            pkt.tag = (int)readByte(4);
            if(HS_FRAME_SYNC_TAG != pkt.tag)
            {
                return null;
            }
            //Log.d("ijkmedia", "begin:"+ begin + " curpos:" + end);
            pkt.size = (int)readByte(4);
            pkt.type = (int)readByte(4);
            pkt.fps = (int)readByte(4);
            pkt.framenum = (int)readByte(4);
            int reserve1 = (int)readByte(4);
            pkt.timestamp = readByte(8);
            long reserve2 = readByte(8);
            //pkt.timestamp = time_sec * 1000 + time_usec/1000;
            pkt.data = new byte[pkt.size];
            m_reader.read(pkt.data);
            if(8 == pkt.type)
            {
                pkt.codec_id = VAVA_CODEC_ID_AAC;
            }
            else
            {
                pkt.codec_id = (m_rechdr.venc == 0) ? VAVA_CODEC_ID_H264 : VAVA_CODEC_ID_H265;
            }
            pkt.keyflag = pkt.type == 3 ? 1 : pkt.type;
            return pkt;
        }
        catch(IOException e)
        {
            System.out.print(e.toString());
        }

        return null;
    }

}

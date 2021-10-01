#include <stdlib.h>
#include <string.h>
#include "ipc_record.h"


const uint32_t ipc_packet_sync_tag = 0xEB0000AA;

// ipc hs003 record  header, 16 bytes
typedef struct
{
    uint8_t     tag;    // 0:create record, 1:complete record
    uint8_t     venc;   // video encode codec id, 0:h264, 1:h265
    uint8_t     aenc;   // audio encode codec id, 3:aac
    uint8_t     res;    // resolution of video
    uint8_t     fps;    // frame rate
    uint8_t     encrypt;// encrypt flag
    uint16_t    vframe; // video frame number
    uint32_t    size;   // ipc record size
    uint32_t    time;   // ipc record duration
} ipc_hs003_record_header;

// ipc hs003 packet header, 24 bytes
typedef struct
{
    uint32_t    sync; // sync 0xEB0000AA
    uint32_t    size; // packet size
    uint32_t    type; // packet type: 0, P frame, 1: I frame, 8: audio frame
    uint32_t    fps;  // frame rate
    uint32_t    sec;  // timestamp in second
    uint32_t    msec; // timestamp in microsecond
} ipc_hs003_packet_header;

// ipc hs004 record header, 32 bytes
typedef struct
{
    uint8_t     tag;    // 0: create record, 1:complete record
    uint8_t     ver;    // version number, default:0
    uint8_t     venc;   // video codec id, 0:h264, 1:h265
    uint8_t     aenc;   // audio codec id, 3:aac
    uint8_t     res;    // resolution of video packet
    uint8_t     fps;    // frame rate
    uint8_t     encrypt;// encrypt type, 0: no encrypt, 1:aes encrpyt
    uint8_t     alarm_type; // alarm type
    uint8_t     reserve1[4]; // reserve 1
    uint16_t    vframe; // video frame number
    uint16_t    samplerate; // audio samplerate
    uint32_t    size;   // record size
    uint32_t    time;   // record time
    uint32_t    reserve2[2]; //reserve 2
} ipc_hs004_record_header;

// ipc hs004 packet header, 40 bytes
typedef struct
{
    uint32_t    sync;   // sync 0xEB0000AA
    uint32_t    size;   // packet size
    uint32_t    type;   // packet type, 0: P frame, 1: I frame, 8:aac packet
    uint32_t    fps;    // video frame rate
    uint32_t    seq;    // packet sequence number
    uint32_t    reserve;
    uint64_t    pts;    // timestamp in microsecond(ms)
    uint32_t    reserve2[2];
} ipc_hs004_packet_header;

typedef void* (*open_demux)(const char* path);

typedef int (*close_demux)(void** ppdc);

typedef int(*read_packet)(void* prc, ipc_record_packt* pkt, void* pdata, int data_len);

typedef void* (*open_muxer)(const char* path, int rec_type, int alarm_type);
//typedef int ipc_record_muxer_write_packet(void* phandle, ipc_record_packt* pkt, void* pdata, int data_len);
typedef int (*write_packet)(void* prc, int codec_id, char* pdata, int data_len, int64_t pts, int key_flag);

typedef void (*close_muxer)(void** ppdc);
typedef struct
{
    uint8_t     tag;    // 0: create record, 1:complete record
    uint8_t     ver;    // version
    uint8_t     venc;   // video codec id, 0:h264, 1:h265
    uint8_t     aenc;   // audio codec id, 3:aac
    uint8_t     res;    // resolution of video packet
    uint8_t     fps;    // frame rate
    uint8_t     encrypt;// encrypt type, 0: no encrypt, 1:aes encrpyt
    uint8_t     alarm_type; // alarm type
    uint16_t    samplerate; // audio samplerate
    uint32_t    size;       // record payload size
    uint32_t    time;       // ipc record duration
    uint16_t    vframe_num;       // video packet number
    uint16_t    record_type; // record type, 3: hs003, 4: hs004
    uint16_t    vseq_num;       // audio sequence number
    uint16_t    aseq_num;       // audio sequence number
    // file handle
    void*       priv;
    
    FILE*       pfile;
    
    // demux function
    open_demux      pdemux_open_func;
    close_demux     pdemux_close_func;
    read_packet     pdemux_read_func;
    write_packet    pmuxer_write_func;
    close_muxer     pmuxer_close_func;
} ipc_record_context;

ipc_record_context* ipc_hs003_record_open(ipc_record_context* prc, const char* path)
{
    ipc_hs003_record_header hdr;
    if(NULL == prc && path)
    {
        prc = (ipc_record_context*)malloc(sizeof(ipc_record_context));
        memset(prc, 0, sizeof(ipc_record_context));
        prc->pfile = fopen(path, "rb");
    }
    if(NULL == prc || NULL == prc->pfile)
    {
        lberror("open ipc record file failed, invalid param prc:%p or path:%s\n", prc, path);
        return NULL;
    }
    /*if(prc == NULL || NULL == prc->pfile)
    {
        lberror("open ipc record file failed, invalid paramter, prc:%p, prc->pfile:%p\n", prc, prc ? prc->pfile : NULL);
        return NULL;
    }*/
    size_t read_len = fread(&hdr, 1, sizeof(hdr), prc->pfile);
    if(read_len < sizeof(hdr))
    {
        lberror("read_len:%lu = fread(&hdr, 1, sizeof(hdr):%lu, prc->pfile:%p)\n", read_len, sizeof(hdr), prc->pfile);
        free((void*)prc);
        prc = NULL;
        return prc;
    }
    if(NULL == prc)
    {
        prc = (ipc_record_context*)malloc(sizeof(ipc_record_context));
    }
    //prc = (ipc_record_context*)malloc(sizeof(ipc_record_context));
    prc->tag = hdr.tag;
    prc->ver = 0;
    prc->venc = hdr.venc;
    prc->aenc = hdr.aenc;
    prc->res = hdr.res;
    prc->fps = hdr.fps;
    prc->encrypt = hdr.encrypt;
    prc->size = hdr.size;
    prc->time = hdr.time;
    prc->record_type = eipc_record_type_hs003;
    prc->vframe_num = hdr.vframe;
    lbtrace("open ipc hs003 record success, prc->venc:%d, prc->aenc:%d prc->time:%d, prc->size:%d, prc->vframe_num:%d\n", prc->venc, prc->aenc, prc->time, prc->size, prc->vframe_num);
    return prc;
}

int ipc_hs003_demux_read_packet(ipc_record_context* prc, ipc_record_packt* pkt, void* pdata, int data_len)
{
    ipc_hs003_packet_header header;
    size_t read_len = 0;
    if(NULL == prc || NULL == pkt || NULL == prc->pfile)
    {
        lberror("read ipc record faild, invalid parameter, prc:%p, pkt:%p, prc->pfile:%p\n", prc, pkt, prc ? prc->pfile : NULL);
        return -1;
    }

    read_len = fread(&header, 1, sizeof(header), prc->pfile);
    if(read_len < 0)
    {
        lberror("read_len:%lu = fread(&header, 1, sizeof(header):%lu, prc->pfile:%p) failed\n", read_len, sizeof(header), prc->pfile);
    }
    if(header.type < 3)
    {
        pkt->key_flag = header.type;
        pkt->codec_id = prc->venc;
        pkt->packet_num = ++prc->vseq_num;
    }
    else
    {
        pkt->codec_id = prc->aenc;
        pkt->packet_num = ++prc->aseq_num;
    }
    pkt->size = header.size;
    pkt->pts = header.sec * 1000 + header.msec;
    
    if(pdata && data_len < pkt->size)
    {
        lberror("read packet fialed becase not enought buffer, need:%d, have:%d\n", pkt->size, data_len);
        return -1;
    }
    if(NULL == pdata)
    {
        fseek(prc->pfile, pkt->size, SEEK_CUR);
    }
    else
    {
        read_len = fread(pdata, 1, pkt->size, prc->pfile);
        if(read_len < 0)
        {
            lberror("read_len:%lu = fread(pdata:%p, 1, pkt->size:%d, prc->pfile:%p) failed\n", read_len, pdata, pkt->size, prc->pfile);
            return -1;
        }
    }
    return 0;
}

ipc_record_context* ipc_hs004_record_open(ipc_record_context* prc, const char* path)
{
    ipc_hs004_record_header hdr;
    if(NULL == prc && path)
    {
        prc = (ipc_record_context*)malloc(sizeof(ipc_record_context));
        memset(prc, 0, sizeof(ipc_record_context));
        prc->pfile = fopen(path, "rb");
    }
    if(NULL == prc || NULL == prc->pfile)
    {
        lberror("open ipc record file failed, invalid param prc:%p or path:%s\n", prc, path);
        return NULL;
    }
    size_t read_len = fread(&hdr, 1, sizeof(hdr), prc->pfile);
    if(read_len < sizeof(hdr))
    {
        lberror("read_len:%lu = fread(&hdr, 1, sizeof(hdr):%lu, prc->pfile:%p)\n", read_len, sizeof(hdr), prc->pfile);
        free((void*)prc);
        prc = NULL;
        return prc;
    }
    if(NULL == prc)
    {
        prc = (ipc_record_context*)malloc(sizeof(ipc_record_context));
    }
    //prc = (ipc_record_context*)malloc(sizeof(ipc_record_context));
    prc->tag = hdr.tag;
    prc->ver = hdr.ver;
    prc->venc = hdr.venc;
    prc->aenc = hdr.aenc;
    prc->res = hdr.res;
    prc->fps = hdr.fps;
    prc->encrypt = hdr.encrypt;
    prc->size = hdr.size;
    prc->time = hdr.time;
    prc->record_type = eipc_record_type_hs004;
    prc->vframe_num = hdr.vframe;
    lbtrace("open ipc hs003 record success, prc->venc:%d, prc->aenc:%d prc->time:%d, prc->size:%d, prc->vframe_num:%d\n", prc->venc, prc->aenc, prc->time, prc->size, prc->vframe_num);
    return prc;
}

int ipc_hs004_demux_read_packet(ipc_record_context* prc, ipc_record_packt* pkt, void* pdata, int data_len)
{
    ipc_hs004_packet_header header;
    size_t read_len = 0;
    if(NULL == prc || NULL == pkt || NULL == prc->pfile)
    {
        lberror("read ipc record faild, invalid parameter, prc:%p, pkt:%p, prc->pfile:%p\n", prc, pkt, prc ? prc->pfile : NULL);
        return -1;
    }

    read_len = fread(&header, 1, sizeof(header), prc->pfile);
    if(read_len < 0)
    {
        lberror("read_len:%lu = fread(&header, 1, sizeof(header):%lu, prc->pfile:%p) failed\n", read_len, sizeof(header), prc->pfile);
    }
    if(header.type < 3)
    {
        pkt->key_flag = header.type;
        pkt->codec_id = prc->venc;
        pkt->packet_num = ++prc->vseq_num;
    }
    else
    {
        pkt->codec_id = prc->aenc;
        pkt->packet_num = ++prc->aseq_num;
    }
    pkt->size = header.size;
    pkt->pts = header.pts;
    
    if(pdata && data_len < pkt->size)
    {
        lberror("read packet fialed becase not enought buffer, need:%d, have:%d\n", pkt->size, data_len);
        return -1;
    }
    if(NULL == pdata)
    {
        fseek(prc->pfile, pkt->size, SEEK_CUR);
    }
    else
    {
        read_len = fread(pdata, 1, pkt->size, prc->pfile);
        if(read_len < 0)
        {
            lberror("read_len:%lu = fread(pdata:%p, 1, pkt->size:%d, prc->pfile:%p) failed\n", read_len, pdata, pkt->size, prc->pfile);
            return -1;
        }
    }
    return 0;
}

ipc_record_context* create_record_demux_contex(const char* path)
{
    uint32_t sync = 0;
    ipc_record_context* prc = NULL;
    eipc_record_type rec_type = eipc_record_type_unknown;
    FILE* pfile = fopen(path, "rb");
    do
    {
        if(NULL == pfile)
        {
            lberror("open ipc record file failed, invalid path:%s or file\n", path);
            return NULL;
        }
        fseek(pfile, sizeof(ipc_hs003_record_header), SEEK_SET);
        fread(&sync, sizeof(uint32_t), 1, pfile);
        if(ipc_packet_sync_tag == sync)
        {
            rec_type = eipc_record_type_hs003;
            break;
        }
        fseek(pfile, sizeof(ipc_hs004_record_header), SEEK_SET);
        fread(&sync, sizeof(uint32_t), 1, pfile);
        if(ipc_packet_sync_tag == sync)
        {
            rec_type = eipc_record_type_hs004;
            break;
        }
        return NULL;
    }while(0);

    prc = (ipc_record_context*)malloc(sizeof(ipc_record_context));
    prc->record_type = rec_type;
    if(eipc_record_type_hs003 == rec_type)
    {
        prc->pdemux_open_func = ipc_hs003_record_open;
        prc->pdemux_read_func = ipc_hs003_demux_read_packet;
    }
    else if(eipc_record_type_hs004 == rec_type)
    {
        prc->pdemux_open_func = ipc_hs004_record_open;
        prc->pdemux_read_func = ipc_hs004_demux_read_packet;
    }
    else
    {
        lberror("Invalid rec_type:%d, path:%s\n", rec_type, path);
        free(prc);
        fclose(pfile);
        return NULL;
    }
    prc->pfile = pfile;
    return prc;
}

void* ipc_record_demux_open(const char* path)
{
    ipc_record_context* prc = create_record_demux_contex(path);
    if(NULL == prc)
    {
        lberror("create record demux failed, invalid path:%s or file\n", path);
        return NULL;
    }
    
    if(prc->pdemux_open_func)
    {
        return prc->pdemux_open_func(prc);
    }
    
    return NULL;
}

int ipc_record_demux_read_packet(void* phandle, ipc_record_packt* pkt, void* pdata, int data_len)
{
    ipc_record_context* prc = (ipc_record_context*)phandle;
    if(prc && prc->pdemux_read_func)
    {
        return prc->pdemux_read_func(prc, pkt, pdata, data_len);
    }
    lberror("invalid param, prc:%p, prc->pdemux_read_func:%p\n", prc, prc ? prc->pdemux_read_func : NULL);
    return -1;
}

void ipc_record_demux_close(void** pphandle)
{
    ipc_record_context** pprc = (ipc_record_context*)pphandle;
    if(pprc && *pprc)
    {
        ipc_record_context* prc = *pprc;
        if(prc->pfile)
        {
            fclose(prc->pfile);
            prc->pfile = NULL;
        }
        free(prc);
    }
}


void* ipc_hs004_muxer_open(const char* path)
{
    ipc_hs004_record_header* ipr = NULL;
    ipc_record_context* prc = NULL;//(ipc_record_context*)malloc(sizeof(ipc_record_context));
    FILE* pfile = fopen(path, "wb");
    if(NULL == pfile)
    {
        return NULL;
    }
    int rh_len = sizeof(ipc_hs004_record_header);
    prc = (ipc_record_context*)malloc(sizeof(ipc_record_context));
    prc->priv = ipr = (ipc_hs004_record_header*)malloc(sizeof(ipc_hs004_record_header));
    memset(ipr, 0, sizeof(ipc_hs004_record_header));
    prc->pfile = pfile;
    prc->alarm_type = 0;
    prc->aseq_num = 0;
    prc->vseq_num = 0;
    prc->encrypt = 0;
    prc->aenc = 0xff;
    prc->venc = 0xff;
    prc->fps = 15;
    fwrite(ipr, 1, rh_len, pfile);
    return prc;
}


int ipc_hs004_record_write_packet(void* phandle, int codec_id, char* pdata, int data_len, int64_t pts_ms, int key_flag)
{
    ipc_record_context* prc = phandle;
    ipc_hs004_record_header* ipr = NULL;
    ipc_hs004_packet_header hdr;
    memset(&hdr, 0, sizeof(hdr));
    if(NULL == prc || NULL == pdata || data_len <= 0 || NULL == prc->pfile)
    {
        lberror("Invalid parameter, phandle:%p, pdata:%p, data_len:%d, prc->pfile:%p\n", phandle, pdata, data_len, prc ? prc->pfile : NULL);
        return -1;
    }
    ipr = (ipc_hs004_record_header*)prc->priv;
    hdr.sync = ipc_packet_sync_tag;
    hdr.fps = 15;
    hdr.pts = pts_ms;
    if(codec_id <= eipc_codec_id_mp3)
    {
        hdr.seq = prc->vseq_num++;
        hdr.type = key_flag;
        if(prc->venc == 0xff)
        {
            prc->venc = codec_id;
        }
    }
    else
    {
        hdr.seq = prc->aseq_num++;
        hdr.type = 8;
        if(prc->aenc == 0xff)
        {
            prc->aenc = codec_id;
        }
    }
    hdr.size = data_len;
    prc->time = (int)hdr.pts;
    prc->size += data_len;
    int hdr_len = sizeof(hdr);
    fwrite(&hdr, 1, sizeof(hdr), prc->pfile);
    fwrite(pdata, 1, data_len, prc->pfile);
    return 0;
}

void ipc_hs004_record_close_muxer(void** pphandle)
{
    ipc_record_context** pprc = pphandle;
    ipc_record_context* prc = NULL;
    if(pprc && *pprc)
    {
        prc = *pprc;
        ipc_hs004_record_header* ihr = (ipc_hs004_record_header*)prc->priv;
        ihr->tag = 1;
        ihr->ver = 0;
        ihr->res = 10;
        ihr->venc = prc->venc;
        ihr->aenc = prc->aenc;
        ihr->fps = prc->fps;
        ihr->encrypt = 0;
        ihr->alarm_type = 0;
        ihr->vframe = prc->vseq_num;
        ihr->samplerate = 16000;//prc->samplerate;
        ihr->size = prc->size;
        ihr->time = prc->time;
        fseek(prc->pfile, SEEK_SET, 0);
        fwrite(ihr, sizeof(ipc_hs004_record_header), 1, prc->pfile);
        fclose(prc->pfile);
        prc->pfile = NULL;
        free(ihr);
        free(prc);
        *pphandle = NULL;
    }
}
void* ipc_record_muxer_open(const char* path, int rec_type)
{
    if(eipc_record_type_hs004 == rec_type)
    {
        ipc_record_context* prc = ipc_hs004_muxer_open(path);
        if(NULL == prc)
        {
            lberror("prc:%p = ipc_hs004_muxer_open(path:%p) failed\n", prc, path);
            return NULL;
        }
        prc->pmuxer_write_func = ipc_hs004_record_write_packet;
        prc->pmuxer_close_func = ipc_hs004_record_close_muxer;
        return prc;
    }
    
    return NULL;
}
//codec_id, pdata, data_len, key_flag, pts/1000
int ipc_record_muxer_write_packet(void* phandle, int codec_id, char* pdata, int data_len, int64_t pts, int key_flag)
{
    ipc_record_context* prc = (ipc_record_context*)phandle;
    if(prc)
    {
        return prc->pmuxer_write_func(prc, codec_id, pdata, data_len, pts, key_flag);
    }
    
    return -1;
}

void ipc_record_muxer_close(void** pphandle)
{
    ipc_hs004_record_close_muxer(pphandle);
}

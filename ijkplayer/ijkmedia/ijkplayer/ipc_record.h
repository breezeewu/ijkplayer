#ifndef _IPC_RECORD_H_
#define _IPC_RECORD_H_
#include <stdio.h>
#include <stdint.h>
#ifndef lbtrace
#define lbtrace(...)
#endif
#ifndef lberror
#define lberror printf
#endif
#ifndef lbinfo
#define lbinfo(...)
#endif
#ifndef lbmemory
#define lbmemory
#endif
// ipc record codec id
enum eipc_codec_id
{
    eipc_codec_id_none = -1,
    eipc_codec_id_h264 = 0,
    eipc_codec_id_h265 = 1,
    eipc_codec_id_mp3  = 2,
    eipc_codec_id_aac  = 3
};

typedef enum
{
    eipc_record_type_unknown = -1,
    eipc_record_type_hs003 = 3,
    eipc_record_type_hs004 = 4
} eipc_record_type;

typedef struct
{
    
    uint8_t     key_flag;
    uint8_t     codec_id;
    uint16_t    packet_num;
    uint32_t    size;
    int64_t     pts;
} ipc_record_packt;



// ipc record demuxer
void* ipc_record_demux_open(const char* path);

int ipc_record_demux_read_packet(void* phandle, ipc_record_packt* pkt, void* pdata, int data_len);

void ipc_record_demux_close(void** pphandle);


// ipc record muxer
void* ipc_record_muxer_open(const char* path, int rec_type);

int ipc_record_muxer_write_packet(void* phandle, int codec_id, char* pdata, int data_len, int64_t pts, int key_flag);
//int ipc_record_muxer_write_packet(void* phandle, ipc_record_packt* pkt, void* pdata, int data_len);

void ipc_record_muxer_close(void** pphandle);
#endif

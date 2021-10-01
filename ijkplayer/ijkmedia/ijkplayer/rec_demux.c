#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "rec_demux.h"
#include <errno.h>
//#define ENABLE_DECRYPT_KEY_FRAME
#ifdef ENABLE_DECRYPT_KEY_FRAME
#include "aesencrypt.h"
#endif
int to_avcodec_id(int rec_codec_id)
{
	if(rec_codec_id_h264 == rec_codec_id)
	{
		// h264
		return av_codec_id_h264;
	}
	else if(rec_codec_id_h265 == rec_codec_id)
	{
		// h265
		return av_codec_id_h265;
	}
	else if(rec_codec_id_aac == rec_codec_id)
	{
		// aac
		return av_codec_id_aac;
	}

	return -1;
}


int to_rec_codec_id(int avcodec_id)
{
	if(av_codec_id_h264 == avcodec_id)
	{
		// h264
		return rec_codec_id_h264;
	}
	else if(av_codec_id_h265 == avcodec_id)
	{
		// h265
		return rec_codec_id_h265;
	}
	else if(av_codec_id_aac == avcodec_id)
	{
		// aac
		return rec_codec_id_aac;
	}

	return -1;
}
ecamera_record_type vava_hs_get_record_type(const char* purl)
{
	uint32_t sync_tag = 0;
	ecamera_record_type rec_type = ecamera_hs_unknown;
	FILE* pfile = NULL;
	pfile = fopen(purl, "rb");
	if(NULL == pfile)
	{
		lberror("pfile:%p = fopen(purl:%s, rb) failed\n", pfile, purl);
		return rec_type;
	}

	fseek(pfile, sizeof(VAVA_HS003_REC_HDR), SEEK_SET);
	fread(&sync_tag, sizeof(sync_tag), 1, pfile);
	if(FRAME_TAG_SYNC_NUM == sync_tag)
	{
		rec_type = ecamera_hs_003;
	}
	else
	{
		fseek(pfile, sizeof(VAVA_HS004_REC_HDR), SEEK_SET);
		fread(&sync_tag, sizeof(sync_tag), 1, pfile);
		if(FRAME_TAG_SYNC_NUM == sync_tag)
		{
			rec_type = ecamera_hs_004;
		}
		
	}
	fclose(pfile);
	return rec_type;
}

int vava_hs003_open_record(VAVA_HS_REC_CTX* prec_ctx)
{
	uint32_t sync_tag = 0;
	VAVA_HS003_REC_HDR hs3_rec_hdr;
	if(NULL == prec_ctx || NULL == prec_ctx->pfile)
	{
		//lberror("Invalid parameter prec_ctx:%p\n", prec_ctx);
		lberror("Invalid parameter prec_ctx:%p, prec_ctx->pfile:%p\n", prec_ctx, prec_ctx ? prec_ctx->pfile:prec_ctx);
		return -1;
	}

	fseek(prec_ctx->pfile, 0, SEEK_SET);
	int readed = fread(&hs3_rec_hdr, 1, sizeof(hs3_rec_hdr), prec_ctx->pfile);
	if(readed < sizeof(hs3_rec_hdr))
	{
		lberror("read hs003 record header failed, readed:%d\n", readed);
		return NULL;
	}

	readed = fread(&sync_tag, 1, sizeof(sync_tag), prec_ctx->pfile);
	if(readed < sizeof(uint32_t) || FRAME_TAG_SYNC_NUM != sync_tag)
	{
		lberror("read hs003 record header failed, readed:%d, sync_tag:%0x\n", readed, sync_tag);
		return NULL;
	}
	fseek(prec_ctx->pfile, sizeof(VAVA_HS003_REC_HDR), SEEK_SET);
	memset(&prec_ctx->rec_hdr, 0, sizeof(hs3_rec_hdr));
	prec_ctx->rec_hdr.tag = hs3_rec_hdr.tag;
	prec_ctx->rec_hdr.vcodec_id = hs3_rec_hdr.v_encode;
	hs3_rec_hdr.a_encode = (char)(hs3_rec_hdr.a_encode > 0 ? hs3_rec_hdr.a_encode : 3);
	prec_ctx->rec_hdr.acodec_id = hs3_rec_hdr.a_encode;
	prec_ctx->rec_hdr.res = hs3_rec_hdr.res;
	prec_ctx->rec_hdr.encrypt = hs3_rec_hdr.encrypt;
	prec_ctx->rec_hdr.alarm_type = 1;
	prec_ctx->rec_hdr.vframe_num = hs3_rec_hdr.vframe;
	prec_ctx->rec_hdr.sample_rate = 8000;
	prec_ctx->rec_hdr.nrec_size = hs3_rec_hdr.size;
	prec_ctx->rec_hdr.nduration = hs3_rec_hdr.time;
	lbtrace("hs003 record open success, vcodec_id:%d, acodec_id:%d, encrypt:%d, res:%d, alarm_type:%d, samplerate:%d, nduration:%d\n", prec_ctx->rec_hdr.vcodec_id, prec_ctx->rec_hdr.acodec_id, prec_ctx->rec_hdr.encrypt, prec_ctx->rec_hdr.res, prec_ctx->rec_hdr.alarm_type, prec_ctx->rec_hdr.sample_rate, prec_ctx->rec_hdr.nduration);
	return 0;
}

int vava_hs003_seek_record(VAVA_HS_REC_CTX* prec_ctx, int64_t pts)
{
	VAVA_HS003_PACKET_HDR pkthdr;
	int64_t llstart_pts = INT64_MIN;
	int ret = -1;
	if(NULL == prec_ctx || NULL == prec_ctx->pfile)
	{
		lberror("hs004 seek record failed, prec_ctx:%p, prec_ctx->pfile:%p\n", prec_ctx, prec_ctx ? prec_ctx->pfile:prec_ctx);
		return -1;
	}

	fseek(prec_ctx->pfile, sizeof(VAVA_HS003_REC_HDR), SEEK_SET);
	while(pts > 0)
	{
		size_t readlen = fread((void*)&pkthdr, 1, sizeof(VAVA_HS003_PACKET_HDR), prec_ctx->pfile);
		if(readlen < 0)
		{
			lberror("read packet header failed, readlen:%d\n", readlen);
			return -1;
		}
		int64_t cur_pts = pkthdr.time_sec * 1000 + pkthdr.time_usec;
		if(INT64_MIN == llstart_pts && cur_pts >= 0)
		{
			llstart_pts = cur_pts;
		}

		if(cur_pts - llstart_pts >= pts)
		{
			int pos = fseek(prec_ctx->pfile, -sizeof(VAVA_HS003_PACKET_HDR), SEEK_CUR);
			lbtrace("hs003 seek record success, pos:%d, cur_pts:%" PRId64 "\n", pos, cur_pts);
			ret = 0;
		}
		else
		{
			fseek(prec_ctx->pfile, pkthdr.size, SEEK_CUR);
		}
	};

	lbtrace("cur record pos:%d\n", ftell(prec_ctx->pfile));
	if(pts > 0 && ret != 0)
	{
		fseek(prec_ctx->pfile,  sizeof(VAVA_HS003_REC_HDR), SEEK_SET);
		lberror("hs003 record seek packet failed, prec_ctx:%p, pts:%" PRId64 ", pos:%d\n", prec_ctx, pts, ftell(prec_ctx->pfile));
	}
	else
	{
		
		fseek(prec_ctx->pfile,  -sizeof(VAVA_HS003_PACKET_HDR), SEEK_CUR);
		lbtrace("hs003 record seek success, pts:%" PRId64 ", pos:%d\n", pts, ftell(prec_ctx->pfile));
		ret = 0;
	}
	
	return 0;
	lberror("hs003 record seek packet failed, prec_ctx:%p, pts:%" PRId64 "\n", prec_ctx, pts);
	return -1;
}

int vava_hs003_read_packet(VAVA_HS_REC_CTX* prec_ctx, IPC_PACKET_HDR* piph, char* pdata, int len)
{
	VAVA_HS003_PACKET_HDR pkthdr;
	lbinfo("vava_hs003_read_packet(prec_ctx:%p, piph:%p, pdata:%p, len:%d)\n", prec_ctx, piph, pdata, len);
	if(NULL == prec_ctx || NULL == piph)
	{
		lberror("read record packet failed, invalid parameter, prec_ctx:%p, piph:%p\n", prec_ctx, piph);
		return -1;
	}

	if(!prec_ctx->pfile)
	{
		lberror("record not open, please open it first!");
		return -1;
	}

	size_t readlen = fread(&pkthdr, 1, sizeof(VAVA_HS003_PACKET_HDR), prec_ctx->pfile);
	//lbtrace("m_rechead.type:%d, m_rechead.size:%d, pts:%u\n", m_rechead.type, m_rechead.size, m_rechead.time_sec *1000 + m_rechead.time_usec);
	if(readlen < sizeof(VAVA_HS003_PACKET_HDR) || pkthdr.tag != 0xEB0000AA)
	{
		lberror("readlen:%ld < sizeof(VAVA_HS003_PACKET_HDR):%ld, read rechdr failed, tag:%0x", readlen, sizeof(VAVA_HS003_PACKET_HDR), pkthdr.tag);
		return -1;
	}
	if(len < (int)pkthdr.size)
	{
		lberror("len:%d < prechdr->size:%d, data buffer not enough", len, pkthdr.size);
		return -1;
	}
	if(pdata && len >= pkthdr.size)
	{
		readlen = fread(pdata, 1, pkthdr.size, prec_ctx->pfile);
		//printf("before readlen:%d = fread(pdata, 1, prechdr->size:%p, pfile)", readlen, prechdr->size);
		if(readlen < pkthdr.size)
		{
			lberror("readlen:%ld < precframe->size:%d, not enough memory", readlen, pkthdr.size);
			return -1; 
		}
	}
	else if(NULL == pdata)
	{
		fseek(prec_ctx->pfile, SEEK_CUR, pkthdr.size);
	}
	

	if(piph)
	{
        piph->mt = pkthdr.type == 8 ? 1 : 0;
		if(0 ==  piph->mt)
		{
			piph->codec_id = prec_ctx->rec_hdr.vcodec_id;
			piph->keyflag = pkthdr.type;
			piph->frame_num = ++prec_ctx->nvframe_cnt;
		}
		else if(8 == pkthdr.type)
		{
            piph->codec_id = 3;//prec_ctx->rec_hdr.acodec_id;
			piph->keyflag = 1;
			piph->frame_num = ++prec_ctx->naframe_cnt;
		}
		piph->size = pkthdr.size;
		piph->pts = (int64_t)pkthdr.time_sec *1000 + pkthdr.time_usec;
		lbinfo("piph->mt:%d, piph->codec_id:%d, piph->keyflag:%d, piph->size:%d, piph->pts:%" PRId64 ", piph.codec_id:%d\n", piph->mt, piph->codec_id, piph->keyflag, piph->size, piph->pts, piph->codec_id);
	}
	return 0;
}

int vava_hs004_open_record(VAVA_HS_REC_CTX* prec_ctx)
{
	uint32_t sync_tag = 0;
	VAVA_HS004_REC_HDR hs4_rec_hdr;
	if(NULL == prec_ctx || NULL == prec_ctx->pfile)
	{
		//lberror("Invalid parameter prec_ctx:%p\n", prec_ctx);
		lberror("Invalid parameter prec_ctx:%p, prec_ctx->pfile:%p\n", prec_ctx, prec_ctx ? prec_ctx->pfile:prec_ctx);
		return -1;
	}

	fseek(prec_ctx->pfile, 0, SEEK_SET);
	int readed = fread(&hs4_rec_hdr, 1, sizeof(hs4_rec_hdr), prec_ctx->pfile);
	if(readed < sizeof(hs4_rec_hdr))
	{
		lberror("read hs004 record header failed, readed:%d, sizeof(VAVA_HS004_REC_HDR):%ld\n", readed, sizeof(VAVA_HS004_REC_HDR));
		return NULL;
	}
	int pos = ftell(prec_ctx->pfile);
	readed = fread(&sync_tag, 1, sizeof(uint32_t), prec_ctx->pfile);
	if(readed < sizeof(uint32_t) || FRAME_TAG_SYNC_NUM != sync_tag)
	{
		lberror("read hs004 record header failed, readed:%d, sync_tag:%u\n", readed, sync_tag);
		return NULL;
	}
	fseek(prec_ctx->pfile, sizeof(VAVA_HS004_REC_HDR), SEEK_SET);
	prec_ctx->rec_hdr.tag = hs4_rec_hdr.tag;
	prec_ctx->rec_hdr.version = hs4_rec_hdr.ver;
	prec_ctx->rec_hdr.vcodec_id = hs4_rec_hdr.v_encode;
	prec_ctx->rec_hdr.res = hs4_rec_hdr.res;
	prec_ctx->rec_hdr.encrypt = hs4_rec_hdr.encrypt;
	prec_ctx->rec_hdr.alarm_type = hs4_rec_hdr.alarmtype;
	prec_ctx->rec_hdr.vframe_num = hs4_rec_hdr.vframe;
	prec_ctx->rec_hdr.sample_rate = hs4_rec_hdr.sample;
	prec_ctx->rec_hdr.nrec_size = hs4_rec_hdr.size;
	prec_ctx->rec_hdr.nduration = hs4_rec_hdr.time;
	hs4_rec_hdr.a_encode = hs4_rec_hdr.a_encode > 0 ? hs4_rec_hdr.a_encode : 3;
	prec_ctx->rec_hdr.acodec_id = hs4_rec_hdr.a_encode;
	lbtrace("hs004 open success, version:%d, vcodec_id:%d, acodec_id:%d, res:%d, encrypt:%d, alarm_type:%d, samplerate:%d, duration:%d, pos:%d, sync_tag:%0x\n", prec_ctx->rec_hdr.version, prec_ctx->rec_hdr.vcodec_id, prec_ctx->rec_hdr.acodec_id, prec_ctx->rec_hdr.res, prec_ctx->rec_hdr.encrypt, prec_ctx->rec_hdr.alarm_type, prec_ctx->rec_hdr.sample_rate, prec_ctx->rec_hdr.nduration, pos, sync_tag);
	return 0;
}

int vava_hs004_seek_record(VAVA_HS_REC_CTX* prec_ctx, int64_t pts)
{
	VAVA_HS004_PACKET_HDR pkthdr;
	int64_t llstart_pts = INT64_MIN;
	int ret = -1;
	if(NULL == prec_ctx || NULL == prec_ctx->pfile)
	{
		lberror("hs004 seek record failed, prec_ctx:%p, prec_ctx->pfile:%p\n", prec_ctx, prec_ctx ? prec_ctx->pfile:prec_ctx);
		return -1;
	}

	fseek(prec_ctx->pfile, sizeof(VAVA_HS004_REC_HDR), SEEK_SET);
	while(pts > 0)
	{
		size_t readlen = fread((void*)&pkthdr, 1, sizeof(VAVA_HS004_PACKET_HDR), prec_ctx->pfile);
		if(readlen < 0)
		{
			lberror("read packet header failed, readlen:%ld\n", readlen);
			return -1;
		}
		if(INT64_MIN == llstart_pts && pkthdr.ntsamp >= 0)
		{
			llstart_pts = pkthdr.ntsamp;
		}

		if(pkthdr.ntsamp - llstart_pts >= pts)
		{
			int pos = fseek(prec_ctx->pfile, -sizeof(VAVA_HS004_PACKET_HDR), SEEK_CUR);
			lbtrace("pos = fseek(prec_ctx->pfile, -sizeof(VAVA_HS004_PACKET_HDR):%ld, SEEK_CUR)\n", pos, prec_ctx->pfile, -sizeof(VAVA_HS004_PACKET_HDR));
			ret = 0;
			break;
		}
		else
		{
			fseek(prec_ctx->pfile, pkthdr.size, SEEK_CUR);
		}
	};

	lbtrace("cur record pos:%ld\n", ftell(prec_ctx->pfile));
	if(pts > 0 && ret != 0)
	{
		fseek(prec_ctx->pfile,  sizeof(VAVA_HS004_REC_HDR), SEEK_SET);
		lberror("hs004 record seek packet failed, prec_ctx:%p, pts:%" PRId64 ", pos:%ld\n", prec_ctx, pts, ftell(prec_ctx->pfile));
	}
	else
	{
		
		fseek(prec_ctx->pfile,  -sizeof(VAVA_HS004_PACKET_HDR), SEEK_CUR);
		lbtrace("hs004 record seek success, pts:%" PRId64 ", pos:%ld\n", pts, ftell(prec_ctx->pfile));
		ret = 0;
	}
	
	return 0;
}

int vava_hs004_read_packet(VAVA_HS_REC_CTX* prec_ctx, IPC_PACKET_HDR* piph, char* pdata, int len)
{
	VAVA_HS004_PACKET_HDR pkthdr;
	memset(&pkthdr, 0, sizeof(pkthdr));
	lbinfo("vava_hs004_read_packet(prec_ctx:%p, piph:%p, pdata:%p, len:%d)\n", prec_ctx, piph, pdata, len);
	if(NULL == prec_ctx || NULL == piph)
	{
		lberror("read record packet failed, invalid parameter, prec_ctx:%p, piph:%p\n", prec_ctx, piph);
		return -1;
	}

	if(!prec_ctx->pfile)
	{
		lberror("record not open, please open it first!");
		return -1;
	}
	int pos = ftell(prec_ctx->pfile);
    size_t readlen = fread(&pkthdr, 1, sizeof(VAVA_HS004_PACKET_HDR), prec_ctx->pfile);
    lbinfo("tag:%0x, framnum:%d, type:%d, size:%d, pts:%" PRId64 ", pos:%d\n", pkthdr.tag, pkthdr.framnum, pkthdr.type, pkthdr.size, pkthdr.ntsamp, pos);
	if(readlen < sizeof(VAVA_HS004_PACKET_HDR) || pkthdr.tag != 0xEB0000AA)
	{
		lberror("readlen:%ld < sizeof(VAVA_HS004_PACKET_HDR):%ld, read rechdr failed, tag:%0x", readlen, sizeof(VAVA_HS004_PACKET_HDR), pkthdr.tag);
		return -1;
	}
	if(len < (int)pkthdr.size)
	{
		lberror("len:%d < prechdr->size:%d, data buffer not enough", len, pkthdr.size);
		return -1;
	}
	if(pdata && len >= pkthdr.size)
	{
		readlen = fread(pdata, 1, pkthdr.size, prec_ctx->pfile);
		if(readlen < pkthdr.size)
		{
			lberror("readlen:%ld < precframe->size:%d, not enough memory", readlen, pkthdr.size);
			return -1; 
		}
	}
	else if(NULL == pdata)
	{
		fseek(prec_ctx->pfile, SEEK_CUR, pkthdr.size);
	}
	
	if(piph)
	{
		piph->mt = pkthdr.type;
		if(0 ==  piph->mt || 1 == piph->mt)
		{
			piph->codec_id = prec_ctx->rec_hdr.vcodec_id;
			piph->keyflag = pkthdr.type;
			piph->frame_num = ++prec_ctx->nvframe_cnt;
		}
		else if(8 == piph->mt)
		{
            piph->codec_id = 3;//prec_ctx->rec_hdr.acodec_id;
			piph->keyflag = 1;
			piph->frame_num = ++prec_ctx->naframe_cnt;
		}
		piph->size = pkthdr.size;
		piph->pts = (int64_t)pkthdr.ntsamp;
		lbinfo("piph->mt:%d, piph->codec_id:%d, piph->keyflag:%d, piph->size:%d, piph->pts:%" PRId64 ", piph.codec_id:%d\n", piph->mt, piph->codec_id, piph->keyflag, piph->size, piph->pts, piph->codec_id);
	}
	return 0;
}

int vava_hs_seek_packet(VAVA_HS_REC_CTX* prec_ctx, int64_t pts)
{
	if(NULL == prec_ctx || NULL == prec_ctx->pfile)
	{
		lberror("Invalid record contex:%p, prec_ctx->pfile:%p\n", prec_ctx, prec_ctx?prec_ctx->pfile : NULL);
		return -1;
	}

	if(prec_ctx->pseek_func)
	{
		return prec_ctx->pseek_func(prec_ctx, pts);
	}
	/*if(ecamera_hs_003 == prec_ctx->rec_type)
	{
		fseek(prec_ctx->pfile, sizeof(VAVA_HS003_REC_HDR), SEEK_SET);
		//prec_ctx = vava_hs003_open_record(purl);
	}
	else if(ecamera_hs_004 == prec_ctx->rec_type)
	{
		fseek(prec_ctx->pfile, sizeof(VAVA_HS004_REC_HDR), SEEK_SET);
	}
	else
	{
		lberror("Unsporrt record prec_ctx->rec_type:%d\n", prec_ctx->rec_type);
		return NULL;
	}
	while(pts > 0)
	{

	}*/
	return -1;
}

VAVA_HS_REC_CTX* vava_hs_open_record(const char* purl)
{
	int ret = 0;
	VAVA_HS_REC_CTX* prec_ctx = NULL;
	if(NULL == purl)
	{
		lberror("Invalid record url:%s\n", purl);
		return NULL;
	}

	ecamera_record_type rec_type = vava_hs_get_record_type(purl);
	if(ecamera_hs_unknown == rec_type)
	{
		lberror("Invalid record type:%d\n", rec_type);
		return NULL;
	}

	prec_ctx = (VAVA_HS_REC_CTX*)malloc(sizeof(VAVA_HS_REC_CTX));
	memset(prec_ctx, 0, sizeof(VAVA_HS_REC_CTX));
	prec_ctx->rec_type = rec_type;
	prec_ctx->pfile = fopen(purl, "rb");
	if(ecamera_hs_003 == rec_type)
	{
		prec_ctx->popen_func = vava_hs003_open_record;
		prec_ctx->pseek_func = vava_hs003_seek_record;
		prec_ctx->pread_func = vava_hs003_read_packet;
		//prec_ctx = vava_hs003_open_record(purl);
	}
	else if(ecamera_hs_004 == rec_type)
	{
		prec_ctx->popen_func = vava_hs004_open_record;
		prec_ctx->pseek_func = vava_hs004_seek_record;
		prec_ctx->pread_func = vava_hs004_read_packet;
	}
	else
	{
		lberror("Unsporrt record version:%d\n", rec_type);
		return NULL;
	}
	ret = prec_ctx->popen_func(prec_ctx);
	if(ret != 0)
	{
		lberror("ret:%d = prec_ctx->popen_func(prec_ctx:%p) failed\n", ret, prec_ctx);
	}
	else
	{
		lbtrace("ret:%d = prec_ctx->popen_func(prec_ctx:%p) success\n", ret, prec_ctx);
	}
	
	return prec_ctx;
}

int vava_hs_read_packet(VAVA_HS_REC_CTX* prec_ctx, IPC_PACKET_HDR* piph, char* pdata, int len)
{
	int ret = 0;
	if(NULL == prec_ctx || NULL == piph || NULL == pdata || NULL == prec_ctx->pread_func)
	{
		lberror("Invalid parameter prec_ctx:%p, piph:%p, pdata:%p, prec_ctx->pread_func:%p\n", prec_ctx, piph, pdata, prec_ctx ? prec_ctx->pread_func : NULL);
		return -1;
	}

	ret = prec_ctx->pread_func(prec_ctx, piph, pdata, len);

	return ret;
}
const char* find_start_code(const char* pdata, int len, int* pstart_code_size)
{
	int i = 0;
	int start_code_size = 3;
	while ((i < len - 3) && (0 != pdata[i] || 0 != pdata[i + 1] || 1 != pdata[i + 2])) i++;
	if (i >= len - 4)
	{
		// can't find start code
		return NULL;
	}
	else
	{
		if ((i > 0 && 0 == pdata[i - 1]))
		{
			start_code_size += 1;
			i--;
		}
		if (pstart_code_size)
		{
			*pstart_code_size = start_code_size;
		}
	}

	return pdata + i;
}
/*
stream type: 4:avc; 5:hevc
*/
int get_nal_type(int stream_type, const char* pdata)
{
	int nal_type = -1;
	if(4 == stream_type)
	{
		nal_type = pdata[0]&0x1f;
	}
	else if(5 == stream_type)
	{
		nal_type = (pdata[0]&0x7e) >> 1;
	}
	else
	{
		return nal_type;
	}

	return nal_type;
}
const char* find_xvc_nal(int stream_type, const char* pdata, int len, int nal_type, int* pnal_len, int has_start_code)
{
	int naltype = 0;
	int offset = 0;
	int start_code_size = 0;
	const char* pnal_begin = NULL;
	const char* pnal_end = NULL;
	while (offset < len)
	{
		pnal_begin = find_start_code(pdata + offset, len - offset, &start_code_size);
		//lbtrace("pnal_begin:%p = find_start_code(pdata + offset:%p, len - offset:%d, &start_code_size:%d)\n", pnal_begin, pdata + offset, len - offset, start_code_size);
		if (NULL == pnal_begin)
		{
			return NULL;
		}
		offset = pnal_begin + start_code_size - pdata;
		naltype = get_nal_type(stream_type, pnal_begin + start_code_size);
		//lbtrace("naltype:%d = get_nal_type(stream_type:%d, pnal_begin + start_code_size:%p), nal_hdr:%0x, nal_hdr+1:%0x\n", naltype, stream_type, pnal_begin + start_code_size, pnal_begin[start_code_size], pnal_begin[start_code_size+1]);
		if (nal_type == naltype)
		{
			pnal_end = find_start_code(pdata + offset, len - offset, NULL);
			//lbtrace("pnal_end:%p = find_start_code(pdata + offset:%p, len - offset:%d, NULL)\n", pnal_end, pdata + offset, len - offset);
			if (NULL == pnal_end)
			{
				pnal_end = pdata + len;
			}
			if (!has_start_code)
			{
				pnal_begin += start_code_size;

			}
			if (pnal_len)
			{
				*pnal_len = pnal_end - pnal_begin;
			}
			return pnal_begin;
		}
	}
	return 0;
}
int vava_hs_parser_sequence_hdr(VAVA_HS_REC_CTX* prec_ctx)
{
	IPC_PACKET_HDR pkthdr;
	int ret = 0;
	vava_hs_seek_packet(prec_ctx, 0);
	int len = 1024*1024;
	int offset = 0;
	unsigned char* readbuff = (char*)malloc(len);
	while(1)
	{
		ret = vava_hs_read_packet(prec_ctx, &pkthdr, readbuff, len);
		lbtrace("ret:%d, codec_id:%d, pkthdr.size:%d, pkthdr.pts:%" PRId64 ", pkthdr.keyflag:%d, prec_ctx->rec_hdr.encrypt:%d, readbuff[0]:%0x\n", ret, pkthdr.codec_id, pkthdr.size, pkthdr.pts, pkthdr.keyflag, prec_ctx->rec_hdr.encrypt, (uint8_t)readbuff[0]);
		if(ret < 0)
		{
			lberror("hs read packet failed, ret:%d\n", ret);
			ret = -1;
			break;
		}
		else
		{
			//char buff[1024] = {0};
			if(pkthdr.keyflag && prec_ctx->rec_hdr.encrypt)
			{
				
				int i;
				for(i = 0; i < 7; i++)
				{
					printf("%02x ", (uint8_t)readbuff[i]);
				}
				printf("\n");
				if(0xbb == readbuff[0])
				{
					readbuff[0] = 0xff;
					//buff[0] = 0xff;
					offset++;
				}
#ifdef ENABLE_DECRYPT_KEY_FRAME
				VAVA_Aes_Decrypt((char*)readbuff + offset, (char*)readbuff + offset, pkthdr.size - offset);

				//memcpy(readbuff, buff, 7);
				for(i = 0; i < 7; i++)
				{
					printf("%02x ", (uint8_t)readbuff[i]);
				}
				printf("\n");
				
				lbtrace("VAVA_Aes_Decrypt(readbuff:%p, readbuff:%p, pkthdr.size:%d, buff[0]:%0x)\n", readbuff, readbuff, pkthdr.size - offset, (uint8_t)readbuff[0]);
#endif
			}
			if((0 == pkthdr.codec_id || 1 == pkthdr.codec_id) && pkthdr.keyflag)
			{
				// h.264
				int start_code_size = 0;
				int offset = 0;
				char* psrc = readbuff;
				const char* pshbegin = NULL;
				const char* pshend = NULL;
				int stream_type = pkthdr.codec_id==0 ? 4:5;
				int nal_len = 0;
				if(4 == stream_type)
				{
					pshbegin = find_xvc_nal(stream_type, readbuff, len, 7, &nal_len, 1);
					//lbtrace("sps nal_len:%d\n", nal_len);
					offset = nal_len;
					pshend = find_xvc_nal(stream_type, readbuff + offset, len - offset, 8, &nal_len, 1);
					pshend += nal_len;
					int spspps_len = pshend - pshbegin;
					//lbtrace("spspps len:%d\n", nal_len, spspps_len);
					prec_ctx->pvsh = (char*)malloc(spspps_len);
					prec_ctx->nvsh = spspps_len;
					memcpy(prec_ctx->pvsh, pshbegin, spspps_len);
				}
				else if(5 == stream_type)
				{
					pshbegin = find_xvc_nal(stream_type, readbuff, len, 32, &nal_len, 1);
					//lbtrace("vps nal_len:%d\n", nal_len);
					offset = nal_len;
					pshend = find_xvc_nal(stream_type, readbuff + offset, len - offset, 34, &nal_len, 1);
					pshend += nal_len;
					int spspps_len = pshend - pshbegin;
					//lbtrace("vpsspspps len:%d\n", nal_len, spspps_len);
					prec_ctx->pvsh = (char*)malloc(spspps_len);
					prec_ctx->nvsh = spspps_len;
					memcpy(prec_ctx->pvsh, pshbegin, spspps_len);
				}
				else
				{
					lberror("Invalid stream type:%d\n", stream_type);
					ret =  -1;
					break;
				}
			}
			else if(3 == pkthdr.codec_id)
			{
				int i = 0;
				for(i = 0; i < 16; i++)
				{
					printf("%02x ", (uint8_t)readbuff[i]);
				}
				printf("\n");
				if(0xff == (uint8_t)readbuff[0])
				{
					int i = 0;
					prec_ctx->padts_hdr = (char*)malloc(7);
					memcpy(prec_ctx->padts_hdr, readbuff, 7);
					prec_ctx->nadts_hdr_len = 7;
					lbtrace("readbuff:%p, len:%d\n", readbuff, 7);
					printf("=================================adts header [size - %d] ====================================\n", prec_ctx->nadts_hdr_len);
					for(i = 0; i < prec_ctx->nadts_hdr_len; i++)
					{
						printf("%02x ", prec_ctx->padts_hdr[i]);
					}
					printf("\n");
					printf("========================================================================\n");
				}
			}
			
		}

		if((prec_ctx->padts_hdr && prec_ctx->pvsh) || pkthdr.pts > 1000)
		{
			break;
		}
	}
	if(readbuff)
	{
		free(readbuff);
		readbuff = NULL;
	}
	vava_hs_seek_packet(prec_ctx, 0);
	return ret;
}

void vava_hs_close_record(VAVA_HS_REC_CTX** pprec_ctx)
{
	if(pprec_ctx && *pprec_ctx)
	{
		VAVA_HS_REC_CTX* prec_ctx = *pprec_ctx;
		if(prec_ctx->pfile)
		{
			fclose(prec_ctx->pfile);
			prec_ctx->pfile = NULL;
		}

		if(prec_ctx->pvsh)
		{
			free(prec_ctx->pvsh);
			prec_ctx->pvsh = NULL;
			prec_ctx->nvsh = 0;
		}

		if(prec_ctx->padts_hdr)
		{
			free(prec_ctx->padts_hdr);
			prec_ctx->padts_hdr = NULL;
			prec_ctx->nadts_hdr_len = 0;
		}
		free((void*)prec_ctx);
		*pprec_ctx = prec_ctx = NULL;
		lbtrace("vava_hs_close_record(pprec_ctx:%p)\n", pprec_ctx);
	}
}

int hs004_create_mux_record(VAVA_HS_MUX_CTX* pmc)
{
	if(NULL == pmc)
	{
		lberror("Invalid param pmc:%p\n", pmc);
		return -1;
	}
	VAVA_HS004_REC_HDR* pprc_hdr = (VAVA_HS004_REC_HDR*)malloc(sizeof(VAVA_HS004_REC_HDR));
	memset(pprc_hdr, 0, sizeof(VAVA_HS004_REC_HDR));
  	pprc_hdr->tag = 0;
	pprc_hdr->ver = 0;
	pprc_hdr->v_encode = pmc->rec_hdr.vcodec_id == 28 ? 0 : 1;
	pprc_hdr->a_encode = 3;
	pprc_hdr->res = 0;
	pprc_hdr->fps = pmc->rec_hdr.fps;
	pprc_hdr->sample = pmc->rec_hdr.sample_rate;
	pprc_hdr->encrypt = pmc->rec_hdr.encrypt;
	pprc_hdr->alarmtype = pmc->alarm_type;

	pmc->pfile = fopen(pmc->prec_url,  "wb");

	if(NULL == pmc->pfile)
	{
		lberror("pmc->pfile:%p = fopen(pmc->prec_url:%s,  wb) failed, err:%s\n", pmc->pfile, pmc->prec_url, strerror(errno));
		free(pprc_hdr);
		return -1;
	}
	pmc->priv_data = pprc_hdr;
	fwrite(pprc_hdr, 1, sizeof(VAVA_HS004_REC_HDR), pmc->pfile);
	lbtrace("hs004_create_mux_record end, ftell:%ld\n", ftell(pmc->pfile));
	return 0;
}

int hs004_close_mux_record(VAVA_HS_MUX_CTX** ppmc)
{
	if(ppmc && *ppmc)
	{
		VAVA_HS_MUX_CTX* pmc = *ppmc;
		if(pmc->priv_data && pmc->pfile)
		{
			VAVA_HS004_REC_HDR* pprc_hdr = (VAVA_HS004_REC_HDR*)pmc->priv_data;
			pprc_hdr->tag = 1;
			fseek(pmc->pfile, 0, SEEK_SET);
			fwrite(pmc->priv_data, 1, sizeof(VAVA_HS004_REC_HDR), pmc->pfile);

			fclose(pmc->pfile);
			pmc->pfile = NULL;
			free(pmc->priv_data);
			pmc->priv_data = NULL;
		}
		
	}
    
    return 0;
}

int hs004_write_mux_record(VAVA_HS_MUX_CTX* pmc, IPC_PACKET_HDR* piph, char* pdata, int len)
{
	if(NULL == pmc || NULL == piph)
	{
		lberror("Invalid param pmc:%p, piph:%p\n", pmc, piph);
		return -1;
	}

	VAVA_HS004_REC_HDR* pprc_hdr = (VAVA_HS004_REC_HDR*)pmc->priv_data;
	VAVA_HS004_PACKET_HDR pkt_hdr;
	pkt_hdr.tag = 0xEB0000AA;
	pkt_hdr.type = to_rec_codec_id(piph->codec_id);
	pkt_hdr.size = len;
	if(0 == piph->mt)
	{
		// vidoe packet
		pkt_hdr.framnum = pmc->rec_hdr.vframe_num;
		pmc->rec_hdr.vframe_num++;
		pkt_hdr.fps = pmc->rec_hdr.fps;
		

	}
	else
	{
		// audio packet
		pkt_hdr.framnum = pmc->rec_hdr.aframe_num;
		pmc->rec_hdr.aframe_num++;
		pkt_hdr.fps = pmc->rec_hdr.sample_rate;
	}
	pkt_hdr.ntsamp = piph->pts;
	
	int writed = fwrite(&pkt_hdr, 1, sizeof(pkt_hdr), pmc->pfile);
	
	writed = fwrite(pdata, 1, len, pmc->pfile);
	if(writed <= 0)
	{
		lberror("writed:%d = fwrite(pdata, 1, len:%d, pmc->pfile)\n", writed, len);
		return -1;
	}
	pprc_hdr->size += writed;

	return 0;
}

VAVA_HS_MUX_CTX* vava_hs_create_mux_record(const char* purl, int rec_type, int alarm_type, int vcodec_id, int acodec_id, int fps, int samplerate, int encrypt)
{
	int ret = -1;
	VAVA_HS_MUX_CTX* pmc = (VAVA_HS_MUX_CTX*)malloc(sizeof(VAVA_HS_MUX_CTX));
	memset(pmc, 0, sizeof(VAVA_HS_MUX_CTX));
	pmc->rec_type = (ecamera_record_type)rec_type;
	pmc->alarm_type = alarm_type;
	pmc->rec_hdr.vcodec_id = vcodec_id;
	pmc->rec_hdr.acodec_id = acodec_id;
	pmc->rec_hdr.fps = fps;
	pmc->rec_hdr.sample_rate = samplerate;
	//pmc->rec_hdr.res = res;
	pmc->rec_hdr.encrypt = encrypt;
	pmc->prec_url = (char*)malloc(strlen(purl)+1);
	memcpy(pmc->prec_url, purl, strlen(purl)+1);
	if(ecamera_hs_004 == rec_type)
	{
		//ret = hs004_create_mux_record(pmc);
		//if(0 == ret)
		pmc->pcreate_rec_func = hs004_create_mux_record;
		pmc->pclose_rec_func = hs004_close_mux_record;
		pmc->pwrite_rec_func = hs004_write_mux_record;
        ret = 0;
	}

	if(0 != ret)
	{
		free(pmc->prec_url);
		free(pmc);
		pmc = NULL;
	}
	ret = pmc->pcreate_rec_func(pmc);
	if(ret != 0)
	{
		lberror("ret:%d = pmc->pcreate_rec_func(pmc:%p)\n", ret, pmc);
		if(pmc->prec_url)
		{
			free(pmc->prec_url);
			pmc->prec_url = NULL;
		}
		free(pmc);
		pmc = NULL;
	}
	return pmc;
}

int vava_hs_close_mux_record(VAVA_HS_MUX_CTX** ppmc)
{
	if(ppmc && *ppmc && (*ppmc)->pclose_rec_func)
	{
		(*ppmc)->pclose_rec_func(ppmc);
	}
    return 0;
}

int vava_hs_write_mux_record(VAVA_HS_MUX_CTX* pmc, int codec_id, char* pdata, int len, int keyflag, int64_t pts)
{
	if(pmc && pmc->pwrite_rec_func)
	{
		pmc->pkt_hdr.mt = codec_id < 10000 ? 0 : 1;
		pmc->pkt_hdr.codec_id = codec_id;
		pmc->pkt_hdr.keyflag = keyflag;
		pmc->pkt_hdr.size = len;
		pmc->pkt_hdr.pts = pts;
		return pmc->pwrite_rec_func(pmc, &pmc->pkt_hdr, pdata, len);
	}

	return -1;
}

#include "lbsc_media_parser.h"
struct lbsp_rational
{
    int num;
    int den;
};

struct lbsp_rational avc_sample_aspect_ratio[17] = {
    { 0,  1 },
    { 1,  1 },
    { 12, 11 },
    { 10, 11 },
    { 16, 11 },
    { 40, 33 },
    { 24, 11 },
    { 20, 11 },
    { 32, 11 },
    { 80, 33 },
    { 18, 11 },
    { 15, 11 },
    { 64, 33 },
    { 160, 99 },
    { 4,  3 },
    { 3,  2 },
    { 2,  1 },
};

lbsc_xvc_ctx* lbsc_open_xvc_context(int stream_type)
{
    lbsc_xvc_ctx* pxc = (lbsc_xvc_ctx*)malloc(sizeof(lbsc_xvc_ctx));
    memset(pxc, 0, sizeof(lbsc_xvc_ctx));
    pxc->m_pbsc = bitstream_open(NULL, 0);
    pxc->m_nidr_frame = -1;
    pxc->m_pon_parse_nal = on_parse_nalu;
    pxc->m_nstream_type = stream_type;
    if(AVC_STREAM_TYPE == pxc->m_nstream_type)
    {
        // h264 stream
        pxc->m_unal_type_vps = 0;
        pxc->m_unal_type_sps = EAVC_NAL_SPS;
        pxc->m_unal_type_pps = EAVC_NAL_PPS;
        pxc->m_unal_type_mask = AVC_NAL_TYPE_MASK;
        pxc->m_uxvcc_fixed_header_size = 5;
        pxc->m_uxvcc_fixed_metadata_prefix_size = 1;
        pxc->m_uxvcc_fixed_nal_prefix_size = 2;
        //lbsc_avc_open_parser* papc
        pxc->m_powner = lbsc_avc_open_parser(pxc);
    }
    else if(HEVC_STREAM_TYPE == pxc->m_nstream_type)
    {
        // h265 stream
        pxc->m_unal_type_vps = HEVC_NAL_VPS;
        pxc->m_unal_type_sps = HEVC_NAL_SPS;
        pxc->m_unal_type_pps = HEVC_NAL_PPS;
        pxc->m_unal_type_mask = HEVC_NAL_TYPE_MASK;
        pxc->m_uxvcc_fixed_header_size = 23;
        pxc->m_uxvcc_fixed_metadata_prefix_size = 3;
        pxc->m_uxvcc_fixed_nal_prefix_size = 2;
        pxc->m_powner = lbsc_hevc_open_parser(pxc);
    }
    else
    {
        assert(0);
    }
    return pxc;
}

void lbsc_close_xvc_context(lbsc_xvc_ctx** ppxc)
{
    if(ppxc && *ppxc)
    {
        lbsc_xvc_ctx* pxc = *ppxc;
        if(AVC_STREAM_TYPE == pxc->m_nstream_type)
        {
            //lbsc_avc_parser_ctx* papc = (lbsc_avc_parser_ctx*)pxc->m_powner;
            lbsc_avc_parser_ctx* pavc = pxc->m_powner;
            if(pavc)
            {
                pavc->m_pxc = NULL;
                lbsc_avc_close_parser(&pavc);
            }
        }
        else if(HEVC_STREAM_TYPE == pxc->m_nstream_type)
        {
            lbsc_hevc_parser_ctx* phevc = (lbsc_hevc_parser_ctx*)pxc->m_powner;
            if(phevc)
            {
                phevc->m_pxc = NULL;
                lbsc_hevc_close_parser(phevc);
            }
        }
        pxc->m_powner = NULL;
        if(pxc->m_pbsc)
        {
            bitstream_close(&pxc->m_pbsc);
        }
        if(pxc->m_pvps)
        {
            free(pxc->m_pvps);
            pxc->m_pvps = NULL;
        }
        if(pxc->m_psps)
        {
            free(pxc->m_psps);
            pxc->m_psps = NULL;
        }
        if(pxc->m_ppps)
        {
            free(pxc->m_ppps);
            pxc->m_ppps = NULL;
        }

        free(pxc);
        //lbsc_reset_xvc_context(pxc);
        //free(pxc);
        pxc = *ppxc = NULL;
    }
}

void lbsc_reset_xvc_context(lbsc_xvc_ctx* pxc)
{
    if(pxc)
    {
        if(pxc->m_pvps)
        {
            free(pxc->m_pvps);
            pxc->m_pvps = NULL;
            pxc->m_nvsh_len = 0;
        }
        
        if(pxc->m_psps)
        {
            free(pxc->m_psps);
            pxc->m_psps = NULL;
            pxc->m_nsps_len = 0;
        }
        
        if(pxc->m_ppps)
        {
            free(pxc->m_ppps);
            pxc->m_ppps = NULL;
            pxc->m_npps_len = 0;
        }
        if(pxc->m_pvsh)
        {
            free(pxc->m_pvsh);
            pxc->m_pvsh = NULL;
            pxc->m_nvsh_len = 0;
        }
        pxc->m_nwidth  = 0;
        pxc->m_nheight = 0;
        //memset(pxc, 0, sizeof(lbsc_xvc_ctx));
    }
}

int is_sequence_header(lbsc_xvc_ctx* pxc, int nal_type)
{
    if(pxc)
    {
        if (AVC_STREAM_TYPE == pxc->m_nstream_type)
        {
            // h264 stream
            if(EAVC_NAL_SPS == nal_type || EAVC_NAL_PPS == nal_type)
            {
                return 1;
            }
        }
        else if (HEVC_STREAM_TYPE == pxc->m_nstream_type)
        {
            // h265 stream
            // h264 stream
            if(HEVC_NAL_VPS == nal_type || HEVC_NAL_SPS == nal_type || HEVC_NAL_PPS == nal_type)
            {
                return 1;
            }
        }
        else
        {
            assert(0);
        }
        return 0;
    }
    
    return -1;
}

int lbsc_get_sequence_header(lbsc_xvc_ctx* pxc, char* psh, int len)
{
    if(NULL == pxc || NULL == psh)
    {
        lberror("Invalid parameter, pxc:%p, psh:%p\n", pxc, psh);
    }
    int offset = 0;
    char start_code[4] = {0, 0, 0, 1};
    int sh_len = pxc->m_nvps_len + pxc->m_nsps_len + pxc->m_npps_len + 8 + (pxc->m_nvps_len > 0 ? 4:0);
    if(sh_len > len)
    {
        lberror("not enough memory buffer, have len:%d, need sh_len:%d\n", len, sh_len);
        return -1;
    }
    
    if(pxc->m_nvps_len > 0 && len > pxc->m_nvps_len+4)
    {
        memcpy(psh, start_code, 4);
        offset += sizeof(start_code);
        memcpy(psh + offset, pxc->m_pvps, pxc->m_nvps_len);
        offset += pxc->m_nvps_len;
    }
    
    if(pxc->m_nsps_len > 0 && len - offset > pxc->m_nsps_len+4)
    {
        memcpy(psh + offset, start_code, 4);
        offset += sizeof(start_code);
        memcpy(psh + offset, pxc->m_psps, pxc->m_nsps_len);
        offset += pxc->m_nsps_len;
    }
    
    if(pxc->m_npps_len > 0 && len - offset > pxc->m_npps_len+4)
    {
        memcpy(psh + offset, start_code, 4);
        offset += sizeof(start_code);
        memcpy(psh + offset, pxc->m_ppps, pxc->m_npps_len);
        offset += pxc->m_npps_len;
    }
    
    return offset;
}

int is_idr_frame(int stream_type, const char* pdata, int len)
{
    //const char* pnal = skip_start_code(pdata, len, NULL);
    const char* pnal =skip_aud_nal(stream_type, pdata, &len);
    if(NULL == pnal)
    {
        lberror("No start code foud, pnal:%p\n", pnal);
        return -1;
    }
    int nal_type = read_nalu_type_from_data(stream_type, pnal, len);
    /*if(EAVC_NAL_AUD == nal_type || HEVC_NAL_AUD == nal_type)
    {
        int start_code_size;
        const char* pnal = find_annexb_start_code(pdata, len - 0, &start_code_size);
        if(pnal)
        {
            read_nalu_type_from_data(stream_type, pnal, len);
        }
        nal_type = get_nal(stream_type, pdata + 4, len - 4, NULL, &nal_type, 1);
    }*/
    if(AVC_STREAM_TYPE == stream_type)
    {
        if(EAVC_NAL_IDR == nal_type)
        {
            return 1;
        }
        else if(EAVC_NAL_SPS == nal_type)
        {
            return 1;
        }
        else if(EAVC_NAL_PPS == nal_type)
        {
            return 1;
        }
    }
    else if(HEVC_STREAM_TYPE == stream_type)
    {
        if(HEVC_NAL_BLA_W_LP <= nal_type && HEVC_NAL_CRA_NUT >= nal_type)
        {
            return 1;
        }
        else if(HEVC_NAL_VPS == nal_type)
        {
            return 1;
        }
        else if(HEVC_NAL_SPS == nal_type)
        {
            return 1;
        }
        else if(HEVC_NAL_PPS == nal_type)
        {
            return 1;
        }
    }

    return 0;
}

int is_frame_nalu(int stream_type, int nal_type, int idr)
{
    if(AVC_STREAM_TYPE == stream_type)
    {
        if(EAVC_NAL_IDR == nal_type)
        {
            return 1;
        }
        else if(!idr && nal_type >= 1 && nal_type < EAVC_NAL_IDR)
        {
            return 1;
        }
    }
    else if(HEVC_STREAM_TYPE == stream_type)
    {
        if(HEVC_NAL_BLA_W_LP <= nal_type && HEVC_NAL_CRA_NUT >= nal_type)
        {
            return 1;
        }
        else if(!idr && HEVC_NAL_TRAIL_N < nal_type && HEVC_NAL_BLA_W_LP > nal_type)
        {
            return 1;
        }
    }

    return 0;
}

int is_start_code(const char* pdata, int len, int* pstart_code_len)
{
    int i = 0;
    if(NULL == pdata || len < 4)
    {
        return 0;
    }
    while(0 == pdata[i] && i < 4)i++;
    if(1 == pdata[i] && (2 == i || 3 == i))
    {
        if(pstart_code_len)
        {
            *pstart_code_len = i + 1;
        }
        return 1;
    }

    return 0;
}

int get_start_code_size(const char* pdata, int len)
{
    int i = 0;
    for(i = 0; i < len; i++) if(0 != pdata[i])break;
    if(i > 0 && 1 == pdata[i])
    {
        i++;
    }

    return i;
}

int parse_stream(lbsc_xvc_ctx* pxc, const char* pdata, int len)
{
    int ret = 0;
    int nal_len = 0;
    int nal_type = 0;
    const char* pnal_begin = NULL;
    if(NULL == pxc || NULL == pdata)
    {
        lberror("Invalid parameter, pxc:%p, pdata:%p\n", pxc, pdata);
        return -1;
    }
    bitstream_ctx* pbs = pxc->m_pbsc;
    ret = initialize(pbs, (char*)pdata, len);
    assert(0 == ret);
    pxc->m_nidr_frame = -1;

    if (remain(pbs) <= 4)
    {
        lberror("Not enought stream for parser remain:%d\n", remain(pbs));
        return -1;
    }

    while (remain(pbs) > 0)
    {
        pnal_begin = get_nal(pxc->m_nstream_type, (char*)cur_data(pbs), remain(pbs), &nal_len, &nal_type, 1);
        if (pnal_begin)
        {
            ret = pxc->m_pon_parse_nal(pxc, nal_type, pnal_begin, nal_len);
            if (ret != 0)
            {
                break;
            }
            move(pbs, nal_len, 0);
            //pbs->bytesoffset = pnal_begin + nal_len - pbs->pdata;//(uint8_t*)pnal_begin + nal_len;
            //pbs->bitsoffset = 0;
        }
        else
        {
            return 0;
        }
    }

    return ret;
    
}

int on_parse_nalu(void* powner, int nalu_type, const char* pdata, int len)
{
    int ret = 0;
    lbsc_xvc_ctx* pxc = (lbsc_xvc_ctx*)powner;
    int nal_len = 0;
    if(NULL == pxc)
    {
        lberror("Invalid parameter pxc:%p, pdata:%p\n", pxc, pdata);
        return -1;
    }
    pdata = skip_start_code(pdata, len, &nal_len);
    if(pxc->m_unal_type_vps > 0 && pxc->m_unal_type_vps == nalu_type)
    {
        if(pxc->m_pvps)
        {
            free(pxc->m_pvps);
            pxc->m_pvps = NULL;
            pxc->m_nvps_len = 0;
        }
        
        /*pxc->m_pvps = (char*)malloc(nal_len);
        memcpy(pxc->m_pvps, pdata, nal_len);
        pxc->m_nvps_len = nal_len;
        if(AVC_STREAM_TYPE == pxc->m_nstream_type)
        {
            lbsc_avc_parser_ctx* papc = (lbsc_avc_parser_ctx*)pxc->m_powner;
            ret = lbsc_avc_demux_sps(, pdata, nal_len);
            
        }
        else */if(HEVC_STREAM_TYPE == pxc->m_nstream_type)
        {
            lbsc_hevc_parser_ctx* phpc = (lbsc_hevc_parser_ctx*)pxc->m_powner;
            lbsc_hevc_demux_vps(phpc, pdata, nal_len);
        }
    }
    else if(pxc->m_unal_type_sps > 0 && pxc->m_unal_type_sps == nalu_type)
    {
        if(pxc->m_psps)
        {
            free(pxc->m_psps);
            pxc->m_psps = NULL;
            pxc->m_nsps_len = 0;
        }
        
        /*pxc->m_psps = (char*)malloc(nal_len);
        memcpy(pxc->m_psps, pdata, nal_len);
        pxc->m_nsps_len = nal_len;*/
        
        if(AVC_STREAM_TYPE == pxc->m_nstream_type)
        {
            lbsc_avc_parser_ctx* papc = (lbsc_avc_parser_ctx*)pxc->m_powner;
            ret = lbsc_avc_demux_sps(papc, pdata, nal_len);
            
        }
        else if(HEVC_STREAM_TYPE == pxc->m_nstream_type)
        {
            lbsc_hevc_parser_ctx* phpc = (lbsc_hevc_parser_ctx*)pxc->m_powner;
            ret = lbsc_hevc_demux_sps(phpc, pdata, nal_len);
        }
    }
    else if(pxc->m_unal_type_pps > 0 && pxc->m_unal_type_pps == nalu_type)
    {
        if(pxc->m_ppps)
        {
            free(pxc->m_ppps);
            pxc->m_ppps = NULL;
            pxc->m_npps_len = 0;
        }
        
        /*pxc->m_ppps = (char*)malloc(nal_len);
        memcpy(pxc->m_ppps, pdata, nal_len);
        pxc->m_npps_len = nal_len;*/
        if(AVC_STREAM_TYPE == pxc->m_nstream_type)
        {
            lbsc_avc_parser_ctx* papc = (lbsc_avc_parser_ctx*)pxc->m_powner;
            ret = lbsc_avc_demux_pps(papc, pdata, nal_len);
            
        }
        else if(HEVC_STREAM_TYPE == pxc->m_nstream_type)
        {
            lbsc_hevc_parser_ctx* phpc = (lbsc_hevc_parser_ctx*)pxc->m_powner;
            ret = lbsc_hevc_demux_pps(phpc, pdata, nal_len);
        }
    }
    else if(AVC_STREAM_TYPE == pxc->m_nstream_type)
    {
        if(EAVC_NAL_IDR == nalu_type)
        {
            // idr frame
            pxc->m_nidr_frame = 1;
        }
        else if(nalu_type >= EAVC_NAL_SLICE && nalu_type < EAVC_NAL_IDR)
        {
            // P/B frame
            pxc->m_nidr_frame = 0;
        }
        else
        {
            // no frame
            pxc->m_nidr_frame = -1;
        }
        
    }
    else if(HEVC_STREAM_TYPE == pxc->m_nstream_type)
    {
        if(HEVC_NAL_BLA_W_LP <= nalu_type && HEVC_NAL_CRA_NUT >= nalu_type)
        {
            // key frame
            pxc->m_nidr_frame = 1;
        }
        else if(nalu_type > 0 && nalu_type < HEVC_NAL_BLA_W_LP)
        {
            // P/B frame
            pxc->m_nidr_frame = 0;
        }
        else
        {
            // no frame
            pxc->m_nidr_frame = -1;
        }
    }
    else
    {
        lberror("Invalid stream type:%d\n", pxc->m_nstream_type);
    }
    return 0;
}

int read_nal_type(bitstream_ctx* pbs, int stream_type)
//int read_nal_type(int stream_type, const char* pdata, int len)
{
    int ret = -1;
    uint8_t nal_type = -1;
    //bitstream_ctx* pbs = bitstream_open((char*)pdata, len);
    do {
        uint8_t forbiden = read_bits(pbs, 1);
        if (0 != forbiden)
        {
            break;
        }
        
        if (AVC_STREAM_TYPE == stream_type)
        {
            read_bits(pbs, 2);
            nal_type = read_bits(pbs, 5);
        }
        else if (HEVC_STREAM_TYPE == stream_type)
        {
            nal_type = read_bits(pbs, 6);
            read_bits(pbs, 6);
            read_bits(pbs, 3);
        }
        else
        {
            break;
        }
        ret = 0;
    } while (0);

    return 0 == ret ? nal_type : ret;
}

int read_nalu_type_from_data(int stream_type, const char* pdata, int len)
{
    int nal_type = 0;
    int nal_len = 0;
    assert(pdata);
    pdata = skip_start_code(pdata, len, &nal_len);
    if(NULL == pdata || nal_len <= 1)
    {
        lberror("read nalu type from data failed, pdata:%p, nal_len:%d\n", pdata, nal_len);
        return -1;
    }

    if (AVC_STREAM_TYPE == stream_type)
    {
        nal_type = pdata[0]&0x1f;
    }
    else if (HEVC_STREAM_TYPE == stream_type)
    {
        nal_type = (pdata[0]&0x7e) >> 1;
    }
    else
    {
        lberror("Invalid stream type:%d\n", stream_type);
        return -1;
    }
    
    return nal_type;
}

const char* read_annexb_nal(const char* pdata, int len, int* pnal_len, int has_start_code)
{
    int start_code_size = 0;
    int nal_len = 0;
    const char* pnal = find_annexb_start_code(pdata, len, &start_code_size);
    if(NULL == pnal)
    {
        return NULL;
    }
    int offset = (int)(pnal - pdata);
    const char* pnal2 = find_annexb_start_code(pnal, len - offset, &start_code_size);
    if(NULL == pnal2)
    {
        nal_len = len - offset;
    }
    else
    {
        nal_len = (int)(pnal2 - pnal) - start_code_size;
    }
    if(pnal_len)
    {
        *pnal_len = nal_len;
    }
    return pnal;
}

const char* find_annexb_start_code(const char* pdata, int len, int* pstart_code_size)
{
    int i = 0;
    int start_code_size = 3;
    while ((i < len - 3) && (0 != pdata[i] || 0 != pdata[i + 1] || 1 != pdata[i + 2])) i++;
    if (i >= len - 4)
    {
        // can't find start code
        lberror("can't find start code, pdata[0]:%0x, pdata[1]:%0x, pdata[2]:%0x, pdata[3]:%0x\n", pdata[0], pdata[1], pdata[2], pdata[3]);
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

const char* get_nal(int stream_type, const char* pdata, int len, int* pnal_len, int* pnal_type, int has_start_code)
{
    int offset = 0;
    int start_code_size = 0;
    const char* pnal_begin = find_annexb_start_code(pdata + offset, len - offset, &start_code_size);
    if (NULL == pnal_begin)
    {
        //lberror("can't find start code in stream data\n");
        return pnal_begin;
    }
    if(pnal_type)
    {
        *pnal_type = read_nalu_type_from_data(stream_type, (char*)pdata, len);
        //bitstream_ctx* pbs = bitstream_open((char*)pdata, len);
        //*pnal_type = read_nal_type(pbs, stream_type);
        //bitstream_close(&pbs);
    }
    offset += pnal_begin - pdata + 4;
    const char* pnal_end = find_annexb_start_code(pdata + offset, len - offset, NULL);
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

const char* find_nal(lbsc_xvc_ctx* pxc, const char* pdata, int len, int nal_type, int* pnal_len, int has_start_code)
{
    int naltype = 0;
    int offset = 0;
    int start_code_size = 0;
    const char* pnal_begin = NULL;
    const char* pnal_end = NULL;
    while (offset < len)
    {
        pnal_begin = find_annexb_start_code(pdata + offset, len - offset, &start_code_size);
        if (NULL == pnal_begin)
        {
            return NULL;
        }
        offset = pnal_begin + start_code_size - pdata;
        if (-1 == nal_type || nal_type == naltype)
        {
            pnal_end = find_annexb_start_code(pdata + offset, len - offset, NULL);
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

char* rbsp_from_nalu(const char* pdata, int len, int* pout_len)
{
    char* rbsp = (char*)malloc(len);
    int rbsp_len = 0;
    int i = 0, j = 0;
    while (i < len)
    {
        if (j >= 2 && pdata[i] == 0x3)
        {
            i++;
            j = 0;
            continue;
        }
        else if (0 == pdata[i])
        {
            j++;
        }
        else
        {
            j = 0;
        }

        rbsp[rbsp_len++] = pdata[i++];
    }
    if(pout_len)
    {
        *pout_len = rbsp_len;
    }
    return rbsp;
}

const char* skip_start_code(const char* pdata, int len, int* pnal_len)
{
    int zero_num = 0;
    for (int i = 0; i < len; i++)
    {
        if (0 == pdata[i])
        {
            zero_num++;
        }
        else if (zero_num >= 2 && 1 == pdata[i])
        {
            if (pnal_len)
            {
                *pnal_len = len - i - 1;
            }
            return (const char*)pdata + i + 1;;
        }
        else
        {
            if (pnal_len)
            {
                *pnal_len = len;
            }
            return pdata;
        }
    }

    return NULL;
}

const char* skip_aud_nal(int stream_type, const char* pdata, int* plen)
{
    int data_len = 0;
    int len = *plen;
    const char* pnal = skip_start_code(pdata, len, &data_len);
    int nal_type = read_nalu_type_from_data(stream_type, pdata, len);
    if(EAVC_NAL_AUD == nal_type || HEVC_NAL_AUD == nal_type)
    {
        pnal = find_annexb_start_code(pnal, data_len, NULL);
        *plen = len - (int)(pnal - pdata);
    }
    
    return pnal;
}

const char* get_nalu_frame(int streamtype, const char* pdata, int len, int* pframe_size, int bparser_to_end, int bhas_start_code)
{
    int naltype = 0;
    int offset = 0;
    int start_code_size = 0;
    int nal_size = 0;
    int nprefix = 0;
    const char* pnal_begin = NULL;
    const char* pnal_end = NULL;
    while (offset < len)
    {
        pnal_begin = find_annexb_start_code(pdata + offset, len - offset, &start_code_size);
        //lbtrace("pnal_begin:%p, offset:%d, len - offset:%d, naltype:%d\n", pnal_begin, offset, len - offset, naltype);
        if (NULL == pnal_begin)
        {
            return NULL;
        }
        offset = pnal_begin + start_code_size - pdata;
        naltype = read_nalu_type_from_data(streamtype, pnal_begin, len - start_code_size);
        if(is_frame_nalu(streamtype, naltype, 0))
        {
            if(!bparser_to_end)
            {
                nal_size = len - offset + start_code_size;
                break;
            }
            else
            {
                pnal_end = find_annexb_start_code(pdata + offset, len - offset, NULL);
                if(NULL == pnal_end)
                {
                    nal_size = len - offset + start_code_size;
                    break;
                }
            }
        }
    }
    if(!bhas_start_code)
    {
        nprefix = start_code_size;
    }
    if(pframe_size)
    {
        *pframe_size = nal_size - nprefix;
    }
    return pnal_begin + nprefix;
}

int add_metadata(lbsc_xvc_ctx* pxc, const char* pdata, int len)
{
    int nal_len = 0;
    if(NULL == pxc || NULL == pdata)
    {
        return -1;
    }
    
    pdata = skip_start_code(pdata, len, &nal_len);
    //initialize(pxc->m_pbsc, pdata, nal_len);
    int nal_type = read_nalu_type_from_data(pxc->m_nstream_type, pdata, len);//read_nal_type(pxc->m_pbsc, pxc->m_nstream_type);
    
    if(pxc->m_unal_type_vps == nal_type)
    {
        free(pxc->m_pvps);
        pxc->m_pvps = malloc(nal_len);
        memcpy(pxc->m_pvps, pdata, nal_len);
        pxc->m_nvps_len = nal_len;
    }
    else if(pxc->m_unal_type_sps == nal_type)
    {
        free(pxc->m_psps);
        pxc->m_psps = malloc(nal_len);
        memcpy(pxc->m_psps, pdata, nal_len);
        pxc->m_nsps_len = nal_len;
    }
    else if(pxc->m_unal_type_pps == nal_type)
    {
        free(pxc->m_ppps);
        pxc->m_ppps = malloc(nal_len);
        memcpy(pxc->m_ppps, pdata, nal_len);
        pxc->m_npps_len = nal_len;
    }
    else
    {
        lberror("Invaid metadata nal_type:%d\n", nal_type);
        return -1;
    }
    
    return 0;
}

int get_extradata_size(lbsc_xvc_ctx* pxc)
{
    if(NULL == pxc)
    {
        return -1;
    }
    if(AVC_STREAM_TYPE == pxc->m_nstream_type)
    {
        return lbsc_avc_get_extradata_size((lbsc_avc_parser_ctx*)pxc->m_powner);
    }
    else if(HEVC_STREAM_TYPE == pxc->m_nstream_type)
    {
        return lbsc_get_mux_hvcc_size((lbsc_hevc_parser_ctx*)pxc->m_powner);
    }
    
    return 0;
    /*extradata_size = pxc->m_uxvcc_fixed_header_size;
    extradata_size += pxc->m_nvps_len > 0 ? pxc->m_uxvcc_fixed_metadata_prefix_size : 0;
    extradata_size += pxc->m_nsps_len > 0 ? pxc->m_uxvcc_fixed_metadata_prefix_size : 0;
    extradata_size += pxc->m_npps_len > 0 ? pxc->m_uxvcc_fixed_metadata_prefix_size : 0;
    extradata_size += 1 + pxc->m_uxvcc_fixed_nal_prefix_size;
    extradata_size += 1 + pxc->m_uxvcc_fixed_nal_prefix_size;
    extradata_size += 1 + pxc->m_uxvcc_fixed_nal_prefix_size;
    return extradata_size;*/
}

int mux_extradata(lbsc_xvc_ctx* pxc, char* pextradata, int size)
{
    if(NULL == pxc || NULL == pextradata)
    {
        lberror("Invalid parameter, pxc:%p, pextradata:%p\n", pxc, pextradata);
        return -1;
    }
    
    if(AVC_STREAM_TYPE == pxc->m_nstream_type)
    {
        lbsc_avc_parser_ctx* papc = (lbsc_avc_parser_ctx*)pxc->m_powner;
        return lbsc_avc_mux_extradata(papc, pextradata, size);
    }
    else if(HEVC_STREAM_TYPE == pxc->m_nstream_type)
    {
        lbsc_hevc_parser_ctx* phpc = (lbsc_hevc_parser_ctx*)pxc->m_powner;
        return lbsc_hevc_mux_hvcc(phpc, pextradata, size);
    }
    
    lberror("Invalid stream type:%d\n");
    return -1;
}

int demux_extradata(lbsc_xvc_ctx* pxc, const char* pextradata, int size)
{
    if(NULL == pxc || NULL == pextradata)
    {
        lberror("Invalid parameter, pxc:%p, pextradata:%p\n", pxc, pextradata);
        return -1;
    }
    
    if(AVC_STREAM_TYPE == pxc->m_nstream_type)
    {
        lbsc_avc_parser_ctx* papc = (lbsc_avc_parser_ctx*)pxc->m_powner;
        return lbsc_avc_demux_extradata(papc, pextradata, size);
    }
    else if(HEVC_STREAM_TYPE == pxc->m_nstream_type)
    {
        lbsc_hevc_parser_ctx* phpc = (lbsc_hevc_parser_ctx*)pxc->m_powner;
        return lbsc_hevc_demux_hvcc(phpc, (char*)pextradata, size);
    }
    
    lberror("Invalid stream type:%d\n");
    return -1;
}

// avc demux begin
lbsc_avc_sps_ctx* lbsc_open_avc_sps_context()
{
    lbsc_avc_sps_ctx* pspsctx = (lbsc_avc_sps_ctx*)malloc(sizeof(lbsc_avc_sps_ctx));
    memset(pspsctx, 0, sizeof(lbsc_avc_sps_ctx));
    
    return pspsctx;
}

void lbsc_close_avc_sps_context(lbsc_avc_sps_ctx** ppasc)
{
    if(ppasc && *ppasc)
    {
        lbsc_avc_sps_ctx* pspsctx = *ppasc;
        if(pspsctx->offset_for_ref_frame)
        {
            free(pspsctx->offset_for_ref_frame);
            pspsctx->offset_for_ref_frame = NULL;
        }
        
        if(pspsctx->seq_scaling_list_present_flag)
        {
            free(pspsctx->seq_scaling_list_present_flag);
            pspsctx->seq_scaling_list_present_flag = NULL;
        }
        
        free(pspsctx);
    }
}

lbsc_avc_parser_ctx* lbsc_avc_open_parser(lbsc_xvc_ctx* pxc)
{
    lbsc_avc_parser_ctx* papc = (lbsc_avc_parser_ctx*)malloc(sizeof(lbsc_avc_parser_ctx));
    memset(papc, 0, sizeof(lbsc_avc_parser_ctx));
    if(NULL != pxc)
    {
        papc->m_pxc = pxc;
    }
    else
    {
        pxc = lbsc_open_xvc_context(AVC_STREAM_TYPE);
    }
    papc->m_pasc = NULL;//(lbsc_avc_sps_ctx*)malloc(sizeof(lbsc_avc_sps_ctx));

    return papc;
}

void lbsc_avc_close_parser(lbsc_avc_parser_ctx** ppapc)
{
    if(ppapc && *ppapc)
    {
        lbsc_avc_parser_ctx* papc = *ppapc;
        if(papc->m_pxc)
        {
            lbsc_close_xvc_context(&papc->m_pxc);
        }

        if(papc->m_pasc)
        {
            if(papc->m_pasc->seq_scaling_list_present_flag)
            {
                free(papc->m_pasc->seq_scaling_list_present_flag);
                papc->m_pasc->seq_scaling_list_present_flag = NULL;
            }
            if(papc->m_pasc->offset_for_ref_frame)
            {
                free(papc->m_pasc->offset_for_ref_frame);
                papc->m_pasc->offset_for_ref_frame = NULL;
            }
            free(papc->m_pasc);
            papc->m_pasc = NULL;
        }
        free(papc);
        papc = *ppapc = NULL;
    }
}

int lbsc_avc_on_parse_nalu(void* powner, int nal_type, const char* pdata, int len)
{
    lbsc_avc_parser_ctx* papc = (lbsc_avc_parser_ctx*)powner;
    int ret= on_parse_nalu(papc->m_pxc, nal_type, pdata, len);
    if(EAVC_NAL_SPS == nal_type)
    {
        ret = lbsc_avc_demux_sps(papc, pdata, len);
    }
    else if(EAVC_NAL_PPS == nal_type)
    {
        ret = lbsc_avc_demux_sps(papc, pdata, len);
    }
    
    return ret;
}



int lbsc_avc_mux_extradata(lbsc_avc_parser_ctx* papc, char* pextradata, int len)
{
    size_t i = 0;
    bitstream_ctx* pbs = papc->m_pxc->m_pbsc;
    initialize(pbs, pextradata, len);
    if (!papc->m_pxc->m_psps || !papc->m_pxc->m_ppps)
    {
        lberror("no sps or pps avaiable, papc->m_pxc->m_psps:%p, papc->m_pxc->m_ppps:%p\n", papc->m_pxc->m_psps, papc->m_pxc->m_ppps);
        return -1;
    }

    if (NULL == pextradata || len < get_extradata_size(papc->m_pxc))
    {
        lberror("not enought buffer for sps and pps, need:%d, have len:%d\n", get_extradata_size(papc->m_pxc), len);
        return -1;
    }

    // 6( + 1) bytes fix header
    write_bits(pbs, 0x01, 8); // version
    write_bits(pbs, papc->m_pasc->profile_idc, 8); // profile
    write_bits(pbs, 0x0, 8);    //
    write_bits(pbs, papc->m_pasc->level_idc, 8); //  level
    write_bits(pbs, 63, 6);        // reserved, all bits on
    write_bits(pbs, 3, 2);            //NALU Length Size minus1
    write_bits(pbs, 7, 3);            // seserved2, all bits on

    write_bits(pbs, 1, 5);            // write sps nalu blocks count(5bit), usually 1
    for (i = 0; i < 1; i++)
    {
        write_bits(pbs, papc->m_pxc->m_nsps_len, 16);    // 32 bits sps len
        write_bytes(pbs, (uint8_t*)papc->m_pxc->m_psps, papc->m_pxc->m_nsps_len);    // write sps data block
    }
    write_bits(pbs, 1, 8);        // write pps nalu block count(8bit), usually 1
    for (i = 0; i < 1; i++)
    {
        write_bits(pbs, papc->m_pxc->m_npps_len, 16);    // 32 bits pps len
        write_bytes(pbs, (uint8_t*)papc->m_pxc->m_ppps, papc->m_pxc->m_npps_len); // write pps data block
    }

    return pbs->bytesoffset;
}

int lbsc_avc_demux_extradata(lbsc_avc_parser_ctx* papc, const char* pextradata, int len)
{
    int i = 0, ret = 0, sps_num = 0, pps_num = 0;
    bitstream_ctx* pbs = papc->m_pxc->m_pbsc;
    //lazy_bitstream bs((void*)pextradata, len);
    if (NULL == pextradata || len <= papc->m_pxc->m_uxvcc_fixed_header_size)
    {
        lberror("extradata not avaiable, pextradata:%p, len:%d <= m_uxvcc_fixed_header_size:%d\n", pextradata, len, (int)papc->m_pxc->m_uxvcc_fixed_header_size);
        return -1;
    }

    uint8_t version = read_bits(pbs, 8);
    if (0x1 != version)
    {
        lberror("Invalid extradata, avcc version:%d\n", (int)version);
        return -1;
    }
    read_byte(pbs, 1); // profile
    read_byte(pbs, 1); // compatibility
    read_byte(pbs, 1); // level
    read_bits(pbs, 6);  // reserved
    read_bits(pbs, 2);  // nal_size_minus1
    read_bits(pbs, 3);     // reserved
    sps_num = (int)read_bits(pbs, 5);
    //m_sps_list.clear();
    for (i = 0; i < sps_num; i++)
    {
        int sps_len = (int)read_byte(pbs, 2);
        ret = lbsc_avc_demux_sps(papc, (char*)pbs->pdata + pbs->bytesoffset, sps_len);
        CHECK_RESULT(ret);
        move(pbs, sps_len, 0);
    }

    pps_num = (int)read_byte(pbs, 1);
    for (i = 0; i < pps_num; i++)
    {
        int pps_len = (int)read_byte(pbs, 2);
        ret = lbsc_avc_demux_pps(papc, (const char*)pbs->pdata + pbs->bytesoffset, pps_len);
        CHECK_RESULT(ret);
        move(pbs, pps_len, 0);
    }
    return ret;
}

int lbsc_avc_demux_sps(lbsc_avc_parser_ctx* papc, const char* pdata, int len)
{
    int sps_len = 0;
    int ret = 0;
    bitstream_ctx* pbs = bitstream_open(pdata, len);//papc->m_pxc->m_pbsc;
    do
    {
        //lazy_xvc_stream bs(4, pdata, len);
        const char* sps = skip_start_code(pdata, len, &sps_len);
        //int sps_len = bs.remain();
        int forbiden = (int)read_bits(pbs, 1);
        int nri = (int)read_bits(pbs, 2);
        int nal_type = (int)read_bits(pbs, 5);

        if (0 != forbiden || EAVC_NAL_SPS != nal_type)
        {
            lberror("Invalid sps nal heaer forbiden:%d, nri:%d, nal_type:%d\n", forbiden, nri, nal_type);
            ret = -1;
            break;
        }
        if (papc->m_pxc->m_psps)
        {
            free(papc->m_pxc->m_psps);
            papc->m_pxc->m_psps = NULL;
            papc->m_pxc->m_nsps_len = 0;
        }
        if(NULL == papc->m_pasc)
        {
            papc->m_pasc = lbsc_open_avc_sps_context();//(lbsc_avc_sps_ctx*)malloc(sizeof(lbsc_avc_sps_ctx));
        }

        //m_psps_ctx = new avc_sps_ctx();
        papc->m_pasc->profile_idc = (uint8_t)read_byte(pbs, 1);
        papc->m_pasc->constraint_set_flag = (uint8_t)read_byte(pbs, 1);
        papc->m_pasc->level_idc = (uint8_t)read_byte(pbs, 1);
        papc->m_pasc->seq_parameter_set_id = (uint8_t)read_ue(pbs);
        papc->m_pasc->chroma_format_idc = (uint8_t)read_ue(pbs);
        if (AVC_PROFILE_HIGHT_FREXT == papc->m_pasc->profile_idc
            || AVC_PROFILE_HIGHT10_FREXT == papc->m_pasc->profile_idc
            || AVC_PROFILE_HIGHT_422_FREXT == papc->m_pasc->profile_idc
            || AVC_PROFILE_HIGHT_444_FREXT == papc->m_pasc->profile_idc)
        {
            papc->m_pasc->chroma_format_idc = (uint8_t)read_ue(pbs);
            if (3 == papc->m_pasc->chroma_format_idc)
            {
                papc->m_pasc->separate_colour_plane_flag = read_bits(pbs, 1);
            }
            papc->m_pasc->bit_depth_luma_minus8 = (uint8_t)read_ue(pbs);
            papc->m_pasc->bit_depth_chroma_minus8 = (uint8_t)read_ue(pbs);
            papc->m_pasc->qpprime_y_zero_transform_bypass_flag = read_bits(pbs, 1);
            papc->m_pasc->seq_scaling_matrix_present_flag = read_bits(pbs, 1);
            if (papc->m_pasc->seq_scaling_matrix_present_flag)
            {
                int len = papc->m_pasc->chroma_format_idc != 3 ? 8 : 12;
                papc->m_pasc->seq_scaling_list_present_flag = (uint8_t*)malloc(len); // uint8_t[len];
                for (int i = 0; i < len; i++)
                {
                    papc->m_pasc->seq_scaling_list_present_flag[i] = read_bits(pbs, 1);
                }
            }
        }
        papc->m_pasc->log2_max_frame_num_minus4 = (uint8_t)read_ue(pbs);
        papc->m_pasc->pic_order_cnt_type = (uint8_t)read_ue(pbs);
        if (papc->m_pasc->pic_order_cnt_type == 0)
        {
            papc->m_pasc->log2_max_pic_order_cnt_lsb_minus4 = (uint8_t)read_ue(pbs);
        }
        else if (papc->m_pasc->pic_order_cnt_type == 1)
        {
            papc->m_pasc->delta_pic_order_always_zero_flag = (uint8_t)read_bits(pbs, 1);
            papc->m_pasc->offset_for_non_ref_pic = (uint8_t)read_se(pbs);
            papc->m_pasc->offset_for_top_to_bottom_field = (uint8_t)read_se(pbs);
            papc->m_pasc->num_ref_frames_in_pic_order_cnt_cycle = (uint8_t)read_ue(pbs);

            //papc->m_pasc->offset_for_ref_frame = new se_size[m_psps_ctx->num_ref_frames_in_pic_order_cnt_cycle];
            for (int i = 0; i < papc->m_pasc->num_ref_frames_in_pic_order_cnt_cycle; i++)
                papc->m_pasc->offset_for_ref_frame[i] = (uint8_t)read_se(pbs);
        }
        read_ue(pbs); //num_ref_frames
        papc->m_pasc->gaps_in_frame_num_value_allowed_flag = read_bits(pbs, 1);//U(1, buf, StartBit);
        int pic_width_in_mbs_minus1 = (uint8_t)read_ue(pbs);//Ue(buf, nLen, StartBit);
        int pic_height_in_map_units_minus1 = (uint8_t)read_ue(pbs);//Ue(buf, nLen, StartBit);

        int encwidth = (pic_width_in_mbs_minus1 + 1) * 16;
        int encheight = (pic_height_in_map_units_minus1 + 1) * 16;

        if (!read_bits(pbs, 1))
            read_bits(pbs, 1);//mb_adaptive_frame_field_flag

        read_bits(pbs, 1); //direct_8x8_inference_flag
        if (read_bits(pbs, 1))
        {
            int frame_crop_left_offset = (int)read_ue(pbs);
            int frame_crop_right_offset = (int)read_ue(pbs);
            int frame_crop_top_offset = (int)read_ue(pbs);
            int frame_crop_bottom_offset = (int)read_ue(pbs);

            //todo: deal with frame_mbs_only_flag
            papc->m_pxc->m_nwidth = encwidth - 2 * frame_crop_left_offset
                - 2 * frame_crop_right_offset;
            papc->m_pxc->m_nheight = encheight - 2 * frame_crop_top_offset
                - 2 * frame_crop_bottom_offset;
        }
        else
        {
            papc->m_pxc->m_nwidth = encwidth;
            papc->m_pxc->m_nheight = encheight;
        }

        if (read_bits(pbs, 1)) // vui_parameters_present_flag
        {
            if (read_bits(pbs, 1)) // aspect_ratio_info_present_flag
            {
                int aspect_ratio_idc = (int)read_byte(pbs, 1);
                if (0xff == aspect_ratio_idc)
                {
                    papc->m_pasc->m_nsar_width = read_bits(pbs, 16);
                    papc->m_pasc->m_nsar_height = read_bits(pbs, 16);
                }
                else if (aspect_ratio_idc < (int)(sizeof(avc_sample_aspect_ratio) / sizeof(struct lbsp_rational)))
                {
                    papc->m_pasc->m_nsar_width = avc_sample_aspect_ratio[aspect_ratio_idc].num;
                    papc->m_pasc->m_nsar_height = avc_sample_aspect_ratio[aspect_ratio_idc].den;
                }
            }
        }

        if (papc->m_pasc->m_nsar_width <= 0 || papc->m_pasc->m_nsar_height <= 0)
        {
            papc->m_pasc->m_nsar_width = 1;
            papc->m_pasc->m_nsar_height = 1;
        }
        ret = add_metadata(papc->m_pxc, sps, sps_len);
    }while(0);

    if(pbs)
    {
        bitstream_close(&pbs);
    }
    return ret;
}

int lbsc_avc_demux_pps(lbsc_avc_parser_ctx* papc, const char* pdata, int len)
{
    assert(papc);
    int nal_len = 0;
    if(papc->m_pxc->m_ppps)
    {
        free(papc->m_pxc->m_ppps);
        papc->m_pxc->m_ppps = NULL;
        papc->m_pxc->m_npps_len = 0;
    }
    pdata = skip_start_code(pdata, len, &nal_len);
    if(NULL == pdata || nal_len <= 0)
    {
        return -1;
    }
    papc->m_pxc->m_ppps = (char*)malloc(nal_len);
    memcpy(papc->m_pxc->m_ppps, pdata, nal_len);
    papc->m_pxc->m_npps_len = nal_len;
    
    return 0;
}

int lbsc_avc_get_extradata_size(lbsc_avc_parser_ctx* papc)
{
    int extradata_size = 0;
    if(papc)
    {
        extradata_size = papc->m_pxc->m_uxvcc_fixed_header_size + (papc->m_pxc->m_uxvcc_fixed_nal_prefix_size + papc->m_pxc->m_uxvcc_fixed_metadata_prefix_size) * 2 + papc->m_pxc->m_nsps_len + papc->m_pxc->m_npps_len;
    }
    
    return extradata_size;
}

// avc demux end
lbsc_hevc_parser_ctx* lbsc_hevc_open_parser(lbsc_xvc_ctx* pxc)
{
    lbsc_hevc_parser_ctx* phpc = (lbsc_hevc_parser_ctx*)malloc(sizeof(lbsc_hevc_parser_ctx));
    if(NULL == pxc)
    {
        phpc->m_pxc = lbsc_open_xvc_context(HEVC_STREAM_TYPE);
    }
    else
    {
        phpc->m_pxc = pxc;
    }
    phpc->m_phvcc = (HEVCDecoderConfigurationRecord*)malloc(sizeof(HEVCDecoderConfigurationRecord));
    lbsc_hevc_reset_parser(phpc);

    return phpc;
}

void lbsc_hevc_close_parser(lbsc_hevc_parser_ctx** pphpc)
{
    if(pphpc && *pphpc)
    {
        lbsc_hevc_parser_ctx* phpc = *pphpc;
        if(phpc->m_pxc)
        {
            lbsc_close_xvc_context(&phpc->m_pxc);
        }
        
        if(phpc->m_phvcc)
        {
            if(phpc->m_phvcc->array)
            {
                free(phpc->m_phvcc->array);
                phpc->m_phvcc->array = NULL;
            }
            free(phpc->m_phvcc);
            phpc->m_phvcc = NULL;
        }
    }
}

void lbsc_hevc_reset_parser(lbsc_hevc_parser_ctx* phpc)
{
    if(NULL == phpc)
    {
        return ;
    }
    memset(phpc->m_phvcc, 0, sizeof(sizeof(HEVCDecoderConfigurationRecord)));
    phpc->m_phvcc->configurationVersion = 1;
    phpc->m_phvcc->lengthSizeMinusOne = 3;
    phpc->m_phvcc->general_profile_compatibility_flags = 0xffffffff;
    phpc->m_phvcc->general_constraint_indicator_flags = 0xffffffffffff;
    phpc->m_phvcc->min_spatial_segmentation_idc = MAX_SPATIAL_SEGMENTATION + 1;
    
    phpc->m_nsar_width    = 0;
    phpc->m_nsar_height   = 0;
    phpc->m_nwidth        = 0;
    phpc->m_nheight       = 0;
}

int lbsc_hevc_on_parser_nalu(lbsc_hevc_parser_ctx* phpc, int nal_type, const char* pdata, int len)
{
    int ret = 0;
    if(NULL == phpc || NULL == phpc->m_pxc)
    {
        return -1;
    }
    if (phpc->m_pxc->m_unal_type_vps == nal_type)
    {
        ret = lbsc_hevc_demux_vps(phpc, pdata, len);
        CHECK_RESULT(ret);
    }
    else if(phpc->m_pxc->m_unal_type_sps == nal_type)
    {
        ret = lbsc_hevc_demux_sps(phpc, pdata, len);
        CHECK_RESULT(ret);
    }
    else if (phpc->m_pxc->m_unal_type_pps == nal_type)
    {
        ret = lbsc_hevc_demux_pps(phpc, pdata, len);
        CHECK_RESULT(ret);
    }
    else if (nal_type <= 22)
    {
        // hevc packet come
        return 1;
    }
    return ret;
}

int lbsc_hevc_demux_vps(lbsc_hevc_parser_ctx* phpc, const char* vps, int vps_len)
{
    int ret = 0;
    unsigned int vps_max_sub_layers_minus1;
    int len = 0;
    int nal_type = 0;
    //lazy_xvc_stream bs(5);
    vps = skip_start_code(vps, vps_len, &len);
    char* vps_rbsp = rbsp_from_nalu(vps, len, &vps_len);
    if(NULL == vps_rbsp)
    {
        lberror("vps_rbsp:%p = rbsp_from_nalu(vps:%p, len:%d, &vps_len:%d) failed\n", vps_rbsp, vps, len, vps_len);
        return -1;
    }
    bitstream_ctx* pbs = bitstream_open(vps_rbsp, vps_len);//phpc->m_pxc->m_pbsc;
    //ret = initialize(pbs, vps_rbsp, vps_len);
    CHECK_RESULT(ret);
    do{
        nal_type = read_nal_type(pbs, phpc->m_pxc->m_nstream_type);
        if (HEVC_NAL_VPS != nal_type)
        {
            lberror("Invalid hevc vps nal_type:%d\n", nal_type);
            lbmemory(vps, vps_len);
            ret = -1;
            break;
        }
        /*
        * vps_video_parameter_set_id u(4)
        * vps_reserved_three_2bits   u(2)
        * vps_max_layers_minus1      u(6)
        */
        read_bits(pbs, 4); // vps_video_parameter_set_id
        read_bits(pbs, 2); // vps_reserved_three_2bits
        read_bits(pbs, 6); // vps_max_layers_minus1
        
        vps_max_sub_layers_minus1 = (int)read_bits(pbs, 3);//get_bits(gb, 3);

                                                   /*
                                                   * numTemporalLayers greater than 1 indicates that the stream to which this
                                                   * configuration record applies is temporally scalable and the contained
                                                   * number of temporal layers (also referred to as temporal sub-layer or
                                                   * sub-layer in ISO/IEC 23008-2) is equal to numTemporalLayers. Value 1
                                                   * indicates that the stream is not temporally scalable. Value 0 indicates
                                                   * that it is unknown whether the stream is temporally scalable.
                                                   */
        phpc->m_phvcc->numTemporalLayers = LBMAX(phpc->m_phvcc->numTemporalLayers, vps_max_sub_layers_minus1 + 1);

        /*
        * vps_temporal_id_nesting_flag u(1)
        * vps_reserved_0xffff_16bits   u(16)
        */
        read_bits(pbs, 1); // vps_temporal_id_nesting_flag
        read_bits(pbs, 16); // vps_reserved_0xffff_16bits

        lbsc_hevc_parse_ptl(phpc, pbs, vps_max_sub_layers_minus1);
        
        ret = add_metadata(phpc->m_pxc, vps, len);
    }while(0);

    if(pbs)
    {
        bitstream_close(&pbs);
    }
    if(vps_rbsp)
    {
        free(vps_rbsp);
        vps_rbsp = NULL;
    }
    return ret;
}

int lbsc_hevc_demux_sps(lbsc_hevc_parser_ctx* phpc, const char* sps, int sps_len)
{
    int ret = 0;
    int len = 0;
    unsigned int i = 0, sps_max_sub_layers_minus1 = 0, log2_max_pic_order_cnt_lsb_minus4 = 0, separate_colour_plane_flag = 0;
    unsigned int num_short_term_ref_pic_sets = 0, num_delta_pocs[HEVC_MAX_SHORT_TERM_REF_PIC_SETS];
    if(NULL == phpc || NULL == sps)
    {
        lberror("Invalid parameter, phpc:%p, sps:%p\n", phpc, sps);
        return -1;
    }
    
    
    //bitstream_ctx* pbs = phpc->m_pxc->m_pbsc;
    sps = skip_start_code(sps, sps_len, &len);
    char* sps_rbsp = rbsp_from_nalu(sps, len, &sps_len);
    if(NULL == sps_rbsp)
    {
        lberror("sps_rbsp:%p = rbsp_from_nalu(sps:%p, len:%d, &sps_len:%d) failed\n", sps_rbsp, sps, len, sps_len);
        return -1;
    }
    bitstream_ctx* pbs = bitstream_open(sps_rbsp, sps_len);
    //ret = initialize(pbs, sps_rbsp, sps_len);
    do
    {
        int nal_type = read_nal_type(pbs, phpc->m_pxc->m_nstream_type);
        if (HEVC_NAL_SPS != nal_type)
        {
            lberror("Invalid hevc sps nal_type:%d\n", nal_type);
            lbmemory(sps, sps_len);
            ret = -1;
            break;
        }
        read_bits(pbs, 4); // sps_video_parameter_set_id
        sps_max_sub_layers_minus1 = read_bits(pbs, 3);
        phpc->m_phvcc->numTemporalLayers = LBMAX(phpc->m_phvcc->numTemporalLayers, sps_max_sub_layers_minus1 + 1);
        phpc->m_phvcc->temporalIdNested = read_bits(pbs, 1);
        lbsc_hevc_parse_ptl(phpc, pbs, sps_max_sub_layers_minus1);
        read_ue(pbs); // sps_seq_parameter_set_id
        phpc->m_phvcc->chromaFormat = read_ue(pbs); // pitcure color space, 1 indicate 4:2:0(yuv420)
        if (3 == phpc->m_phvcc->chromaFormat)
        {
            separate_colour_plane_flag = read_bits(pbs, 1); // separate_colour_plane_flag, specity for solor space 4:4:4
        }
        phpc->m_pxc->m_nwidth = phpc->m_nwidth = read_ue(pbs); // pic_width_in_luma_samples
        phpc->m_pxc->m_nheight = phpc->m_nheight = read_ue(pbs); // pic_height_in_luma_samples

        if (read_bits(pbs, 1)) // conformance_window_flag
        {
            int conf_win_left_offset = 0, conf_win_right_offset = 0, conf_win_top_offset = 0, conf_win_bottom_offset = 0;
            int sub_width_c = ((1 == phpc->m_phvcc->chromaFormat) || (2 == phpc->m_phvcc->chromaFormat)) && (0 == separate_colour_plane_flag) ? 2 : 1;
            int sub_height_c = (1 == phpc->m_phvcc->chromaFormat) && (0 == separate_colour_plane_flag) ? 2 : 1;

            conf_win_left_offset = read_ue(pbs);    // conf_win_left_offset
            conf_win_right_offset = read_ue(pbs);    // conf_win_right_offset
            conf_win_top_offset = read_ue(pbs);    // conf_win_top_offset
            conf_win_bottom_offset = read_ue(pbs);    // conf_win_bottom_offset
            phpc->m_nwidth -= (sub_width_c*conf_win_right_offset + sub_width_c*conf_win_left_offset);
            phpc->m_nheight -= (sub_height_c*conf_win_bottom_offset + sub_height_c*conf_win_top_offset);
            lbtrace("parser sps width:%d, height:%d\n", phpc->m_nwidth, phpc->m_nheight);
        }

        phpc->m_phvcc->bitDepthLumaMinus8 = read_ue(pbs);            // luminance/(luma/brightness(Y)
        phpc->m_phvcc->bitDepthChromaMinus8 = read_ue(pbs);        // chroma
        log2_max_pic_order_cnt_lsb_minus4 = read_ue(pbs);
        // sps_sub_layer_ordering_info_present_flag
        i = read_bits(pbs, 1) ? 0 : sps_max_sub_layers_minus1;
        for (; i <= sps_max_sub_layers_minus1; i++)
        {
            read_ue(pbs);    // max_dec_pic_buffering_minus1
            read_ue(pbs);    // max_num_reorder_pics
            read_ue(pbs);    // max_latency_increase_plus1
        }
        read_ue(pbs);    // log2_min_luma_coding_block_size_minus3
        read_ue(pbs);    // log2_diff_max_min_luma_coding_block_size
        read_ue(pbs);    // log2_min_transform_block_size_minus2
        read_ue(pbs);    // log2_diff_max_min_transform_block_size
        read_ue(pbs);    // max_transform_hierarchy_depth_inter
        read_ue(pbs);    // max_transform_hierarchy_depth_intra

        if (read_bits(pbs, 1) && read_bits(pbs, 1)) // scaling_list_enabled_flag && sample_adaptive_offset_enabled_flag
        {
            int i = 0, j = 0, k = 0, num_coeffs = 0;

            for (i = 0; i < 4; i++)
                for (j = 0; j < (i == 3 ? 2 : 6); j++)
                    if (!read_bits(pbs, 1))         // scaling_list_pred_mode_flag[i][j]
                        read_ue(pbs); // scaling_list_pred_matrix_id_delta[i][j]
                    else {
                        num_coeffs = LBMIN(64, 1 << (4 + (i << 1)));

                        if (i > 1)
                            read_se(pbs); // scaling_list_dc_coef_minus8[i-2][j]

                        for (k = 0; k < num_coeffs; k++)
                            read_se(pbs); // scaling_list_delta_coef
                    }
        }
        read_bits(pbs, 1);    // amp_enabled_flag
        read_bits(pbs, 1);    // sample_adaptive_offset_enabled_flag

        if (read_bits(pbs, 1))    // pcm_enabled_flag
        {
            read_bits(pbs, 4);    // pcm_sample_bit_depth_luma_minus1
            read_bits(pbs, 4);    // pcm_sample_bit_depth_chroma_minus1
            read_ue(pbs);    // log2_min_pcm_luma_coding_block_size_minus3
            read_ue(pbs);    // log2_diff_max_min_pcm_luma_coding_block_size
            read_bits(pbs, 1); // pcm_loop_filter_disabled_flag
        }

        num_short_term_ref_pic_sets = read_ue(pbs);
        if (num_short_term_ref_pic_sets > HEVC_MAX_SHORT_TERM_REF_PIC_SETS)
        {
            lberror("Invalid hevc data, num_short_term_ref_pic_sets:%d\n", num_short_term_ref_pic_sets);
            return -1;
        }

        for (i = 0; i < num_short_term_ref_pic_sets; i++)
        {
            int ret = lbsc_hevc_parse_rps(phpc, pbs, i, num_short_term_ref_pic_sets, num_delta_pocs);
            if (ret < 0)
            {
                lberror("ret:%d = parser_rps(&bs, i:%u, num_short_term_ref_pic_sets:%u, num_delta_pocs:%p)\n", ret, i, num_short_term_ref_pic_sets, num_delta_pocs);
                return ret;
            }
        }

        if (read_bits(pbs, 1))
        {
            unsigned num_long_term_ref_pics_sps = read_ue(pbs);
            if (num_long_term_ref_pics_sps > 31U)
            {
                lberror("Invalid sps data, num_long_term_ref_pics_sps:%d > 31U\n", num_long_term_ref_pics_sps);
                return -1;
            }
            for (i = 0; i < num_long_term_ref_pics_sps; i++) { // num_long_term_ref_pics_sps
                int len = LBMIN(log2_max_pic_order_cnt_lsb_minus4 + 4, 16);
                read_bits(pbs, len);    // lt_ref_pic_poc_lsb_sps[i]
                read_bits(pbs, 1);      // used_by_curr_pic_lt_sps_flag[i]
            }
        }

        read_bits(pbs, 1);    // sps_temporal_mvp_enabled_flag
        read_bits(pbs, 1); // strong_intra_smoothing_enabled_flag

        if (read_bits(pbs, 1)) // vui_parameters_present_flag
        {
            unsigned int min_spatial_segmentation_idc;

            if (read_bits(pbs, 1))              // aspect_ratio_info_present_flag
                if (read_bits(pbs, 8) == 255)    // aspect_ratio_idc
                    read_bits(pbs, 32);        // sar_width u(16), sar_height u(16)

            if (read_bits(pbs, 1))                // overscan_info_present_flag
                read_bits(pbs, 1);                // overscan_appropriate_flag

            if (read_bits(pbs, 1)) {            // video_signal_type_present_flag
                read_bits(pbs, 4);                // video_format u(3), video_full_range_flag u(1)

                if (read_bits(pbs, 1))            // colour_description_present_flag
                                            /*
                                            * colour_primaries         u(8)
                                            * transfer_characteristics u(8)
                                            * matrix_coeffs            u(8)
                                            */
                    read_bits(pbs, 24);
            }

            if (read_bits(pbs, 1)) {        // chroma_loc_info_present_flag
                read_ue(pbs); // chroma_sample_loc_type_top_field
                read_ue(pbs); // chroma_sample_loc_type_bottom_field
            }

            /*
            * neutral_chroma_indication_flag u(1)
            * field_seq_flag                 u(1)
            * frame_field_info_present_flag  u(1)
            */
            read_bits(pbs, 1); // neutral_chroma_indication_flag
            read_bits(pbs, 1); // field_seq_flag
            read_bits(pbs, 1); // frame_field_info_present_flag
            //int default_display_window_flag = read_bits(pbs,1);
            if (read_bits(pbs, 1)) {        // default_display_window_flag
                read_ue(pbs); // def_disp_win_left_offset
                read_ue(pbs); // def_disp_win_right_offset
                read_ue(pbs); // def_disp_win_top_offset
                read_ue(pbs); // def_disp_win_bottom_offset
            }

            if (read_bits(pbs, 1)) { // vui_timing_info_present_flag
                                  //skip_timing_info(gb);
                read_bits(pbs, 32);
                read_bits(pbs, 32);
                if (read_bits(pbs, 1))
                {
                    read_ue(pbs);
                }

                if (read_bits(pbs, 1)) // vui_hrd_parameters_present_flag
                    lbsc_hevc_skip_hrd_parameters(phpc, pbs, 1, sps_max_sub_layers_minus1);
            }

            if (read_bits(pbs, 1)) { // bitstream_restriction_flag
                                  /*
                                  * tiles_fixed_structure_flag              u(1)
                                  * motion_vectors_over_pic_boundaries_flag u(1)
                                  * restricted_ref_pic_lists_flag           u(1)
                                  */
                read_bits(pbs, 3);

                min_spatial_segmentation_idc = read_ue(pbs);

                /*
                * unsigned int(12) min_spatial_segmentation_idc;
                *
                * The min_spatial_segmentation_idc indication must indicate a level of
                * spatial segmentation equal to or less than the lowest level of
                * spatial segmentation indicated in all the parameter sets.
                */
                phpc->m_phvcc->min_spatial_segmentation_idc = LBMIN(phpc->m_phvcc->min_spatial_segmentation_idc,
                    min_spatial_segmentation_idc);

                read_ue(pbs); // max_bytes_per_pic_denom
                read_ue(pbs); // max_bits_per_min_cu_denom
                read_ue(pbs); // log2_max_mv_length_horizontal
                read_ue(pbs); // log2_max_mv_length_vertical
            }
        }
        ret = add_metadata(phpc->m_pxc, sps, len);
        //add_xps(HEVC_NAL_SPS, sps, sps_len);
        return 0;
    }while(0);
    if(pbs)
    {
        bitstream_close(&pbs);
    }
    if(sps_rbsp)
    {
        free(sps_rbsp);
        sps_rbsp = NULL;
    }
    return ret;
}

int lbsc_hevc_demux_pps(lbsc_hevc_parser_ctx* phpc, const char* pps, int pps_len)
{
    int ret = 0;
    int len = 0;
    uint8_t tiles_enabled_flag = 0, entropy_coding_sync_enabled_flag = 0;
    uint8_t nal_type = 0;
    if(NULL == phpc || NULL == pps)
    {
        lberror("Invalid parameter, phpc:%p, pps:%p\n", phpc, pps);
        return -1;
    }
    //bitstream_ctx* pbs = phpc->m_pxc->m_pbsc;
    pps = skip_start_code(pps, pps_len, &len);
    char* pps_rbsp = rbsp_from_nalu(pps, len, &pps_len);
    if(NULL == pps_rbsp)
    {
        lberror("pps_rbsp:%p = rbsp_from_nalu(pps:%p, len:%d, &pps_len:%d) failed\n", pps_rbsp, pps, len, pps_len);
        return -1;
    }
    bitstream_ctx* pbs = bitstream_open(pps_rbsp, pps_len);
    //ret = initialize(pbs, pps_rbsp, pps_len);
    CHECK_RESULT(ret);
    do
    {
        nal_type = read_nal_type(pbs, phpc->m_pxc->m_nstream_type);
        if (HEVC_NAL_PPS != nal_type)
        {
            lberror("Invalid hevc pps nal_type:%d\n", nal_type);
            lbmemory(pps, pps_len);
            ret = -1;
            break;
        }
        read_ue(pbs); // pps_pic_parameter_set_id
        read_ue(pbs); // pps_seq_parameter_set_id

          /*
          * dependent_slice_segments_enabled_flag u(1)
          * output_flag_present_flag              u(1)
          * num_extra_slice_header_bits           u(3)
          * sign_data_hiding_enabled_flag         u(1)
          * cabac_init_present_flag               u(1)
          */
        read_bits(pbs,7);

        read_ue(pbs); // num_ref_idx_l0_default_active_minus1
        read_ue(pbs); // num_ref_idx_l1_default_active_minus1
        read_se(pbs); // init_qp_minus26

                      /*
                      * constrained_intra_pred_flag u(1)
                      * transform_skip_enabled_flag u(1)
                      */
        read_bits(pbs,2);

        if (read_bits(pbs,1))          // cu_qp_delta_enabled_flag
            read_ue(pbs); // diff_cu_qp_delta_depth

        read_se(pbs); // pps_cb_qp_offset
        read_se(pbs); // pps_cr_qp_offset

                      /*
                      * pps_slice_chroma_qp_offsets_present_flag u(1)
                      * weighted_pred_flag               u(1)
                      * weighted_bipred_flag             u(1)
                      * transquant_bypass_enabled_flag   u(1)
                      */
        read_bits(pbs,4);

        tiles_enabled_flag = read_bits(pbs,1);
        entropy_coding_sync_enabled_flag = read_bits(pbs,1);

        if (entropy_coding_sync_enabled_flag && tiles_enabled_flag)
            phpc->m_phvcc->parallelismType = 0; // mixed-type parallel decoding
        else if (entropy_coding_sync_enabled_flag)
            phpc->m_phvcc->parallelismType = 3; // wavefront-based parallel decoding
        else if (tiles_enabled_flag)
            phpc->m_phvcc->parallelismType = 2; // tile-based parallel decoding
        else
            phpc->m_phvcc->parallelismType = 1; // slice-based parallel decoding

                                          /* nothing useful for hvcC past this point */
        add_metadata(phpc->m_pxc, pps, pps_len);
        //add_xps(HEVC_NAL_PPS, pps, pps_len);
        ret = 0;
    }while(0);
    
    if(pbs)
    {
        bitstream_close(&pbs);
    }
    
    if(pps_rbsp)
    {
        free(pps_rbsp);
        pps_rbsp = NULL;
    }
    return ret;
}

int lbsc_hevc_mux_hvcc(lbsc_hevc_parser_ctx* phpc, char* pdata, int len)
{
    if(NULL == phpc || NULL == pdata || len < 0)
    {
        lberror("Invalid parameter, phpc:%p, pps:%p, len:%d\n", phpc, pdata, len);
        return -1;
    }
    
    bitstream_ctx* pbs = phpc->m_pxc->m_pbsc;
    int ret = initialize(pbs, pdata, len);
    CHECK_RESULT(ret);

    int need_bytes = lbsc_get_mux_hvcc_size(phpc);
    if (len < need_bytes)
    {
        lberror("not enought buffer for hvcc metadata, need %d, have %d\n", need_bytes, len);
        return -1;
    }
    /*
    * We only support writing HEVCDecoderConfigurationRecord version 1.
    */
    phpc->m_phvcc->configurationVersion = 1;

    /*
    * If min_spatial_segmentation_idc is invalid, reset to 0 (unspecified).
    */
    if (phpc->m_phvcc->min_spatial_segmentation_idc > MAX_SPATIAL_SEGMENTATION)
        phpc->m_phvcc->min_spatial_segmentation_idc = 0;

    /*
    * parallelismType indicates the type of parallelism that is used to meet
    * the restrictions imposed by min_spatial_segmentation_idc when the value
    * of min_spatial_segmentation_idc is greater than 0.
    */
    if (!phpc->m_phvcc->min_spatial_segmentation_idc)
        phpc->m_phvcc->parallelismType = 0;

    /*
    * It's unclear how to properly compute these fields, so
    * let's always set them to values meaning 'unspecified'.
    */
    phpc->m_phvcc->avgFrameRate = 0;
    phpc->m_phvcc->constantFrameRate = 0;
    phpc->m_phvcc->numOfArrays = 3;
    write_bits(pbs, phpc->m_phvcc->configurationVersion, 8);
    write_bits(pbs, phpc->m_phvcc->general_profile_space, 2);
    write_bits(pbs, phpc->m_phvcc->general_tier_flag, 1);
    write_bits(pbs, phpc->m_phvcc->general_profile_idc, 5);
    write_bits(pbs, phpc->m_phvcc->general_profile_compatibility_flags, 32);
    write_bits(pbs, phpc->m_phvcc->general_constraint_indicator_flags, 48);

    write_bits(pbs, phpc->m_phvcc->general_level_idc, 8);// 13
    write_bits(pbs, 0xf, 4);
    write_bits(pbs, phpc->m_phvcc->min_spatial_segmentation_idc, 12);
    write_bits(pbs, 0x3f, 6);
    write_bits(pbs, phpc->m_phvcc->parallelismType, 2);
    write_bits(pbs, 0x3f, 6);
    write_bits(pbs, phpc->m_phvcc->chromaFormat, 2);
    write_bits(pbs, 0x1f, 5);
    write_bits(pbs, phpc->m_phvcc->bitDepthLumaMinus8, 3);
    write_bits(pbs, 0x1f, 5);
    write_bits(pbs, phpc->m_phvcc->bitDepthChromaMinus8, 3);
    write_bits(pbs, phpc->m_phvcc->avgFrameRate, 16);

    write_bits(pbs, phpc->m_phvcc->constantFrameRate, 2);
    write_bits(pbs, phpc->m_phvcc->numTemporalLayers, 3);
    write_bits(pbs, phpc->m_phvcc->temporalIdNested, 1);
    write_bits(pbs, phpc->m_phvcc->lengthSizeMinusOne, 2);

    write_bits(pbs, phpc->m_phvcc->numOfArrays, 8);//23

    ret = lbsc_hevc_write_xps(phpc, pbs);
    CHECK_RESULT(ret);

    return pbs->bytesoffset;
}

int lbsc_hevc_demux_hvcc(lbsc_hevc_parser_ctx* phpc, char* pdata, int len)
{
    if(NULL == phpc || NULL == pdata || len < 0)
    {
        lberror("Invalid parameter, phpc:%p, pps:%p, len:%d\n", phpc, pdata, len);
        return -1;
    }

    bitstream_ctx* pbs = phpc->m_pxc->m_pbsc;
    int ret = initialize(pbs, pdata, len);
    CHECK_RESULT(ret);

    phpc->m_phvcc->configurationVersion = read_bits(pbs, 8);
    phpc->m_phvcc->general_profile_space = read_bits(pbs, 2);
    phpc->m_phvcc->general_tier_flag = read_bits(pbs, 1);
    phpc->m_phvcc->general_profile_idc = read_bits(pbs, 5);
    phpc->m_phvcc->general_profile_compatibility_flags = (int)read_bits(pbs, 32);
    phpc->m_phvcc->general_constraint_indicator_flags = read_bits(pbs, 48);

    phpc->m_phvcc->general_level_idc = read_bits(pbs, 8);// 13
    read_bits(pbs, 4);
    phpc->m_phvcc->min_spatial_segmentation_idc = read_bits(pbs, 12);
    read_bits(pbs, 6);
    phpc->m_phvcc->parallelismType = read_bits(pbs, 2);
    read_bits(pbs, 6);
    phpc->m_phvcc->chromaFormat = read_bits(pbs, 2);
    read_bits(pbs, 5);
    phpc->m_phvcc->bitDepthLumaMinus8 = read_bits(pbs, 3);
    read_bits(pbs, 5);
    phpc->m_phvcc->bitDepthChromaMinus8 = read_bits(pbs, 3);
    phpc->m_phvcc->avgFrameRate = read_bits(pbs, 16);

    phpc->m_phvcc->constantFrameRate = read_bits(pbs, 2);
    phpc->m_phvcc->numTemporalLayers = read_bits(pbs, 3);
    phpc->m_phvcc->temporalIdNested = read_bits(pbs, 1);
    phpc->m_phvcc->lengthSizeMinusOne = read_bits(pbs, 2);

    phpc->m_phvcc->numOfArrays = read_bits(pbs, 8);

    //dump_hvcc();
    lbsc_reset_xvc_context(phpc->m_pxc);

    ret = lbsc_hevc_read_xps(phpc, pbs);
    CHECK_RESULT(ret);

    return ret;
}

int lbsc_hevc_write_xps(lbsc_hevc_parser_ctx* phpc, bitstream_ctx* pbsc)
{
    size_t i = 0;
    if(NULL == phpc || !phpc->m_pxc->m_pbsc)
    {
        lberror("write xps failed, no enought memory buffer, phpc:%p, phpc->m_pxc->m_pbsc:%p\n", phpc, phpc->m_pxc->m_pbsc);
        return -1;
    }
    bitstream_ctx* pbs = phpc->m_pxc->m_pbsc;
    if(pbsc)
    {
        pbs = pbsc;
    }

    write_bits(pbs, 0, 1);
    write_bits(pbs, 0, 1);
    write_bits(pbs, HEVC_NAL_VPS, 6);
    write_bits(pbs, 1, 16);
    for (i = 0; i < 1; i++)
    {
        write_bits(pbs, phpc->m_pxc->m_nvps_len, 16);
        write_bytes(pbs, phpc->m_pxc->m_pvps, phpc->m_pxc->m_nvps_len);
    }

    write_bits(pbs, 0, 1);
    write_bits(pbs, 0, 1);
    write_bits(pbs, HEVC_NAL_SPS, 6);
    write_bits(pbs, 1, 16);
    for (i = 0; i < 1; i++)
    {
        write_bits(pbs, phpc->m_pxc->m_nsps_len, 16);
        write_bytes(pbs, phpc->m_pxc->m_psps, phpc->m_pxc->m_nsps_len);
    }

    write_bits(pbs, 0, 1);
    write_bits(pbs, 0, 1);
    write_bits(pbs, HEVC_NAL_PPS, 6);
    write_bits(pbs, 1, 16);
    for (i = 0; i < 1; i++)
    {
        write_bits(pbs, phpc->m_pxc->m_npps_len, 16);
        write_bytes(pbs, phpc->m_pxc->m_ppps, phpc->m_pxc->m_npps_len);
    }
    return 0;
}

int lbsc_hevc_read_xps(lbsc_hevc_parser_ctx* phpc, bitstream_ctx* pbsc)
{
    int ret = 0;
    if (NULL == phpc || !phpc->m_pxc->m_pbsc)
    {
        lberror("read xps failed, Invalid parameter, phpc:%p, phpc->m_pxc->m_pbsc:%p\n", phpc, phpc->m_pxc->m_pbsc);
        return -1;
    }
    bitstream_ctx* pbs = phpc->m_pxc->m_pbsc;
    if(pbsc)
    {
        pbs = pbsc;
    }
    int num_of_array = 3;// m_phvcc->numOfArrays;
    while (num_of_array > 0)
    {
        read_bits(pbs, 1); //array_completeness
        read_bits(pbs, 1); //reserved
        uint8_t nal_type = read_bits(pbs, 6); // nal_type
        int numNalus = read_bits(pbs, 16);
        /*if (HEVC_NAL_VPS == nal_type)
        {
            pxps_list = &m_vps_list;
        }
        else if (HEVC_NAL_SPS == nal_type)
        {
            pxps_list = &m_sps_list;
        }
        else if (HEVC_NAL_PPS == nal_type)
        {
            pxps_list = &m_pps_list;
        }
        else
        {
            lberror("Invalid hevc nal type %d\n", nal_type);
            return -1;
        }
        pxps_list->clear();*/
        for (int i = 0; i < numNalus; i++)
        {
            //string nal_str;
            int nal_size = (int)read_bits(pbs, 16);
            char nal_buf[256] = { 0 };
            ret = read_bytes(pbs, nal_buf, nal_size);
            //ret = pbs->read_bytes((uint8_t*)nal_buf, nal_size);
            CHECK_RESULT(ret);

            //nal_str.append((const char*)nal_buf, nal_size);
            //pxps_list->push_back(nal_str);
            if (HEVC_NAL_VPS == nal_type)
            {
                ret = lbsc_hevc_demux_vps(phpc, nal_buf, nal_size);
                CHECK_RESULT(ret);
            }
            else if (HEVC_NAL_SPS == nal_type)
            {
                ret = lbsc_hevc_demux_sps(phpc, nal_buf, nal_size);
                CHECK_RESULT(ret);
            }
            else if (HEVC_NAL_PPS == nal_type)
            {
                ret = lbsc_hevc_demux_pps(phpc, nal_buf, nal_size);
                CHECK_RESULT(ret);
            }

        }
        num_of_array--;
    }

    return ret;
}

int lbsc_hevc_parse_rps(lbsc_hevc_parser_ctx* phpc, bitstream_ctx* pbsc, unsigned int rps_idx, unsigned int num_rps, unsigned int* num_delta_pocs)
{
    int ret = 0;
    unsigned int i = 0;
    if (NULL == phpc || !phpc->m_pxc->m_pbsc)
    {
        lberror("read xps failed, Invalid parameter, phpc:%p, phpc->m_pxc->m_pbsc:%p\n", phpc, phpc->m_pxc->m_pbsc);
        return -1;
    }
    bitstream_ctx* pbs = phpc->m_pxc->m_pbsc;
    if(pbsc)
    {
        pbs = pbsc;
    }
    if (rps_idx && read_bits(pbs, 1)) // inter_ref_pic_set_prediction_flag
    {
        if (rps_idx >= num_rps)
        {
            lberror("Invalid hevc data, rps_idx:%u >= num_rps:%u\n", rps_idx, num_rps);
            return -1;
        }
        read_bits(pbs, 1);    //    delta_rps_sign
        read_ue(pbs);        //    abs_delta_rps_minus1

        num_delta_pocs[rps_idx] = 0;

        for (i = 0; i <= num_delta_pocs[rps_idx - 1]; i++)
        {
            uint8_t use_delta_flag = 0;
            uint8_t used_by_curr_pic_flag = read_bits(pbs, 1);
            if (!used_by_curr_pic_flag)
                use_delta_flag = read_bits(pbs, 1);

            if (used_by_curr_pic_flag || use_delta_flag)
                num_delta_pocs[rps_idx]++;
        }
    }
    else
    {
        unsigned int num_negative_pics = (unsigned int)read_ue(pbs);
        unsigned int num_positive_pics = (unsigned int)read_ue(pbs);

        if(pbs->datalen - pbs->bytesoffset < (num_positive_pics + (uint64_t)num_negative_pics) * 2)
        {
            //lberror("Invalid hevc data, require bits %" PRId64 " have:%d, not enught\n", (num_positive_pics + (int64_t)num_negative_pics) * 2, pbs->datalen - pbs->bytesoffset);
            return -1;
        }

        num_delta_pocs[rps_idx] = num_negative_pics + num_positive_pics;

        for (i = 0; i < num_negative_pics; i++) {
            read_ue(pbs); // delta_poc_s0_minus1[rps_idx]
            read_bits(pbs, 1); // used_by_curr_pic_s0_flag[rps_idx]
        }

        for (i = 0; i < num_positive_pics; i++) {
            read_ue(pbs); // delta_poc_s1_minus1[rps_idx]
            read_bits(pbs, 1); // used_by_curr_pic_s1_flag[rps_idx]
        }
    }
    return 0;
}

void lbsc_hevc_parse_vui(lbsc_hevc_parser_ctx* phpc, bitstream_ctx* pbsc, unsigned int max_sub_layers_minus1)
{
    unsigned int min_spatial_segmentation_idc;
    if (NULL == phpc || !phpc->m_pxc || phpc->m_pxc->m_pbsc)
    {
        if(NULL == phpc)
        {
            lberror("read xps failed, Invalid parameter, phpc:%p\n", phpc);
        }
        else if(NULL == phpc->m_pxc)
        {
            lberror("read xps failed, Invalid parameter, phpc->m_pxc:%p\n", phpc->m_pxc);
        }
        else if(NULL == phpc->m_pxc->m_pbsc)
        {
            lberror("read xps failed, Invalid parameter, phpc->m_pxc->m_pbsc:%p\n", phpc->m_pxc->m_pbsc);
        }
        
        return ;
    }
    bitstream_ctx* pbs = phpc->m_pxc->m_pbsc;
    if(pbsc)
    {
        pbs = pbsc;
    }
    
    if (read_bits(pbs, 1))              // aspect_ratio_info_present_flag
        if (read_bits(pbs, 8) == 255) // aspect_ratio_idc
            phpc->m_nsar_width = read_byte(pbs, 2);    // sar_width u(16)
    phpc->m_nsar_height = read_byte(pbs, 2);    // sar_height u(16)

    if (read_bits(pbs, 1))  // overscan_info_present_flag
        read_bits(pbs, 1); // overscan_appropriate_flag

    if (read_bits(pbs, 1)) {  // video_signal_type_present_flag
        read_bits(pbs, 4); // video_format u(3), video_full_range_flag u(1)

        if (read_bits(pbs, 1)) // colour_description_present_flag
                              /*
                              * colour_primaries         u(8)
                              * transfer_characteristics u(8)
                              * matrix_coeffs            u(8)
                              */
            read_byte(pbs, 3);
    }

    if (read_bits(pbs, 1)) {        // chroma_loc_info_present_flag
        read_ue(pbs); // chroma_sample_loc_type_top_field
        read_ue(pbs); // chroma_sample_loc_type_bottom_field
    }

    /*
    * neutral_chroma_indication_flag u(1)
    * field_seq_flag                 u(1)
    * frame_field_info_present_flag  u(1)
    */
    read_bits(pbs, 3);

    if (read_bits(pbs, 1)) {        // default_display_window_flag
        read_ue(pbs); // def_disp_win_left_offset
        read_ue(pbs); // def_disp_win_right_offset
        read_ue(pbs); // def_disp_win_top_offset
        read_ue(pbs); // def_disp_win_bottom_offset
    }

    if (read_bits(pbs, 1)) { // vui_timing_info_present_flag
                            //skip_timing_info(gb);
        int num_units_in_tick = read_byte(pbs, 4);    //
        float time_scale = read_byte(pbs, 4);    // time_scale
        phpc->m_fframe_rate = time_scale/(2*num_units_in_tick);
        if (read_bits(pbs, 1))          // poc_proportional_to_timing_flag
            read_ue(pbs); // num_ticks_poc_diff_one_minus1

        if (read_bits(pbs, 1)) // vui_hrd_parameters_present_flag
            lbsc_hevc_skip_hrd_parameters(phpc, pbs, 1, max_sub_layers_minus1);
    }

    if (read_bits(pbs, 1)) { // bitstream_restriction_flag
                            /*
                            * tiles_fixed_structure_flag              u(1)
                            * motion_vectors_over_pic_boundaries_flag u(1)
                            * restricted_ref_pic_lists_flag           u(1)
                            */
        read_bits(pbs, 3);

        min_spatial_segmentation_idc = read_ue(pbs);

        /*
        * unsigned int(12) min_spatial_segmentation_idc;
        *
        * The min_spatial_segmentation_idc indication must indicate a level of
        * spatial segmentation equal to or less than the lowest level of
        * spatial segmentation indicated in all the parameter sets.
        */
        phpc->m_phvcc->min_spatial_segmentation_idc = LBMIN(phpc->m_phvcc->min_spatial_segmentation_idc,
            min_spatial_segmentation_idc);

        read_ue(pbs); // max_bytes_per_pic_denom
        read_ue(pbs); // max_bits_per_min_cu_denom
        read_ue(pbs); // log2_max_mv_length_horizontal
        read_ue(pbs); // log2_max_mv_length_vertical
    }
}

void lbsc_hevc_parse_ptl(lbsc_hevc_parser_ctx* phpc, bitstream_ctx* pbsc, unsigned int max_sub_layers_minus1)
{
    unsigned int i;
    HVCCProfileTierLevel general_ptl;
    uint8_t sub_layer_profile_present_flag[HEVC_MAX_SUB_LAYERS];
    uint8_t sub_layer_level_present_flag[HEVC_MAX_SUB_LAYERS];
    assert(phpc);
    assert(phpc->m_pxc);
    assert(phpc->m_pxc->m_pbsc);
    bitstream_ctx* pbs = phpc->m_pxc->m_pbsc;
    if(pbsc)
    {
        pbs = pbsc;
    }
    general_ptl.profile_space = (uint8_t)read_bits(pbs, 2);
    general_ptl.tier_flag = (uint8_t)read_bits(pbs, 1);
    general_ptl.profile_idc = (uint8_t)read_bits(pbs, 5);
    general_ptl.profile_compatibility_flags = (uint32_t)read_bits(pbs, 32);
    general_ptl.constraint_indicator_flags = read_bits(pbs, 48);
    general_ptl.level_idc = (uint8_t)read_bits(pbs, 8);
    //hvcc_update_ptl(hvcc, &general_ptl);
    phpc->m_phvcc->general_profile_space = general_ptl.profile_space;
    if (phpc->m_phvcc->general_tier_flag < general_ptl.tier_flag)
    {
        phpc->m_phvcc->general_level_idc = general_ptl.level_idc;
    }
    else
    {
        phpc->m_phvcc->general_level_idc = LBMAX(phpc->m_phvcc->general_level_idc, general_ptl.level_idc);
    }
    phpc->m_phvcc->general_tier_flag = LBMAX(phpc->m_phvcc->general_tier_flag, general_ptl.tier_flag);
    phpc->m_phvcc->general_profile_idc = LBMAX(phpc->m_phvcc->general_profile_idc, general_ptl.profile_idc);

    /*
    * Each bit in general_profile_compatibility_flags may only be set if all
    * the parameter sets set that bit.
    */
    phpc->m_phvcc->general_profile_compatibility_flags &= general_ptl.profile_compatibility_flags;

    /*
    * Each bit in general_constraint_indicator_flags may only be set if all
    * the parameter sets set that bit.
    */
    phpc->m_phvcc->general_constraint_indicator_flags &= general_ptl.constraint_indicator_flags;

    for (i = 0; i < max_sub_layers_minus1; i++) {
        sub_layer_profile_present_flag[i] = (uint8_t)read_bits(pbs, 1);
        sub_layer_level_present_flag[i] = (uint8_t)read_bits(pbs, 1);
    }

    if (max_sub_layers_minus1 > 0)
        for (i = max_sub_layers_minus1; i < 8; i++)
            read_bits(pbs, 2); // reserved_zero_2bits[i]

    for (i = 0; i < max_sub_layers_minus1; i++) {
        if (sub_layer_profile_present_flag[i]) {
            /*
            * sub_layer_profile_space[i]                     u(2)
            * sub_layer_tier_flag[i]                         u(1)
            * sub_layer_profile_idc[i]                       u(5)
            * sub_layer_profile_compatibility_flag[i][0..31] u(32)
            * sub_layer_progressive_source_flag[i]           u(1)
            * sub_layer_interlaced_source_flag[i]            u(1)
            * sub_layer_non_packed_constraint_flag[i]        u(1)
            * sub_layer_frame_only_constraint_flag[i]        u(1)
            * sub_layer_reserved_zero_44bits[i]              u(44)
            */
            read_bits(pbs, 32);
            read_bits(pbs, 32);
            read_bits(pbs, 24);
        }

        if (sub_layer_level_present_flag[i])
            read_bits(pbs, 8);
    }
}

int lbsc_hevc_skip_hrd_parameters(lbsc_hevc_parser_ctx* phpc, bitstream_ctx* pbsc, uint8_t cprms_present_flag, unsigned int max_sub_layers_minus1)
{
    unsigned int i = 0;
    uint8_t sub_pic_hrd_params_present_flag = 0;
    uint8_t nal_hrd_parameters_present_flag = 0;
    uint8_t vcl_hrd_parameters_present_flag = 0;
    assert(phpc);
    assert(phpc->m_pxc);
    assert(phpc->m_pxc->m_pbsc);
    bitstream_ctx* pbs = phpc->m_pxc->m_pbsc;
    if(pbsc)
    {
        pbs = pbsc;
    }
    if (cprms_present_flag) {
        nal_hrd_parameters_present_flag = (uint8_t)read_bits(pbs, 1);
        vcl_hrd_parameters_present_flag = (uint8_t)read_bits(pbs, 1);

        if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
            sub_pic_hrd_params_present_flag = (uint8_t)read_bits(pbs, 1);

            if (sub_pic_hrd_params_present_flag)
                /*
                * tick_divisor_minus2                          u(8)
                * du_cpb_removal_delay_increment_length_minus1 u(5)
                * sub_pic_cpb_params_in_pic_timing_sei_flag    u(1)
                * dpb_output_delay_du_length_minus1            u(5)
                */
                read_bits(pbs, 19);

            /*
            * bit_rate_scale u(4)
            * cpb_size_scale u(4)
            */
            read_bits(pbs, 8);

            if (sub_pic_hrd_params_present_flag)
                read_bits(pbs, 4); // cpb_size_du_scale

                                  /*
                                  * initial_cpb_removal_delay_length_minus1 u(5)
                                  * au_cpb_removal_delay_length_minus1      u(5)
                                  * dpb_output_delay_length_minus1          u(5)
                                  */
            read_bits(pbs, 15);
        }
    }

    for (i = 0; i <= max_sub_layers_minus1; i++) {
        unsigned int cpb_cnt_minus1 = 0;
        uint8_t low_delay_hrd_flag = 0;
        uint8_t fixed_pic_rate_within_cvs_flag = 0;
        uint8_t fixed_pic_rate_general_flag = (uint8_t)read_bits(pbs, 1);

        if (!fixed_pic_rate_general_flag)
            fixed_pic_rate_within_cvs_flag = (uint8_t)read_bits(pbs, 1);

        if (fixed_pic_rate_within_cvs_flag)
            read_ue(pbs); // elemental_duration_in_tc_minus1
        else
            low_delay_hrd_flag = (uint8_t)read_bits(pbs, 1);

        if (!low_delay_hrd_flag) {
            cpb_cnt_minus1 = (uint32_t)read_ue(pbs);
            if (cpb_cnt_minus1 > 31)
            {
                lberror("Invalid hevc data, parser hrd parameter failed, cpb_cnt_minus1:%d\n", cpb_cnt_minus1);
                return -1;
            }
        }

        if (nal_hrd_parameters_present_flag)
            lbsc_hevc_skip_sub_layer_hrd_parameters(phpc, pbs, cpb_cnt_minus1, sub_pic_hrd_params_present_flag);

        if (vcl_hrd_parameters_present_flag)
            lbsc_hevc_skip_sub_layer_hrd_parameters(phpc, pbs, cpb_cnt_minus1, sub_pic_hrd_params_present_flag);
    }

    return 0;
}

int lbsc_get_mux_hvcc_size(lbsc_hevc_parser_ctx* phpc)
{
    int size = 23 + 3 * 3;
    assert(phpc);
    size_t i = 0;
    for (i = 0; i < 1; i++)
    {
        size += phpc->m_pxc->m_nvps_len + 2;
    }

    for (i = 0; i < 1; i++)
    {
        size += phpc->m_pxc->m_nsps_len + 2;
    }

    for (i = 0; i < 1; i++)
    {
        size += phpc->m_pxc->m_nsps_len + 2;
    }

    return size;
}

void lbsc_hevc_skip_sub_layer_hrd_parameters(lbsc_hevc_parser_ctx* phpc, bitstream_ctx* pbsc, unsigned int cpb_cnt_minus1, uint8_t sub_pic_hrd_params_present_flag)
{
    unsigned int i;
    assert(phpc);
    bitstream_ctx* pbs = phpc->m_pxc->m_pbsc;
    if(pbsc)
    {
        pbs = pbsc;
    }
    for (i = 0; i <= cpb_cnt_minus1; i++) {
        read_ue(pbs); // bit_rate_value_minus1
        read_ue(pbs); // cpb_size_value_minus1

        if (sub_pic_hrd_params_present_flag) {
            read_ue(pbs); // cpb_size_du_value_minus1
            read_ue(pbs); // bit_rate_du_value_minus1
        }

        read_bits(pbs, 1); // cbr_flag
    }
}

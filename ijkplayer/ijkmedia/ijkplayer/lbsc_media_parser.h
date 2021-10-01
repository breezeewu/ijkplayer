#pragma once
#include "bitstream.h"
#include "lbsc_media_parser.h"

#define AVC_NAL_TYPE_MASK    0x1f
#define HEVC_NAL_TYPE_MASK    0x7e

#define AVC_PROFILE_BASELINE        66
#define AVC_PROFILE_MAIN            77
#define AVC_PROFILE_EXTENDED        88
#define AVC_PROFILE_HIGHT_FREXT        100
#define AVC_PROFILE_HIGHT10_FREXT   110
#define AVC_PROFILE_HIGHT_422_FREXT 122
#define AVC_PROFILE_HIGHT_444_FREXT 144

#define AVC_STREAM_TYPE                 4
#define HEVC_STREAM_TYPE                5
#define ue_size uint8_t
#define se_size uint8_t

#define LBMAX(a,b) ((a) > (b) ? (a) : (b))
#define LBMIN(a,b) ((a) > (b) ? (b) : (a))

#ifndef lbtrace
#define lbtrace
#endif
#ifndef lbmemory
#define lbmemory(ptr, len)
#endif
#ifndef lberror
#define lberror
#endif

#ifndef CHECK_RESULT
#define CHECK_RESULT(ret) if(0 > ret) {lberror("%s:%d, %s check result failed, ret:%d\n", __FILE__, __LINE__, __FUNCTION__, ret); return ret;}
#endif
#ifndef CHECK_POINTER
#define CHECK_POINTER(ptr, ret) if(NULL == ptr) {lberror("%s:%d, %s NULL ptr error, ret:%d\n", __FILE__, __LINE__, __FUNCTION__, ret); return ret;}
#endif
typedef int (*on_parse_nalu_func)(void* powner, int nalu_type, const char* pdata, int len);
//typedef int (*demux_sps)()
//typedef int on_parse_nalu(lbsc_xvc_ctx* pxc, int nalu_type, const char* pdata, int len);
enum EAVC_NAL_TYPE
{
    EAVC_NAL_UNKNOWN            = 0, // not use
    EAVC_NAL_SLICE                = 1, // not idr slice
    EAVC_NAL_DPA                = 2, // slice data partition a layer
    EAVC_NAL_DPB                = 3, // slice data partition b layer
    EAVC_NAL_DPC                = 4, // slice data partition c layer
    EAVC_NAL_IDR                = 5, // instantaneous decoding refresh slice
    EAVC_NAL_SEI                = 6, // supplemental enhancement information
    EAVC_NAL_SPS                = 7, // sequence parameter set
    EAVC_NAL_PPS                = 8, // picture parameter set
    EAVC_NAL_AUD                = 9, // access unit delimiter
    EAVC_NAL_END_SEQ            = 10,// end of sequence
    EAVC_NAL_END_STREAM            = 11,// end of stream
    EAVC_NAL_FILLER_DATA        = 12,// fill data
    // 13-23 reserved
    // 24-31 not use
};

enum EHEVC_NAL_TYPE {
    HEVC_NAL_TRAIL_N = 0,
    HEVC_NAL_TRAIL_R = 1,
    HEVC_NAL_TSA_N = 2,
    HEVC_NAL_TSA_R = 3,
    HEVC_NAL_STSA_N = 4,
    HEVC_NAL_STSA_R = 5,
    HEVC_NAL_RADL_N = 6,
    HEVC_NAL_RADL_R = 7,
    HEVC_NAL_RASL_N = 8,
    HEVC_NAL_RASL_R = 9,
    HEVC_NAL_VCL_N10 = 10,
    HEVC_NAL_VCL_R11 = 11,
    HEVC_NAL_VCL_N12 = 12,
    HEVC_NAL_VCL_R13 = 13,
    HEVC_NAL_VCL_N14 = 14,
    HEVC_NAL_VCL_R15 = 15,
    // hevc keyframe
    HEVC_NAL_BLA_W_LP = 16,
    HEVC_NAL_BLA_W_RADL = 17,
    HEVC_NAL_BLA_N_LP = 18,
    HEVC_NAL_IDR_W_RADL = 19,
    HEVC_NAL_IDR_N_LP = 20,
    HEVC_NAL_CRA_NUT = 21,

    HEVC_NAL_IRAP_VCL22 = 22,
    HEVC_NAL_IRAP_VCL23 = 23,
    HEVC_NAL_RSV_VCL24 = 24,
    HEVC_NAL_RSV_VCL25 = 25,
    HEVC_NAL_RSV_VCL26 = 26,
    HEVC_NAL_RSV_VCL27 = 27,
    HEVC_NAL_RSV_VCL28 = 28,
    HEVC_NAL_RSV_VCL29 = 29,
    HEVC_NAL_RSV_VCL30 = 30,
    HEVC_NAL_RSV_VCL31 = 31,
    HEVC_NAL_VPS = 32,
    HEVC_NAL_SPS = 33,
    HEVC_NAL_PPS = 34,
    HEVC_NAL_AUD = 35,
    HEVC_NAL_EOS_NUT = 36,
    HEVC_NAL_EOB_NUT = 37,
    HEVC_NAL_FD_NUT = 38,
    HEVC_NAL_SEI_PREFIX = 39,
    HEVC_NAL_SEI_SUFFIX = 40,
    HEVC_NAL_RSV_NVCL41 = 41,
    HEVC_NAL_RSV_NVCL42 = 42,
    HEVC_NAL_RSV_NVCL43 = 43,
    HEVC_NAL_RSV_NVCL44 = 44,
    HEVC_NAL_RSV_NVCL45 = 45,
    HEVC_NAL_RSV_NVCL46 = 46,
    HEVC_NAL_RSV_NVCL47 = 47,
    HEVC_NAL_UNSPEC48 = 48,
    HEVC_NAL_UNSPEC49 = 49,
    HEVC_NAL_UNSPEC50 = 50,
    HEVC_NAL_UNSPEC51 = 51,
    HEVC_NAL_UNSPEC52 = 52,
    HEVC_NAL_UNSPEC53 = 53,
    HEVC_NAL_UNSPEC54 = 54,
    HEVC_NAL_UNSPEC55 = 55,
    HEVC_NAL_UNSPEC56 = 56,
    HEVC_NAL_UNSPEC57 = 57,
    HEVC_NAL_UNSPEC58 = 58,
    HEVC_NAL_UNSPEC59 = 59,
    HEVC_NAL_UNSPEC60 = 60,
    HEVC_NAL_UNSPEC61 = 61,
    HEVC_NAL_UNSPEC62 = 62,
    HEVC_NAL_UNSPEC63 = 63,
};
typedef struct lbsc_xvc_context
{
    int                m_nstream_type;
    uint8_t            m_unal_type_mask;
    uint8_t            m_unal_type_vps;
    uint8_t            m_unal_type_sps;
    uint8_t            m_unal_type_pps;

    uint8_t            m_uxvcc_fixed_header_size;
    uint8_t            m_uxvcc_fixed_metadata_prefix_size;
    uint8_t            m_uxvcc_fixed_nal_prefix_size;
    uint8_t            m_reserved;

    int                m_nwidth;
    int                m_nheight;
    
    bitstream_ctx*      m_pbsc;
    //bitstream_ctx*      m_pcur_bsc;
    // sequence header
    char*               m_pvsh;
    int                 m_nvsh_len;

    char*               m_pvps;
    int                 m_nvps_len;
    char*               m_psps;
    int                 m_nsps_len;
    char*               m_ppps;
    int                 m_npps_len;
    float               m_fframe_rate;
    int                 m_nidr_frame;
    void*               m_powner;

    on_parse_nalu_func  m_pon_parse_nal;
} lbsc_xvc_ctx;

lbsc_xvc_ctx* lbsc_open_xvc_context(int stream_type);

void lbsc_close_xvc_context(lbsc_xvc_ctx** ppxc);

void lbsc_reset_xvc_context(lbsc_xvc_ctx* pxc);

int is_sequence_header(lbsc_xvc_ctx* pxc, int nal_type);

int lbsc_get_sequence_header(lbsc_xvc_ctx* pxc, char* psh, int len);
//int is_sequence_header(lbsc_xvc_ctx* pxc);

int is_idr_frame(int stream_type, const char* pdata, int len);

int is_frame_nalu(int stream_type, int nal_type, int idr);
//int is_frame_nalu(int streamtype, int naltype, int idr);

int is_start_code(const char* pdata, int len, int* pstart_code_len);

int get_start_code_size(const char* pdata, int len);

int parse_stream(lbsc_xvc_ctx* pxc, const char* pdata, int len);

int on_parse_nalu(void* powner, int nalu_type, const char* pdata, int len);

int read_nal_type(bitstream_ctx* pbs, int stream_type);

const char* read_annexb_nal(const char* pdata, int len, int* pnal_len, int has_start_code);

int read_nalu_type_from_data(int stream_type, const char* pdata, int len);

const char* find_annexb_start_code(const char* pdata, int len, int* pstart_code_size);
//const char* find_start_code(const char* pdata, int len, int* pstart_code_size);
//const char* find_start_code(lbsc_xvc_ctx* pxc, const char* pdata, int len, int* pstart_code_size);

const char* get_nal(int stream_type, const char* pdata, int len, int* pnal_len, int* pnal_type, int has_start_code);

const char* find_nal(lbsc_xvc_ctx* pxc, const char* pdata, int len, int nal_type, int* pnal_len, int has_start_code);


//int rbsp_from_nalu(const char* pdata, int len, char* pout, int out_len);
char* rbsp_from_nalu(const char* pdata, int len, int* pout_len);
//char* rbsp_from_nalu(const char* pdata, int len);

const char* skip_start_code(const char* pdata, int len, int* pnal_len);
const char* skip_aud_nal(int stream_type, const char* pdata, int* plen);

const char* get_nalu_frame(int streamtype, const char* pdata, int len, int* pframe_size, int bparser_to_end, int bhas_start_code);

int add_metadata(lbsc_xvc_ctx* pxc, const char* pdata, int len);

int get_extradata_size(lbsc_xvc_ctx* pxc);

int mux_extradata(lbsc_xvc_ctx* pxc, char* pextradata, int size);

int demux_extradata(lbsc_xvc_ctx* pxc, const char* pextradata, int size);

// avc sps context
typedef struct lbsc_avc_sps_context
{
    uint8_t    id;
    uint8_t    profile_idc;    // 8 bit, 66 baseline,77:main,88:extend,100:high(FRExt),110 high10(FRExt),122 high4:2:2(FRExt),144 high4:4:4(FRExt)
    uint8_t constraint_set_flag;    // 8 bit
    uint8_t level_idc;                // 8 bit
    ue_size seq_parameter_set_id;    // ue
    ue_size chroma_format_idc;        // ue
    uint8_t    separate_colour_plane_flag;    // 1 bit
    ue_size bit_depth_luma_minus8; // ue
    ue_size bit_depth_chroma_minus8; // ue
    uint8_t qpprime_y_zero_transform_bypass_flag;    // 1 bit
    uint8_t seq_scaling_matrix_present_flag;        //1 bit
    uint8_t* seq_scaling_list_present_flag;            // 1 bit,
    ue_size log2_max_frame_num_minus4;                // ue
    ue_size pic_order_cnt_type;                        // ue
    ue_size log2_max_pic_order_cnt_lsb_minus4;        // ue
    ue_size delta_pic_order_always_zero_flag;        // 1bit, if 1 == pic_order_cnt_type
    se_size offset_for_non_ref_pic;                    // se
    se_size offset_for_top_to_bottom_field;            // se
    ue_size num_ref_frames_in_pic_order_cnt_cycle;  // ue
    se_size* offset_for_ref_frame;                    // se, list
    ue_size max_num_ref_frames;                        // ue
    uint8_t gaps_in_frame_num_value_allowed_flag;    // 1 bit
    uint16_t m_nsar_width;
    uint16_t m_nsar_height;
} lbsc_avc_sps_ctx;

lbsc_avc_sps_ctx* lbsc_open_avc_sps_context();

void lbsc_close_avc_sps_context(lbsc_avc_sps_ctx** ppasc);

// avc parser context
typedef struct lbsc_avc_parser_context
{
    lbsc_xvc_ctx*       m_pxc;
    lbsc_avc_sps_ctx*   m_pasc;
} lbsc_avc_parser_ctx;

lbsc_avc_parser_ctx* lbsc_avc_open_parser(lbsc_xvc_ctx* pxc);

void lbsc_avc_close_parser(lbsc_avc_parser_ctx** ppapc);

int lbsc_avc_on_parse_nalu(void* powner, int nal_type, const char* pdata, int len);

int lbsc_avc_get_extradata(lbsc_avc_parser_ctx* papc);

int lbsc_avc_mux_extradata(lbsc_avc_parser_ctx* papc, char* pextradata, int len);

int lbsc_avc_demux_extradata(lbsc_avc_parser_ctx* papc, const char* pextradata, int len);

int lbsc_avc_demux_sps(lbsc_avc_parser_ctx* papc, const char* pdata, int len);

//int lbsc_avc_demux_pps(const char* pdata, int len);
int lbsc_avc_demux_pps(lbsc_avc_parser_ctx* papc, const char* pdata, int len);

int lbsc_avc_get_extradata_size(lbsc_avc_parser_ctx* papc);

#define MAX_SPATIAL_SEGMENTATION 4096 // max. value of u(12) field

enum HEVCSliceType {
    HEVC_SLICE_B = 0,
    HEVC_SLICE_P = 1,
    HEVC_SLICE_I = 2,
};

enum {
    // 7.4.3.1: vps_max_layers_minus1 is in [0, 62].
    HEVC_MAX_LAYERS = 63,
    // 7.4.3.1: vps_max_sub_layers_minus1 is in [0, 6].
    HEVC_MAX_SUB_LAYERS = 7,
    // 7.4.3.1: vps_num_layer_sets_minus1 is in [0, 1023].
    HEVC_MAX_LAYER_SETS = 1024,

    // 7.4.2.1: vps_video_parameter_set_id is u(4).
    HEVC_MAX_VPS_COUNT = 16,
    // 7.4.3.2.1: sps_seq_parameter_set_id is in [0, 15].
    HEVC_MAX_SPS_COUNT = 16,
    // 7.4.3.3.1: pps_pic_parameter_set_id is in [0, 63].
    HEVC_MAX_PPS_COUNT = 64,

    // A.4.2: MaxDpbSize is bounded above by 16.
    HEVC_MAX_DPB_SIZE = 16,
    // 7.4.3.1: vps_max_dec_pic_buffering_minus1[i] is in [0, MaxDpbSize - 1].
    HEVC_MAX_REFS = HEVC_MAX_DPB_SIZE,

    // 7.4.3.2.1: num_short_term_ref_pic_sets is in [0, 64].
    HEVC_MAX_SHORT_TERM_REF_PIC_SETS = 64,
    // 7.4.3.2.1: num_long_term_ref_pics_sps is in [0, 32].
    HEVC_MAX_LONG_TERM_REF_PICS = 32,

    // A.3: all profiles require that CtbLog2SizeY is in [4, 6].
    HEVC_MIN_LOG2_CTB_SIZE = 4,
    HEVC_MAX_LOG2_CTB_SIZE = 6,

    // E.3.2: cpb_cnt_minus1[i] is in [0, 31].
    HEVC_MAX_CPB_CNT = 32,

    // A.4.1: in table A.6 the highest level allows a MaxLumaPs of 35 651 584.
    HEVC_MAX_LUMA_PS = 35651584,
    // A.4.1: pic_width_in_luma_samples and pic_height_in_luma_samples are
    // constrained to be not greater than sqrt(MaxLumaPs * 8).  Hence height/
    // width are bounded above by sqrt(8 * 35651584) = 16888.2 samples.
    HEVC_MAX_WIDTH = 16888,
    HEVC_MAX_HEIGHT = 16888,

    // A.4.1: table A.6 allows at most 22 tile rows for any level.
    HEVC_MAX_TILE_ROWS = 22,
    // A.4.1: table A.6 allows at most 20 tile columns for any level.
    HEVC_MAX_TILE_COLUMNS = 20,

    // A.4.2: table A.6 allows at most 600 slice segments for any level.
    HEVC_MAX_SLICE_SEGMENTS = 600,

    // 7.4.7.1: in the worst case (tiles_enabled_flag and
    // entropy_coding_sync_enabled_flag are both set), entry points can be
    // placed at the beginning of every Ctb row in every tile, giving an
    // upper bound of (num_tile_columns_minus1 + 1) * PicHeightInCtbsY - 1.
    // Only a stream with very high resolution and perverse parameters could
    // get near that, though, so set a lower limit here with the maximum
    // possible value for 4K video (at most 135 16x16 Ctb rows).
    HEVC_MAX_ENTRY_POINT_OFFSETS = HEVC_MAX_TILE_COLUMNS * 135,
};

typedef struct hevc_nal_header
{
    uint8_t        forbiden;
    uint8_t        nal_type;
    uint8_t        layer_id;
    uint8_t        tid;
} hevc_nal_hdr;
typedef struct HVCCNALUnitArray {
    uint8_t  array_completeness;
    uint8_t  NAL_unit_type;
    uint16_t numNalus;
    uint16_t *nalUnitLength;
    uint8_t  **nalUnit;
} HVCCNALUnitArray;

typedef struct HEVCDecoderConfigurationRecord {
    uint8_t  configurationVersion;
    uint8_t  general_profile_space;
    uint8_t  general_tier_flag;
    uint8_t  general_profile_idc;
    uint32_t general_profile_compatibility_flags;
    uint64_t general_constraint_indicator_flags;
    uint8_t  general_level_idc;
    uint16_t min_spatial_segmentation_idc;
    uint8_t  parallelismType;
    uint8_t  chromaFormat;
    uint8_t  bitDepthLumaMinus8;
    uint8_t  bitDepthChromaMinus8;
    uint16_t avgFrameRate;
    uint8_t  constantFrameRate;
    uint8_t  numTemporalLayers;
    uint8_t  temporalIdNested;
    uint8_t  lengthSizeMinusOne;
    uint8_t  numOfArrays;
    HVCCNALUnitArray *array;
} HEVCDecoderConfigurationRecord;

typedef struct HVCCProfileTierLevel {
    uint8_t  profile_space;
    uint8_t  tier_flag;
    uint8_t  profile_idc;
    uint32_t profile_compatibility_flags;
    uint64_t constraint_indicator_flags;
    uint8_t  level_idc;
} HVCCProfileTierLevel;

/**
Profile, tier and level
@see 7.3.3 Profile, tier and level syntax
*/
typedef struct
{
    uint8_t general_profile_space;
    uint8_t general_tier_flag;
    uint8_t general_profile_idc;
    uint8_t general_profile_compatibility_flag[32];
    uint8_t general_progressive_source_flag;
    uint8_t general_interlaced_source_flag;
    uint8_t general_non_packed_constraint_flag;
    uint8_t general_frame_only_constraint_flag;
    uint8_t general_max_12bit_constraint_flag;
    uint8_t general_max_10bit_constraint_flag;
    uint8_t general_max_8bit_constraint_flag;
    uint8_t general_max_422chroma_constraint_flag;
    uint8_t general_max_420chroma_constraint_flag;
    uint8_t general_max_monochrome_constraint_flag;
    uint8_t general_intra_constraint_flag;
    uint8_t general_one_picture_only_constraint_flag;
    uint8_t general_lower_bit_rate_constraint_flag;
    uint64_t general_reserved_zero_34bits; // todo
    uint64_t general_reserved_zero_43bits; // todo
    uint8_t general_inbld_flag;
    uint8_t general_reserved_zero_bit;
    uint8_t general_level_idc;
    /*vector<uint8_t> sub_layer_profile_present_flag;
    vector<uint8_t> sub_layer_level_present_flag;
    uint8_t reserved_zero_2bits[8];
    vector<uint8_t> sub_layer_profile_space;
    vector<uint8_t> sub_layer_tier_flag;
    vector<uint8_t> sub_layer_profile_idc;
    //vector<vector<uint8_t>> sub_layer_profile_compatibility_flag;
    vector<uint8_t> sub_layer_progressive_source_flag;
    vector<uint8_t> sub_layer_interlaced_source_flag;
    vector<uint8_t> sub_layer_non_packed_constraint_flag;
    vector<uint8_t> sub_layer_frame_only_constraint_flag;
    vector<uint8_t> sub_layer_max_12bit_constraint_flag;
    vector<uint8_t> sub_layer_max_10bit_constraint_flag;
    vector<uint8_t> sub_layer_max_8bit_constraint_flag;
    vector<uint8_t> sub_layer_max_422chroma_constraint_flag;
    vector<uint8_t> sub_layer_max_420chroma_constraint_flag;
    vector<uint8_t> sub_layer_max_monochrome_constraint_flag;
    vector<uint8_t> sub_layer_intra_constraint_flag;
    vector<uint8_t> sub_layer_one_picture_only_constraint_flag;
    vector<uint8_t> sub_layer_lower_bit_rate_constraint_flag;
    vector<uint64_t> sub_layer_reserved_zero_34bits;
    vector<uint64_t> sub_layer_reserved_zero_43bits;
    vector<uint8_t> sub_layer_inbld_flag;
    vector<uint8_t> sub_layer_reserved_zero_bit;
    vector<uint8_t> sub_layer_level_idc;*/

} profile_tier_level_t;

typedef struct lbsc_hevc_parser_context
{
    lbsc_xvc_ctx*       m_pxc;
    HEVCDecoderConfigurationRecord*        m_phvcc;

    int                m_nsar_width;
    int                m_nsar_height;
    float              m_fframe_rate;

    int                m_nwidth;
    int                m_nheight;
} lbsc_hevc_parser_ctx;

lbsc_hevc_parser_ctx* lbsc_hevc_open_parser(lbsc_xvc_ctx* pxc);

void lbsc_hevc_close_parser(lbsc_hevc_parser_ctx** pphpc);

void lbsc_hevc_reset_parser(lbsc_hevc_parser_ctx* phpc);

int lbsc_hevc_on_parser_nalu(lbsc_hevc_parser_ctx* phpc, int nal_type, const char* pdata, int len);
//int lbsc_hevc_on_parser_nalu(int nal_type, const char* pdata, int len);

int lbsc_hevc_demux_vps(lbsc_hevc_parser_ctx* phpc, const char* vps, int vps_len);

int lbsc_hevc_demux_sps(lbsc_hevc_parser_ctx* phpc, const char* sps, int sps_len);

int lbsc_hevc_demux_pps(lbsc_hevc_parser_ctx* phpc, const char* pps, int pps_len);

int lbsc_hevc_mux_hvcc(lbsc_hevc_parser_ctx* phpc, char* pdata, int len);

int lbsc_hevc_demux_hvcc(lbsc_hevc_parser_ctx* phpc, char* pdata, int len);

int lbsc_hevc_write_xps(lbsc_hevc_parser_ctx* phpc, bitstream_ctx* pbsc);

int lbsc_hevc_read_xps(lbsc_hevc_parser_ctx* phpc, bitstream_ctx* pbsc);

int lbsc_hevc_parse_rps(lbsc_hevc_parser_ctx* phpc, bitstream_ctx* pbsc, unsigned int rps_idx, unsigned int num_rps, unsigned int* num_delta_pocs);

void lbsc_hevc_parse_vui(lbsc_hevc_parser_ctx* phpc, bitstream_ctx* pbsc, unsigned int max_sub_layers_minus1);

void lbsc_hevc_parse_ptl(lbsc_hevc_parser_ctx* phpc, bitstream_ctx* pbsc, unsigned int max_sub_layers_minus1);

int lbsc_hevc_skip_hrd_parameters(lbsc_hevc_parser_ctx* phpc, bitstream_ctx* pbsc, uint8_t cprms_present_flag, unsigned int max_sub_layers_minus1);

int lbsc_get_mux_hvcc_size(lbsc_hevc_parser_ctx* phpc);

void lbsc_hevc_skip_sub_layer_hrd_parameters(lbsc_hevc_parser_ctx* phpc, bitstream_ctx* pbsc, unsigned int cpb_cnt_minus1, uint8_t sub_pic_hrd_params_present_flag);

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "lazylog.h"
#include "audio_proc.h"
#include "list.h"
#include "libavutil/time.h"
#include "avresample.h"
#include "audio_processing/modules/audio_processing/aec/include/echo_cancellation.h"
#include "audio_processing/modules/audio_processing/agc/include/gain_control.h"
#include "audio_processing/modules/audio_processing/ns/include/noise_suppression.h"
#include "audio_processing/common_audio/signal_processing/include/signal_processing_library.h"

#define MAX_DELAY_TIMESTAMP 50000
typedef struct noise_reduce_context
{
    void* pns_handle;
    int nr_mode;
    int samplerate;
    int nframe_bytes;
    int flt_state_1[6];
    int flt_state_2[6];
    int syn_state_1[6];
    int syn_state_2[6];
    short sh_in_low[160];
    short sh_in_high[160];
    short sh_out_low[160];
    short sh_out_high[160];
    FILE* pnr_output_file;
} nr_ctx;

typedef struct agc_context
{
    void*   pagc_handle;
    int     agc_mode;
    int     samplerate;
    int     min_level;
    int     max_level;
    WebRtcAgc_config_t  config;

    short*  pcopy_buf;
    int     ncopy_buf_size;
    int     frame_size;

    int     mic_level_in;
    int     mic_level_out;
    FILE*   pagc_output_file;
} agc_ctx;
#define WRITE_FILE_TEST
#define WRITE_INPUT_DATA
#define WRITE_OUTPUT_FILE
typedef struct audio_proc_ctx
{
    void* paec_ctx;
    struct noise_reduce_context* pnr_ctx;
    void* pagc_ctx;
    list_ctx*  plist_ctx;
    struct sample_buffer*   psb;
    struct sample_buffer*   paec_pcm_buf;
    struct avresample_context*     prc;
    pthread_mutex_t*    pmutex;

    int nbsamples;
    int nbsample_bytes;
    int msdelay;
    int ms_aec_delay;
    int us_timestamp_delay;
    int ns_mode;
    
    int channel;
    int samplerate;
    int samplebytes;
    int sampleformat;
    
    int far_channel;
    int far_samplerate;
    int far_sample_bytes;
    
    int discontinue;
    int64_t lstart_timestamp;
    int64_t lcur_far_timestamp;
    int64_t lcur_near_timestamp;
    int64_t llast_fix_timestamp;
    int64_t lstart_drop_timestamp;
    int     nlast_median;
    AVFrame* ptmp_frame;
    int64_t laec_pcm_pts;
    char* pnbsample_buffer;
    char*   pcopy_buf_1;
    char*   pcopy_buf_2;
    int     ncopy_buf_size;
    int nbuf_offset;
#ifdef WRITE_FILE_TEST
    FILE*   pfar_file;
    FILE*   pnear_file;
    FILE*   pinput_near_file;
#endif
#ifdef WRITE_INPUT_DATA
    FILE*   pinput_far_file;
    FILE*   pnear_buf_file;
#endif
#ifdef WRITE_OUTPUT_FILE
    FILE*   pout_file;
#endif
} audio_process_context;

typedef struct
{
    int channel;
    int samplerate;
    int sample_bytes;
    int nbsamples;
    char* pdata;
    int data_len;
    int64_t timestamp;
} pcm_frame;

struct audio_proc_ctx* lbaudio_proc_open_context(int channel, int samplerate, int samformat, int nbsamples)
{
    lbtrace("lbaudio_proc_open_context(channel:%d, samplerate:%d, samformat:%d, nbsamples:%d)\n", channel, samplerate, samformat, nbsamples);
    struct audio_proc_ctx* papc = (struct audio_proc_ctx*)malloc(sizeof(struct audio_proc_ctx));
    memset(papc, 0, sizeof(struct audio_proc_ctx));
    papc->channel = channel;
    papc->samplerate = samplerate;
    papc->sampleformat = samformat;
    papc->nbsamples = nbsamples;
    papc->discontinue = 1;
    papc->nbsample_bytes = channel*nbsamples * av_get_bytes_per_sample(samformat);
    //papc->nbsample_max_buf_size = papc->nbsample_bytes > BAND_FRAME_LENGTH ? papc->nbsample_bytes : BAND_FRAME_LENGTH;
    papc->pnbsample_buffer = (char*)malloc(papc->nbsample_bytes);
    papc->nbuf_offset = 0;
    papc->lstart_timestamp = INT64_MIN;//lbget_system_timestamp_in_us();
    papc->lcur_far_timestamp = INT64_MIN;
    papc->lcur_near_timestamp = INT64_MIN;
    papc->llast_fix_timestamp = 0;
    papc->nlast_median = -1;
    papc->plist_ctx = list_context_create(1000);
    papc->pmutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutexattr_t attr;
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(papc->pmutex, &attr);
    lbtrace("end papc:%p\n", papc);
    return papc;
}

int lbfar_audio_callback(void* powner, char* pdata, int len, int64_t timestamp, int channel, int samplerate, int samformat)
{
    int ret = 0;
    if(NULL == powner || NULL == pdata || len <= 0)
    {
        lberror("Invalid parameter:powner:%p, pdata:%p, len:%d", powner, pdata, len);
        return -1;
    }
    struct audio_proc_ctx* papc = (struct audio_proc_ctx*)powner;
    
    timestamp = lbaudio_timestamp_fix(papc, len, timestamp, &papc->lcur_far_timestamp, channel, samplerate, samformat);
    lbdebug("timestamp:%" PRId64 ", papc->lcur_far_timestamp:%" PRId64", len:%d\n", timestamp, papc->lcur_far_timestamp, len);
    /*if(timestamp < 0)
    {
        timestamp = lbget_system_timestamp_in_us();
    }
    
    timestamp -= papc->lstart_timestamp;
    int64_t dur_ts = 0;//timestamp - papc->llast_timestamp;
    int64_t pcm_dur = len * 1000 / (papc->samplerate * av_get_bytes_per_sample(papc->sampleformat));
    if(INT64_MIN == papc->llast_timestamp)
    {
        dur_ts = timestamp - papc->llast_timestamp;
        papc->lcur_far_timestamp = timestamp;
    }
    else
    {
        papc->lcur_timestamp += pcm_dur;
    }
    if(abs(timestamp - papc->lcur_timestamp) > 100000)
    {
        lbtrace("abs(timestamp:%" PRId64 " - papc->lcur_timestamp:%" PRId64 "):%" PRId64 " > 100000\n", timestamp, papc->lcur_timestamp, abs(timestamp - papc->lcur_timestamp));;
    }
    lbtrace("powner:%p, pdata:%p,  len:%d, timestamp:%"PRId64", channel:%d, samplerate:%d, samformat:%d, dur:%" PRId64 ", pcm_dur:%" PRId64 "\n", powner, pdata,  len, timestamp, channel, samplerate, samformat, dur_ts, pcm_dur);*/

    //lbtrace("new add test, papc->pinput_far_file:%p\n", papc->pinput_far_file);
#ifdef WRITE_INPUT_DATA
    if(NULL == papc->pinput_far_file)
    {
        char* plogpath = get_log_path();
        char farpath[256];
        //lbtrace("plogpath:%s", plogpath);
        sprintf(farpath, "%s/input_far_%d.pcm", plogpath, papc->msdelay);
        
        papc->pinput_far_file = fopen(farpath, "wb");
        //lbtrace("papc->pinput_far_file:%p, farpath:%s, plogpath:%s", papc->pinput_far_file, farpath, plogpath);
    }

    if(papc->pinput_far_file)
    {
        //lbtrace("fwrite(pdata:%p, 1, len:%, papc->pinput_far_file:%p)\n", pdata, len, papc->pinput_far_file);
        fwrite(pdata, 1, len, papc->pinput_far_file);
        //lbtrace("after fwrite(pdata, 1, len, papc->pinput_far_file)\n");
    }
    //return 0;
#endif
    //lbtrace("before if, papc->ptmp_frame:%p", papc->ptmp_frame);
    if(papc->ptmp_frame && len == papc->ptmp_frame->linesize[0])
    {
        //lbtrace("enter if, papc->ptmp_frame->data[0]:%p, pdata:%p, len:%d\n", papc->ptmp_frame->data[0], pdata, len);
        memcpy(papc->ptmp_frame->data[0], pdata, len);
        papc->ptmp_frame->pts = timestamp;
    }
    else
    {
        //lbtrace("Invalid ptmp_frame, len:%d, papc->ptmp_frame->linesize[0]:%d, reset sample\n", papc->ptmp_frame ? papc->ptmp_frame->linesize[0] : 0);
        if(papc->ptmp_frame)
        {
            av_frame_free(&papc->ptmp_frame);
        }
        papc->ptmp_frame = lbaudio_make_frame(pdata, len, timestamp, channel, samplerate, samformat);
        //memcpy(papc->ptmp_frame->data[0], pdata, len);
        //return -1;
    }
    //lbtrace("before lbaudio_check_resample_enable\n");
    //AVFrame* pframe = lbaudio_make_frame(pdata, len, timestamp, channel, samplerate, samformat);
    if(lbaudio_check_resample_enable(papc, channel, samplerate, samformat))
    {
        ret = avresample_resample_frame(papc->prc, papc->ptmp_frame);
        //lbtrace("ret:%d = avresample_resample_frame(papc->prc, papc->ptmp_frame)\n", ret);
    }
    //lbtrace("after lbaudio_check_resample_enable\n");
    if(NULL == papc->psb)
    {
        papc->psb = lbsample_buffer_open(papc->channel, papc->samplerate, papc->sampleformat, papc->channel * papc->samplerate * av_get_bytes_per_sample(papc->sampleformat));
    }
    //lbtrace("after papc->psb = lbsample_buffer_open\n");
    ret = lbsample_buffer_deliver(papc->psb, (char*)papc->ptmp_frame->data[0], papc->ptmp_frame->linesize[0], timestamp);
    int flen = papc->nbsamples * channel * av_get_bytes_per_sample(samformat);
    int pts_off = 0;
    //lbtrace("before while\n");
    while(lbsample_buffer_size(papc->psb) >= flen)
    {
        //lbtrace("enter while\n");
        AVFrame* pframe = av_frame_alloc();
        pframe->channels = papc->channel;
        pframe->sample_rate = papc->samplerate;
        pframe->format = papc->sampleformat;
        pframe->nb_samples = papc->nbsamples;
        //lbtrace("before av_frame_get_buffer\n");
        av_frame_get_buffer(pframe, 4);
        //lbtrace("after av_frame_get_buffer\n");
        ret = lbsample_buffer_fetch(papc->psb, pframe->data[0], pframe->linesize[0], &pframe->pts);
        //lbtrace("ret:%d = lbsample_buffer_fetch\n", ret);
        push(papc->plist_ctx, pframe);
        pts_off += papc->nbsamples * 1000000 / papc->samplerate;
    };
    //lbdebug("lbfar_audio_callback end\n");
    return 0;
}

int lbadd_aec_filter(struct audio_proc_ctx* papc, int far_channel, int far_samplerate, int far_sample_bytes, int msdelay)
{
    if(NULL == papc)
    {
        lberror("Invalid parameter, papc:%p\n", papc);
        return -1;
    }
    
    papc->far_channel       = far_channel;
    papc->far_samplerate    = far_samplerate;
    papc->far_sample_bytes  = far_sample_bytes;
    papc->ms_aec_delay      = (msdelay*1000)%MAX_DELAY_TIMESTAMP;
    papc->us_timestamp_delay = msdelay*1000 - papc->ms_aec_delay;

    int ret = WebRtcAec_Create(&papc->paec_ctx);
    if(ret < 0 || NULL == papc->paec_ctx)
    {
        lberror("WebRtcAec_Create(&papc->paec_ctx) failed, ret:%d, papc->paec_ctx:%p\n", ret, papc->paec_ctx);
        return ret;
    }
    ret = WebRtcAec_Init(papc->paec_ctx, papc->samplerate, papc->samplerate);
    if(ret < 0)
    {
        lberror("ret:%d = WebRtcAec_Init(papc->paec_ctx:%p, papc->far_samplerate:%d, papc->samplerate:%d) failed\n", ret, papc->paec_ctx, papc->far_samplerate, papc->samplerate);
        return ret;
    }
    
    AecConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.nlpMode = kAecNlpConservative;
    ret = WebRtcAec_set_config(&papc->paec_ctx, cfg);
    if(ret < 0)
    {
        lberror("ret:%d = WebRtcAec_set_config(&papc->paec_ctx:%p, cfg)\n", ret, papc->paec_ctx);
        return ret;
    }
    
    return ret;
}

int laddd_aec_filterex(struct audio_proc_ctx* papc, int msdelay)
{
    if(NULL == papc)
    {
        lberror("Invalid parameter, papc:%p\n", papc);
        return -1;
    }
    lbdebug("laddd_aec_filterex(msdelay:%d)\n", msdelay);
    int delay = msdelay;
#ifdef LOAD_AEC_DELAY_CONFIG_FROM_FILE
    //delay = lbaudio_load_delay_from_config(get_log_path(), "config.ini");
    //lbtrace("load msdelay from config file, msdelay:%d, config delay:%d\n", msdelay, delay);
#endif
    if(INT32_MIN != delay)
    {
        papc->msdelay = delay;
    }
    else
    {
        papc->msdelay = msdelay;
    }
    papc->ms_aec_delay      = 40; //(papc->msdelay*1000);//%MAX_DELAY_TIMESTAMP;
    papc->us_timestamp_delay = papc->msdelay*1000;//papc->msdelay*1000 - papc->ms_aec_delay;
    int ret = WebRtcAec_Create(&papc->paec_ctx);
    if(ret < 0 || NULL == papc->paec_ctx)
    {
        lberror("WebRtcAec_Create(&papc->paec_ctx) failed, ret:%d, papc->paec_ctx:%p\n", ret, papc->paec_ctx);
        return ret;
    }
    ret = WebRtcAec_Init(papc->paec_ctx, papc->samplerate, papc->samplerate);
    if(ret < 0)
    {
        lberror("ret:%d = WebRtcAec_Init(papc->paec_ctx:%p, papc->samplerate:%d, papc->samplerate:%d) failed\n", ret, papc->paec_ctx, papc->samplerate, papc->samplerate);
        return ret;
    }
    
    AecConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.nlpMode = kAecNlpConservative;
    cfg.delay_logging = kAecTrue;
    cfg.metricsMode = kAecFalse;
    cfg.skewMode = kAecFalse;

    ret = WebRtcAec_set_config(papc->paec_ctx, cfg);
    if(ret < 0)
    {
        lberror("ret:%d = WebRtcAec_set_config(&papc->paec_ctx:%p, cfg)\n", ret, papc->paec_ctx);
        return ret;
    }
    WebRtcAec_enable_delay_correct(papc->paec_ctx, 1);
    lbtrace("WebRtcAec_enable_delay_correct(papc->paec_ctx, 1)\n");
    return ret;
}


int lbadd_noise_reduce_filter(struct audio_proc_ctx* papc, int nmode, const char* plog_pcm_path)
{
    if(NULL == papc)
    {
        lberror("Invalid parameter, papc:%p\n", papc);
        return -1;
    }

    if(papc->pnr_ctx)
    {
        lbaudio_close_noise_reduce_contextp(&papc->pnr_ctx);
    }
    
    papc->pnr_ctx = lbaudio_create_noise_reduce_context(papc->samplerate, nmode);
    if(NULL == papc->pnr_ctx)
    {
        lberror("papc->pnr_ctx:%p = lbaudio_create_noise_reduce_context(nmode:%d) failed\n", papc->pnr_ctx, nmode);
        return -1;
    }

    if(plog_pcm_path)
    {
        lbaudio_noise_reduce_log_pcm(papc->pnr_ctx, plog_pcm_path);
    }
    lbtrace("%s(papc:%p, nmode:%d, plog_pcm_path:%s) success, papc->pnr_ctx:%p\n", __func__, papc, nmode, plog_pcm_path, papc->pnr_ctx);
    return 0;
}


int lbadd_agc_filter(struct audio_proc_ctx* papc, int agcmode, const char* plog_pcm_path)
{
    if(NULL == papc)
    {
        lberror("Invalid parameter, papc:%p\n", papc);
        return -1;
    }

    if(papc->pagc_ctx)
    {
        lbaudio_close_agc_contextp(&papc->pagc_ctx);
    }
    lbtrace("%s(papc:%p, agcmode:%d, plog_pcm_path:%s)\n", __func__, papc, agcmode, plog_pcm_path);
    papc->pagc_ctx = lbaudio_create_agc_context(agcmode, papc->samplerate);
    if(NULL == papc->pagc_ctx)
    {
        lberror("papc->pagc_ctx:%p = lbaudio_create_agc_context(agcmode:%d, papc->samplerate:%d) failed\n", papc->pagc_ctx, agcmode, papc->samplerate);
        return -1;
    }
    
    if(plog_pcm_path)
    {
        lbaudio_agc_log_pcm(papc->pagc_ctx, plog_pcm_path);
    }
    lbtrace("%s(papc:%p, nmode:%d, plog_pcm_path:%s) success, papc->pagc_ctx:%p\n", __func__, papc, agcmode, plog_pcm_path, papc->pagc_ctx);
    return 0;
}

int lbaudio_proc_far(struct audio_proc_ctx* papc, char* pdata, int len, int64_t timestamp)
{
    if(NULL == papc || NULL == pdata || len <= 0)
    {
        lberror("Invalid parameter, papc:%p, pdata:%p, len:%d\n", papc, pdata, len);
        return -1;
    }
    lbdebug("lbaudio_proc_far(papc:%p, pdata:%p, len:%d, timestamp:%" PRId64 ")\n", papc, pdata, len, timestamp);
    if(NULL == papc->paec_ctx)
    {
        lberror("aec handle is not init, papc->paec_ctx:%p\n", papc->paec_ctx);
        return -1;
    }
    
    int ret = WebRtcAec_BufferFarend(papc->paec_ctx, pdata, len);
    if(ret < 0)
    {
        lberror("ret:%d = WebRtcAec_BufferFarend(papc->paec_ctx:%p, pdata:%p, len:%d)\n", ret, papc->paec_ctx, pdata, len);
        return ret;
    }
    return ret;
}

int lbaec_proc(struct audio_proc_ctx* papc, char* pfar, char* pnear, char* pout, int len)
{
    if(NULL == papc || NULL == pnear || len <= 0)
    {
        return -1;
    }

    int nbsamples = len / (papc->channel * av_get_bytes_per_sample(papc->sampleformat));
    int ret = WebRtcAec_BufferFarend(papc->paec_ctx, pfar, nbsamples);
    lbtrace("ret:%d = WebRtcAec_BufferFarend(papc->paec_ctx:%p, pfar:%p, nbsamples:%d), pout:%p, len:%d\n", ret, papc->paec_ctx, pfar, nbsamples, pout, len);
    ret = WebRtcAec_Process(papc->paec_ctx, pnear, NULL, pout, NULL, nbsamples, papc->ms_aec_delay, 0);
    lbtrace("ret:%d = WebRtcAec_Process(papc->paec_ctx, pnear, NULL, pout, NULL, nbsamples:%d, papc->ms_aec_delay:%d, 0)\n", ret, nbsamples, papc->ms_aec_delay);
    if(ret < 0)
    {
        int errcode = WebRtcAec_get_error_code(papc->paec_ctx);
        lbtrace("errcode:%d = WebRtcAec_get_error_code(papc->paec_ctx)\n", errcode);
    }
    int median = 0, std = 0;
    ret = WebRtcAec_GetDelayMetrics(papc->paec_ctx, &median, &std);
    lbtrace("ret:%d = WebRtcAec_GetDelayMetrics(papc->paec_ctx:%p, &median:%d, &std:%d), papc->ms_aec_delay:%d, papc->nlast_median:%d, abs(papc->lcur_near_timestamp - papc->llast_fix_timestamp):%d, papc->us_timestamp_delay:%d\n", ret, papc->paec_ctx, median, std, papc->ms_aec_delay, papc->nlast_median, abs(papc->lcur_near_timestamp - papc->llast_fix_timestamp), papc->us_timestamp_delay);
    if(median != papc->nlast_median && abs(median - papc->nlast_median) > 10 && abs(median) > 10 && abs(papc->lcur_near_timestamp - papc->llast_fix_timestamp) > 2000000)
    {
        /*papc->us_timestamp_delay += median*1000;
        lbtrace("fix aec delay, us_timestamp_delay:%d, median:%d, lcur_near_timestamp:%d, nlast_median:%d, llast_fix_timestamp:%" PRId64 ", cur_near_pts:%" PRId64 "", papc->us_timestamp_delay, median, papc->lcur_near_timestamp, papc->nlast_median, papc->llast_fix_timestamp, papc->lcur_near_timestamp);
        papc->nlast_median = median;
        papc->llast_fix_timestamp = papc->lcur_near_timestamp;
        //papc->discontinue = 1;*/
    }
#ifdef WRITE_FILE_TEST
    if(NULL == papc->pfar_file)
    {
        char* plogpath = get_log_path();
        char farpath[256];
        char nearpath[256];
        sprintf(farpath, "%s/aec_far_%d.pcm", plogpath, papc->msdelay);
        sprintf(nearpath, "%s/aec_near_%d.pcm", plogpath, papc->msdelay);
        lbtrace("plogpath:%s\n", plogpath);
        papc->pfar_file = fopen(farpath, "wb");
        papc->pnear_file = fopen(nearpath, "wb");
    }
    if(papc->pfar_file)
    {
        fwrite(pfar, 1, len, papc->pfar_file);
    }
    if(papc->pnear_file)
    {
        fwrite(pnear, 1, len, papc->pnear_file);
    }
#endif
#ifdef WRITE_OUTPUT_FILE
    if(NULL == papc->pout_file)
    {
        //char* plogpath = get_log_path();
        char outpath[256];
        sprintf(outpath, "%s/aec_out_%d.pcm", get_log_path(), papc->msdelay);
        papc->pout_file = fopen(outpath, "wb");
    }
    if(papc->pout_file)
    {
        fwrite(pout, 1, len, papc->pout_file);
    }
#endif
    return ret;
}

int lbaudio_proc_process(struct audio_proc_ctx* papc, char* pdata, char* pout, int len, int64_t timestamp)
{
    int ret = -1;
    int out_off = 0;
    int num = 0;
    int max_cycle_cnt = 1000;
    static int input_near_bytes = 0;
    static int inbytes = 0;
    static int outbytes = 0;
    if(NULL == papc || NULL == pdata || len <= 0)
    {
        lberror("Invalid parameter, papc:%p, pdata:%p, len:%d\n", papc, pdata, len);
        return -1;
    }
#ifdef WRITE_INPUT_DATA
    if(NULL == papc->pinput_near_file)
    {
        char* plogpath = get_log_path();
        char nearpath[256];
        sprintf(nearpath, "%s/input_near_%d.pcm", plogpath, papc->msdelay);
        char nearbuf[256];
        sprintf(nearbuf, "%s/near_buf_%d.pcm", plogpath, papc->msdelay);
        papc->pinput_near_file = fopen(nearpath, "wb");
        //papc->pnear_buf_file = fopen(nearbuf, "wb");
    }

    if(papc->pinput_near_file)
    {
        
        input_near_bytes += fwrite(pdata, 1, len, papc->pinput_near_file);
        //lbdebug("input_near_bytes:%d\n", input_near_bytes);
    }
    //memcpy(pout, pdata, len);
    //return len;
#endif

    if(timestamp < 0)
    {
        timestamp = lbget_system_timestamp_in_us();
    }

    pthread_mutex_lock(papc->pmutex);
    //timestamp -= papc->lstart_timestamp;
    timestamp = lbaudio_timestamp_fix(papc, len, timestamp, &papc->lcur_near_timestamp, papc->channel, papc->samplerate, papc->sampleformat);
    lbtrace("near audio timestamp:%" PRId64 ", papc->lcur_far_timestamp:%" PRId64"\n", timestamp, papc->lcur_near_timestamp);
    
    
    //lbdebug("papc:%p, pdata:%p, pout:%p, timestamp:%"PRId64", len:%d\n", papc, pdata, pout, timestamp, len);
    if(NULL == papc->paec_pcm_buf)
    {
        papc->paec_pcm_buf = lbsample_buffer_open(papc->channel, papc->samplerate, papc->sampleformat, papc->channel * papc->samplerate * av_get_bytes_per_sample(papc->sampleformat));
    }
    
    if(lbsample_buffer_size(papc->paec_pcm_buf) <= 0)
    {
        papc->laec_pcm_pts = timestamp;
    }
    
    int devlen = lbsample_buffer_deliver(papc->paec_pcm_buf, pdata, len, timestamp);
    inbytes += devlen;
    //lbdebug("size:%d, input_near_bytes:%d, devlen:%d, len:%d\n", lbsample_buffer_size(papc->paec_pcm_buf), input_near_bytes, devlen, len);
    while(papc->paec_ctx && lbsample_buffer_size(papc->paec_pcm_buf) >= papc->nbsample_bytes- papc->nbuf_offset && out_off <= len)
    {
        AVFrame* pf = NULL;
        int64_t pts = 0;
        int fetchlen = lbsample_buffer_fetch(papc->paec_pcm_buf, papc->pnbsample_buffer + papc->nbuf_offset, papc->nbsample_bytes - papc->nbuf_offset, &pts);
        papc->nbuf_offset += fetchlen;

        //lbdebug("lbsample_buffer_size(papc->paec_pcm_buf):%d, papc->nbsample_bytes:%d, fetchlen:%d, outbytes:%d, list_size(papc->plist_ctx):%d\n", lbsample_buffer_size(papc->paec_pcm_buf), papc->nbsample_bytes, fetchlen, outbytes, list_size(papc->plist_ctx));
        assert(fetchlen >= papc->nbsample_bytes);
        papc->laec_pcm_pts += papc->nbsamples * 1000000 /papc->samplerate;
        do{
            if(list_size(papc->plist_ctx) <= 0 && num++ <= max_cycle_cnt)
            {
                usleep(10000);
                continue;
            }
            if(list_size(papc->plist_ctx) <= 0 && papc->nbuf_offset >= papc->nbsample_bytes)//(num++ > max_cycle_cnt)
            {
                //memcpy(pout + out_off, papc->pnbsample_buffer, papc->nbsample_bytes);
                //out_off += fetchlen;

                /*lbsample_buffer_reset(papc->paec_pcm_buf);*/
                //papc->discontinue = 0;
                //lbdebug("not far data, num:%d, fetchlen:%d\n", num, fetchlen);
                /*if(out_off >= len)
                {
                    break;
                }*/
                /*if(lbsample_buffer_size(papc->paec_pcm_buf) < papc->nbsample_bytes)
                {
                    break;
                }
                fetchlen = lbsample_buffer_fetch(papc->paec_pcm_buf, papc->pnbsample_buffer, papc->nbsample_bytes, &pts);
                if(fetchlen < papc->nbsample_bytes)
                {
                    break;
                }*/
                
                break;
                //pthread_mutex_unlock(papc->pmutex);
                //return len;
            }
            num = 0;
            AVFrame* pframe = front(papc->plist_ctx);
            //lbdebug("pframe:%p = front(papc->plist_ctx)\n", pframe);
            pf = NULL;//= pop(papc->plist_ctx);
            if(NULL == pframe)
            {
                continue;
            }
            lbdebug("papc->discontinue:%d && pf->pts:%" PRId64 " + papc->us_timestamp_delay:%d - 5000 < papc->laec_pcm_pts:%" PRId64 "= %d, pf->nb_samples:%d\n", papc->discontinue, pframe->pts, papc->us_timestamp_delay, papc->laec_pcm_pts, abs(pframe->pts + papc->us_timestamp_delay - papc->laec_pcm_pts), pframe->nb_samples);
            if(papc->discontinue)
            {
                /*if(INT64_MIN == papc->lstart_drop_timestamp)
                {
                    papc->lstart_drop_timestamp = ;
                }*/
                //far delay
                if(abs(pframe->pts + papc->us_timestamp_delay - papc->laec_pcm_pts) <= 5000)
                //if(abs(pf->pts - papc->us_timestamp_delay - timestamp) <= 10000 || pf->pts - papc->us_timestamp_delay >= timestamp)
                {
                    papc->discontinue = 0;
                    lbtrace("discontinue end, pf->pts:%" PRId64 " + papc->us_timestamp_delay:%" PRId64 " - papc->laec_pcm_pts:%" PRId64 " <= 5000\n", pframe->pts, papc->us_timestamp_delay, papc->laec_pcm_pts);
                    pf = pop(papc->plist_ctx);
                    break;
                }
                else if(pframe->pts + papc->us_timestamp_delay - papc->laec_pcm_pts < -5000)
                {
                    lbdebug("drop far frame, pframe->pts:%" PRId64 " + papc->us_timestamp_delay:%d - papc->laec_pcm_pts:%" PRId64 " = %" PRId64 " < -5000", pframe->pts, papc->us_timestamp_delay, papc->laec_pcm_pts, pframe->pts + papc->us_timestamp_delay - papc->laec_pcm_pts);
                    pf = pop(papc->plist_ctx);
                    av_frame_free(&pf);
                    //break;
                }
                else
                {
                    papc->nbuf_offset = 0;
                    lbdebug("drop near farame, pframe->pts:%" PRId64 " + papc->us_timestamp_delay:%d - papc->laec_pcm_pts:%" PRId64 " = %" PRId64 " > 5000", pframe->pts, papc->us_timestamp_delay, papc->laec_pcm_pts, pframe->pts + papc->us_timestamp_delay - papc->laec_pcm_pts);
                    break;
                }
            }
            else
            {
                pf = pop(papc->plist_ctx);
                lbdebug("pf:%p = pop(papc->plist_ctx)\n", pf);
            }
        }while(papc->discontinue);
        
        if(pf)
        {
            lbtrace("pf->pts:%" PRId64 " linesize[0]:%d, papc->laec_pcm_pts:%" PRId64 ", papc->nbsample_bytes:%d, out_off:%d, aec ts delay:%" PRId64 "\n", pf->pts, pf->linesize[0], papc->laec_pcm_pts, papc->nbsample_bytes, out_off, papc->laec_pcm_pts - pf->pts);
            ret = lbaec_proc(papc, pf->data[0], papc->pnbsample_buffer, pout + out_off, papc->nbuf_offset);
            av_frame_free(&pf);
            papc->nbuf_offset = 0;
        }
        else
        {
            lbdebug("Invalid pf:%p, out_off:%d, papc->nbsample_bytes:%d\n", pf, out_off, papc->nbsample_bytes);
            memcpy(pout + out_off, papc->pnbsample_buffer, papc->nbsample_bytes);
#ifdef WRITE_FILE_TEST
            if(papc->pnear_file)
            {
                fwrite(papc->pnbsample_buffer, 1, papc->nbsample_bytes, papc->pnear_file);
            }
#endif
#ifdef WRITE_OUTPUT_FILE
            if(papc->pout_file)
            {
                fwrite(pout, 1, len, papc->pout_file);
            }
#endif
        }
        out_off += papc->nbsample_bytes;
    }
    ret = out_off;
    if(out_off >= 640)
    {
        if(papc->pnr_ctx)
        {
            
            ret = lbaudio_noise_reduce(papc->pnr_ctx, pout, out_off);
            lbtrace("ret:%d = lbaudio_noise_reduce(papc->pnr_ctx, pout:%p, out_off:%d)\n", ret, pout, out_off);
        }

        if(papc->pagc_ctx)
        {
            ret = lbaudio_agc_process(papc->pagc_ctx, pout, out_off);
            lbtrace("ret:%d = lbaudio_agc_process(papc->pagc_ctx, pout:%p, out_off:%d)\n", ret, pout, out_off);
        }
    }

    pthread_mutex_unlock(papc->pmutex);
    lbtrace("lbaudio_proc_process end, ret:%d, papc->ms_aec_delay:%d, insize:%d, outsize:%d, inbytes:%d, outbytes:%d, papc->pnr_ctx:%p, papc->pagc_ctx:%p\n", ret, papc->ms_aec_delay, len, out_off, inbytes, outbytes, papc->pnr_ctx, papc->pagc_ctx);
    if(len != out_off || inbytes != outbytes)
    {
        lbdebug("pcm data maybe drop, inbytes:%d, outbytes:%d\n", inbytes, outbytes);
    }
    return ret;
}

int64_t lbget_system_timestamp_in_us()
{
    /*struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t ts = tv.tv_sec;
    ts = ts * 1000000;
    ts += tv.tv_usec;
    
    return ts;*/
    return av_gettime_relative();
}

void lbaudio_proc_close_contextp(struct audio_proc_ctx** ppapc)
{
    if(ppapc && *ppapc)
    {
        struct audio_proc_ctx* papc = *ppapc;
        lbtrace("lbaudio_proc_close_contextp papc:%p\n", papc);
        if(papc->paec_ctx)
        {
            WebRtcAec_Free(papc->paec_ctx);
            papc->paec_ctx = NULL;
        }
        lbdebug("lbaudio_proc_close_contextp papc after papc->paec_ctx\n");
#ifdef ENABLE_NS_PROCESS
        if(papc->pnr_ctx)
        {
            if(papc->pnr_ctx->pns_handle)
            {
                WebRtcNs_Free(papc->pnr_ctx->pns_handle);
                papc->pnr_ctx->pns_handle = NULL;
            }
        }
#endif
        lbdebug("lbaudio_proc_close_contextp papc after papc->pnr_ctx->pns_handle\n");
        if(papc->pagc_ctx)
        {
            WebRtcAgc_Free(papc->pagc_ctx);
            papc->pagc_ctx = NULL;
        }
        lbdebug("lbaudio_proc_close_contextp papc after papc->pagc_ctx\n");
        if(papc->plist_ctx)
        {
            while(list_size(papc->plist_ctx) > 0)
            {
                AVFrame* pf = pop(papc->plist_ctx);
                if(pf)
                {
                    lbdebug("av_frame_free(&pf:%p), list_size(papc->plist_ctx):%d\n", pf, list_size(papc->plist_ctx));
                    av_frame_free(&pf);
                }
            };
            list_context_close(&papc->plist_ctx);
            papc->plist_ctx = NULL;
        }
        if(papc->ptmp_frame)
        {
            av_frame_free(&papc->ptmp_frame);
        }

        if(papc->pmutex)
        {
            pthread_mutex_destroy(papc->pmutex);
            free(papc->pmutex);
        }
#ifdef WRITE_FILE_TEST
    if(papc->pfar_file)
    {
        fclose(papc->pfar_file);
        papc->pfar_file = NULL;
    }

    if(papc->pnear_file)
    {
        fclose(papc->pnear_file);
        papc->pnear_file = NULL;
    }

    if(papc->pout_file)
    {
        fclose(papc->pout_file);
        papc->pout_file = NULL;
    }
#endif
        
#ifdef WRITE_INPUT_DATA
    if(papc->pinput_near_file)
    {
        fclose(papc->pinput_near_file);
        papc->pinput_near_file = NULL;
    }

    if(papc->pinput_far_file)
    {
        fclose(papc->pinput_far_file);
        papc->pinput_far_file = NULL;
    }
#endif

    if(papc->pnr_ctx)
    {
        lbaudio_close_noise_reduce_contextp(&papc->pnr_ctx);
        papc->pnr_ctx = NULL;
    }

    if(papc->pagc_ctx)
    {
        lbaudio_close_agc_contextp(&papc->pagc_ctx);
        papc->pagc_ctx = NULL;
    }

    if(papc->pnbsample_buffer)
    {
        free(papc->pnbsample_buffer);
        papc->pnbsample_buffer = NULL;
    }

    lbdebug("lbaudio_proc_close_contextp after papc->plist_ctx\n");
    free(papc);
    lbtrace("lbaudio_proc_close_contextp after free(papc)\n");
    *ppapc = papc = NULL;
    }
}

int lbaudio_check_resample_enable(struct audio_proc_ctx* papc, int channel, int samplerate, int format)
{
    if(papc)
    {
        if(channel != papc->channel ||  samplerate != papc->samplerate || format != papc->sampleformat)
        {
            if(NULL == papc->prc)
            {
                papc->prc = avresample_init(NULL, papc->channel, papc->samplerate, papc->sampleformat);
                sv_trace("papc->prc:%p = avresample_init(NULL, papc->channel:%d, papc->samplerate:%d, papc->sampleformat:%d)\n", papc->prc, papc->channel, papc->samplerate, papc->sampleformat);
            }
            
            return 1;
        }
        else
        {
            return 0;
        }
    }
    
    return 0;
}

struct AVFrame* lbaudio_make_frame(char* pdata, int len, int64_t timestamp, int channel, int samplerate, int samformat)
{
    AVFrame* pframe = av_frame_alloc();
    pframe->channels = channel;
    pframe->sample_rate = samplerate;
    pframe->format = samformat;
    pframe->pts = timestamp;
    pframe->nb_samples = len / (channel * av_get_bytes_per_sample((enum AVSampleFormat)samformat));
    
    av_frame_get_buffer(pframe, 4);
    if(pframe->linesize[0] < len)
    {
        lberror("pframe->linesize[0]:%d < len:%d\n", pframe->linesize[0], len);
        return -1;
    }
    memcpy(pframe->data[0], pdata, len);
    
    return pframe;
}

int64_t lbaudio_timestamp_fix(struct audio_proc_ctx* papc, int len, int64_t timestamp, int64_t* pmod_timestamp, int channel, int samplerate, int format)
{
    if(NULL == papc || NULL == pmod_timestamp)
    {
        lberror("Invalid parameter, papc:%p, len:%d, timestamp:%" PRId64 ", pmod_timestamp:%p\n", papc, len, timestamp, pmod_timestamp);
        return -1;
    }
    //int64_t timestamp = *ptimestamp;
    int64_t mod_timestamp = *pmod_timestamp;
    //int64_t out_pts = timestamp;
    int64_t pcm_dur = len*1000000 / (channel * av_get_bytes_per_sample(format) * samplerate);
    //lbtrace("pcm_dur:%d = len:%d / (channel:%d * av_get_bytes_per_sample(format:%d) * samplerate:%d)\n", pcm_dur, len, channel, format, samplerate);
    if(-1 == timestamp)
    {
        timestamp = lbget_system_timestamp_in_us();;
    }
    //papc->lstart_timestamp = INT64_MIN;//lbget_system_timestamp_in_us();
    if(INT64_MIN == papc->lstart_timestamp)
    {
        lbtrace("papc->lstart_timestamp:%"PRId64", timestamp:%" PRId64 "", papc->lstart_timestamp, timestamp);
        papc->lstart_timestamp = timestamp;
        mod_timestamp = 0;
        timestamp = 0;
        //out_pts = 0;
    }
    else
    {
        timestamp -= papc->lstart_timestamp;
        if(INT64_MIN == mod_timestamp)
        {
            mod_timestamp = timestamp;
            //out_pts = timestamp;
        }
        else
        {
            mod_timestamp += pcm_dur;
            //out_pts = mod_timestamp;
        }
    }

    if(abs((int)(mod_timestamp - timestamp)) > 200000)
    {
        lbtrace("audio maybe drop, abs(timestamp%:%" PRId64 " - mod_timestamp:%" PRId64 ") = %d > 200000, mod_timestamp change %" PRId64 "\n", timestamp, mod_timestamp, abs((int)(mod_timestamp - timestamp)), mod_timestamp);
        mod_timestamp = timestamp;
    }
    if(pmod_timestamp)
    {
        *pmod_timestamp = mod_timestamp;
    }
    lbdebug("new timestamp:%" PRId64 ", org timestamp:%" PRId64 ", pcm_dur:%" PRId64 "", mod_timestamp, timestamp, pcm_dur);
    return mod_timestamp;
}

int lbaudio_get_estimate_delay(struct audio_proc_ctx* papc)
{
    
    return 0;
}
#ifdef LOAD_AEC_DELAY_CONFIG_FROM_FILE
int lbaudio_load_delay_from_config(const char* ppath, const char* pcfgname)
{
    char cfg_path[256];
    sprintf(cfg_path, "%s/%s", ppath, pcfgname);
    FILE* pfile = fopen(cfg_path, "rb");
    int ms_delay = INT32_MIN;
    if(pfile)
    {
        char line[256];
        int readlen = fread(line, 1, 256, pfile);
        sscanf(line, "%d", &ms_delay);
        fclose(pfile);
        lbtrace("line:%s, ms_delay:%d\n", line, ms_delay);
    }
    return ms_delay;
}
#endif

struct agc_context* lbaudio_create_agc_context(int mode, int samplerate)
{
    lbtrace("%s(mode:%d, samplerate:%d)\n", __func__, mode, samplerate);
    struct agc_context* pagc = (struct agc_context*)malloc(sizeof(struct agc_context));
    memset(pagc, 0, sizeof(struct agc_context));
    pagc->agc_mode = mode;
    pagc->samplerate = samplerate;
    pagc->min_level = 0;
    pagc->max_level = 255;
    if(samplerate == 8000)
    {
        pagc->frame_size = 80;
    }
    else
    {
        pagc->frame_size = 160;
    }
    pagc->config.compressionGaindB = 20;
	pagc->config.limiterEnable     = 1;
	pagc->config.targetLevelDbfs   = 3;
    int ret = WebRtcAgc_Create(&pagc->pagc_handle);
    if(ret != 0 || NULL == pagc->pagc_handle)
    {
        lberror("ret:%d = WebRtcAgc_Create(&pagc->pagc_handle:%p) failed\n", ret, pagc->pagc_handle);
        free(pagc);
        return NULL;
    }
    WebRtcAgc_Init(pagc->pagc_handle, pagc->min_level, pagc->max_level, pagc->agc_mode, pagc->samplerate);
    WebRtcAgc_set_config(pagc->pagc_handle, pagc->config);
    pagc->ncopy_buf_size = pagc->frame_size * sizeof(short);
    pagc->pcopy_buf = (short*)malloc(pagc->ncopy_buf_size);

    pagc->mic_level_in = 0;
    pagc->mic_level_out = 0;

    return pagc;
}

void lbaudio_agc_log_pcm(struct agc_context* pagc, const char* plog_path)
{
    if(NULL == pagc || NULL == plog_path)
    {
        lberror("Invalid param pagc:%p, plog_path:%p, ignore\n", pagc, plog_path);
        return ;
    }
    if(pagc->pagc_output_file)
    {
        fclose(pagc->pagc_output_file);
        pagc->pagc_output_file = NULL;
    }

    if(plog_path && NULL == pagc->pagc_output_file)
    {
        pagc->pagc_output_file = fopen(plog_path, "wb");
        lbtrace("pagc->pagc_output_file:%p = fopen(plog_path:%s, wb)\n", pagc->pagc_output_file, plog_path);
    }
    lbtrace("%s(pagc:%p, plog_path:%s) end,  pagc->pagc_output_file:%p\n", __func__, pagc, plog_path,pagc->pagc_output_file);
}

int lbaudio_agc_process(struct agc_context* pagc, char* ppcm_data, int data_len)
{
    int ret = 0;
    uint8_t saturationWarning = 0;
    int offset = 0;
    lbtrace("%s(pagc:%p, ppcm_data:%p, data_len:%d)", __func__, pagc, ppcm_data, data_len);
    if(NULL == pagc || NULL == ppcm_data || (data_len % pagc->ncopy_buf_size) != 0)
    {
        lberror("Invalid param, pagc:%p, ppcm_data:%p or data_len:%d %% pagc->ncopy_buf_size:%d != 0\n", pagc, ppcm_data, data_len, pagc->ncopy_buf_size);
        return -1;
    }

    do
    {
        //int copy_len = data_len - offset > pagc->ncopy_buf_size ? pagc->ncopy_buf_size : 
        memcpy(pagc->pcopy_buf, ppcm_data + offset, pagc->ncopy_buf_size);
        ret = WebRtcAgc_Process(pagc->pagc_handle, pagc->pcopy_buf, NULL, pagc->frame_size, ppcm_data + offset, NULL, pagc->mic_level_in, &pagc->mic_level_out, 0, &saturationWarning);
        lbtrace("ret:%d = WebRtcAgc_Process(pagc->pagc_handle:%p, ...)", ret, pagc->pagc_handle);
        if(0 != ret)
        {
            lberror("ret:%d = WebRtcAgc_Process(pagc->pagc_handle:%p, pagc->pcopy_buf:%p, NULL, pagc->frame_size:%d, ppcm_data + offset:%p, NULL, pagc->mic_level_in:%d, &pagc->mic_level_out:%d, 0, &saturationWarning:%d) failed\n", ret, pagc->pagc_handle, pagc->pcopy_buf, pagc->frame_size, ppcm_data + offset, pagc->mic_level_in, pagc->mic_level_out, (int)saturationWarning);
            break;
        }
        pagc->mic_level_in = pagc->mic_level_out;
        //data_len -= pagc->ncopy_buf_size;
        offset += pagc->ncopy_buf_size;
    } while (offset < data_len);
    
    if(pagc->pagc_output_file && offset > 0)
    {
        int writed = fwrite(ppcm_data, 1, offset, pagc->pagc_output_file);
        lbtrace("writed:%d = fwrite(ppcm_data:%p, offset:%d, pagc->pagc_output_file:%p)\n", writed, ppcm_data, offset, pagc->pagc_output_file);
    }

    lbtrace("offset:%d = lbaudio_agc_process end\n", offset);
    return offset;
}

void lbaudio_close_agc_contextp(struct agc_context** ppagc)
{
    if(ppagc && *ppagc)
    {
        struct agc_context* pagc = *ppagc;
        WebRtcAgc_Free(pagc->pagc_handle);
        if(pagc->pcopy_buf)
        {
            free(pagc->pcopy_buf);
            pagc->pcopy_buf = NULL;
        }

        free(pagc);
    }
}

struct noise_reduce_context* lbaudio_create_noise_reduce_context(int samplerate, int nr_mode)
{
    struct noise_reduce_context* pnr_ctx = (struct noise_reduce_context*)malloc(sizeof(struct noise_reduce_context));
    memset(pnr_ctx, 0, sizeof(struct noise_reduce_context));
    int ret = WebRtcNs_Create(&pnr_ctx->pns_handle);
    lbtrace("ret:%d = WebRtcNs_Create(&pnr_ctx->pns_handle:%p)\n", ret, pnr_ctx->pns_handle);
    pnr_ctx->samplerate = samplerate;
    pnr_ctx->nr_mode = nr_mode;
    pnr_ctx->nframe_bytes = 320*sizeof(short);
    memset(pnr_ctx->flt_state_1, 0, sizeof(int)*6);
    memset(pnr_ctx->flt_state_2, 0, sizeof(int)*6);
    memset(pnr_ctx->syn_state_1, 0, sizeof(int)*6);
    memset(pnr_ctx->syn_state_2, 0, sizeof(int)*6);
    memset(pnr_ctx->sh_in_low, 0, sizeof(short)*160);
    memset(pnr_ctx->sh_in_high, 0, sizeof(short)*160);
    memset(pnr_ctx->sh_out_low, 0, sizeof(short)*160);
    memset(pnr_ctx->sh_out_high, 0, sizeof(short)*160);
    WebRtcNs_Init(pnr_ctx->pns_handle, samplerate);
    lbtrace("ret:%d = WebRtcNs_Init(pnr_ctx->pns_handle:%p, samplerate:%d)\n", ret, pnr_ctx->pns_handle, pnr_ctx->pns_handle, samplerate);
    WebRtcNs_set_policy(pnr_ctx->pns_handle, pnr_ctx->nr_mode);
    lbtrace("ret:%d = WebRtcNs_set_policy(pnr_ctx->pns_handle:%p, nr_mode:%d)\n", ret, pnr_ctx->pns_handle, pnr_ctx->pns_handle, nr_mode);

    return pnr_ctx;
}

void lbaudio_noise_reduce_log_pcm(struct noise_reduce_context* pnr_ctx, const char* plog_path)
{
    if(NULL == pnr_ctx || NULL == plog_path)
    {
        lberror("Invalid param pnr_ctx:%p, plog_path:%p, ignore\n", pnr_ctx, plog_path);
        return ;
    }
    //lbtrace("%s(pagc:%p, plog_path:%s) begin,  pagc->pnr_input_file:%p, pagc->pnr_output_file:%p\n", __func__, pnr_ctx, plog_path, pnr_ctx->pnr_output_file);
    
    if(pnr_ctx->pnr_output_file)
    {
        fclose(pnr_ctx->pnr_output_file);
        pnr_ctx->pnr_output_file = NULL;
    }
    if(plog_path && NULL == pnr_ctx->pnr_output_file)
    {
        pnr_ctx->pnr_output_file = fopen(plog_path, "wb");
        lbtrace("pnr_ctx->pnr_output_file:%p = fopen(plog_path:%s, wb)\n",  pnr_ctx->pnr_output_file, plog_path);
    }
    lbtrace("%s(pagc:%p, plog_path:%s) end, pagc->pnr_output_file:%p\n", __func__, pnr_ctx, plog_path, pnr_ctx->pnr_output_file);
    return ;
}

// 使用高频和低频数据分类的方式进行降噪
int lbaudio_noise_reduce(struct noise_reduce_context* pnr_ctx, char* ppcm_data, int data_len)
{
    int ret = -1;
    int offset = 0;
    lbdebug("%s(pnr_ctx:%p, ppcm_data:%p, data_len:%d)", __func__, pnr_ctx, ppcm_data, data_len);
    if(NULL == pnr_ctx || NULL == ppcm_data || data_len % pnr_ctx->nframe_bytes != 0)
    {
        lberror("Invalid param, pnr_ctx:%p, ppcm_data:%p, data_len:%d %% pnr_ctx->nframe_bytes:%d != 0\n", pnr_ctx, ppcm_data, data_len, pnr_ctx->nframe_bytes);
        return -1;
    }
    static FILE* pfile = NULL;
    if(NULL == pfile)
    {
        char str[256];
        sprintf(str, "%s/ns_in.pcm", get_log_path());
        pfile = fopen(str, "wb");
    }
    if(pfile)
    {
        fwrite(ppcm_data, 1, data_len, pfile);
    }
    do
    {
        //首先需要使用滤波函数将音频数据分高低频，以高频和低频的方式传入降噪函数内部（分离高频和低频声音数据）
        WebRtcSpl_AnalysisQMF((const int16_t*)(ppcm_data + offset), pnr_ctx->sh_in_low, pnr_ctx->sh_in_high, pnr_ctx->flt_state_1, pnr_ctx->flt_state_2);
        //WebRtcSpl_AnalysisQMF((const int16_t*)(ppcm_data + offset), pnr_ctx->sh_out_low, pnr_ctx->sh_out_high, pnr_ctx->flt_state_1, pnr_ctx->flt_state_2);
        /*if(0 != ret)
        {
            lberror("ret:%d = WebRtcSpl_AnalysisQMF failed\n", ret);
            return -1;
        }*/
        lbdebug("WebRtcSpl_AnalysisQMF(ppcm_data:%p, pnr_ctx->sh_in_low:%p, pnr_ctx->sh_in_high:%p, pnr_ctx->flt_state_1:%p, pnr_ctx->flt_state_2:%p)\n", ppcm_data, pnr_ctx->sh_in_low, pnr_ctx->sh_in_high, pnr_ctx->flt_state_1, pnr_ctx->flt_state_2);
        //将需要降噪的数据以高频和低频传入对应接口，同时需要注意返回数据也是分高频和低频（对高频和低频数据进行降噪处理）
        ret = WebRtcNs_Process(pnr_ctx->pns_handle, pnr_ctx->sh_in_low, pnr_ctx->sh_in_high, pnr_ctx->sh_out_low, pnr_ctx->sh_out_high);
        if(0 != ret)
        {
            lberror("ret:%d = WebRtcNs_Process failed, pnr_ctx->pns_handle:%p\n", ret, pnr_ctx->pns_handle);
            return -1;
        }
        lbtrace("ret:%d = WebRtcNs_Process(pnr_ctx->pns_handle:%p, pnr_ctx->sh_in_low:%p, pnr_ctx->sh_in_high:%p, pnr_ctx->sh_out_low:%p, pnr_ctx->sh_out_high:%p)\n", ret, pnr_ctx->pns_handle, pnr_ctx->sh_in_low, pnr_ctx->sh_in_high, pnr_ctx->sh_out_low, pnr_ctx->sh_out_high);
        // 降噪成功，还原音频数据
        WebRtcSpl_SynthesisQMF(pnr_ctx->sh_out_low, pnr_ctx->sh_out_high, (const int16_t*)(ppcm_data + offset), pnr_ctx->syn_state_1, pnr_ctx->syn_state_2);
        offset += pnr_ctx->nframe_bytes;
    } while (offset < data_len);
    
    

    if(pnr_ctx->pnr_output_file && offset > 0)
    {
        int writed = fwrite(ppcm_data, 1, offset, pnr_ctx->pnr_output_file);
        lbtrace("writed:%d = fwrite(ppcm_data:%p, 1, offset:%d, pnr_ctx->pnr_output_file:%p)\n", writed, ppcm_data, offset, pnr_ctx->pnr_output_file);
    }

    lbtrace("%s end, pnr_ctx->pnr_output_file:%p, offset:%d\n", __func__, pnr_ctx->pnr_output_file, offset);
    return offset;
}

void lbaudio_close_noise_reduce_contextp(struct noise_reduce_context** ppnr_ctx)
{
    if(ppnr_ctx && *ppnr_ctx)
    {
        struct noise_reduce_context* pnr_ctx = *ppnr_ctx;
        if(pnr_ctx->pns_handle)
        {
            WebRtcNs_Free(pnr_ctx->pns_handle);
            pnr_ctx->pns_handle = NULL;
        }
        free(pnr_ctx);
        *ppnr_ctx = pnr_ctx = NULL;
    }
}

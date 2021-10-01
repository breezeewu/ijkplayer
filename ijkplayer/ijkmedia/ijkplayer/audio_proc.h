#ifndef _AUDIO_PROCESS_H_
#define _AUDIO_PROCESS_H_
#define ENABLE_NS_PROCESS
#define LOAD_AEC_DELAY_CONFIG_FROM_FILE
#define NB_SAMPLES 80
#define BAND_FRAME_LENGTH 160*sizeof(short)
typedef void (*play_audio_callback)(void* pvoid, char* pdata, int len, int64_t timestamp, int channel, int samplerate, int samformat);

int lbfar_audio_callback(void* powner, char* pdata, int len, int64_t timestamp, int channel, int samplerate, int samformat);

struct audio_proc_ctx* lbaudio_proc_open_context(int channel, int samplerate, int samformat, int nbsamples);

int lbadd_aec_filter(struct audio_proc_ctx* papc, int far_channel, int far_samplerate, int samformat, int msdelay);

int laddd_aec_filterex(struct audio_proc_ctx* papc, int msdelay);

int lbadd_noise_reduce_filter(struct audio_proc_ctx* papc, int nmode, const char* plog_pcm_path);

int lbadd_agc_filter(struct audio_proc_ctx* papc, int agcmode, const char* plog_pcm_path);

int lbaudio_proc_far(struct audio_proc_ctx* papc, char* pdata, int len, int64_t timestamp);
int lbaec_proc(struct audio_proc_ctx* papc, char* pfar, char* pnear, char* pout, int len);
int lbaudio_proc_process(struct audio_proc_ctx* papc, char* pdata, char* pout, int len, int64_t timestamp);

int64_t lbget_system_timestamp_in_us();

void lbaudio_proc_close_contextp(struct audio_proc_ctx** ppapc);

int lbaudio_check_resample_enable(struct audio_proc_ctx* papc, int channel, int samplerate, int fromat);

struct AVFrame* lbaudio_make_frame(char* pdata, int len, int64_t timestamp, int channel, int samplerate, int samformat);
//void lbaudio_proc_close_contextp(struct audio_proc_ctx** ppapc);
int64_t lbaudio_timestamp_fix(struct audio_proc_ctx* papc, int len, int64_t timestamp, int64_t* pmod_timestamp, int channel, int samplerate, int format);
//int64_t lbaudio_timestamp_fix(struct audio_proc_ctx* papc, int len, int64_t* ptimestamp, int64_t* pmod_timestamp);

int lbaudio_get_estimate_delay(struct audio_proc_ctx* papc);
#ifdef LOAD_AEC_DELAY_CONFIG_FROM_FILE
int lbaudio_load_delay_from_config(const char* ppath, const char* pcfgname);
#endif

struct agc_context* lbaudio_create_agc_context(int mode, int samplerate);

//void lbaudio_agc_log_pcm(agc_ctx* pagc, const char* plog_path, int bebable_in, int benable_out)
void lbaudio_agc_log_pcm(struct agc_context* pagc, const char* plog_path);

int lbaudio_agc_process(struct agc_context* pagc, char* ppcm_data, int data_len);

void lbaudio_close_agc_contextp(struct agc_context** ppagc);

struct noise_reduce_context* lbaudio_create_noise_reduce_context(int samplerate, int nr_mode);

void lbaudio_noise_reduce_log_pcm(struct noise_reduce_context* pnr_ctx, const char* plog_path);

// 使用高频和低频数据分类的方式进行降噪
int lbaudio_noise_reduce(struct noise_reduce_context* pnr_ctx, char* ppcm_data, int data_len);

void lbaudio_close_noise_reduce_contextp(struct noise_reduce_context** ppnr_ctx);
#endif

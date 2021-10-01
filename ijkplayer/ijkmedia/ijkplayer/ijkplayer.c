/*
 * ijkplayer.c
 *
 * Copyright (c) 2013 Bilibili
 * Copyright (c) 2013 Zhang Rui <bbcallen@gmail.com>
 *
 * This file is part of ijkPlayer.
 *
 * ijkPlayer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * ijkPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ijkPlayer; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "ijkplayer.h"
#include "ijkplayer_internal.h"
#include "ijkversion.h"
#include "avmuxer.h"
#include "avrecord.h"
#include "audio_proc.h"
#include "lazylog.h"
#include "lbsc_util_conf.h"
#include "lbsc_media_parser.h"
#include "ipc_record.h"

#define MP_RET_IF_FAILED(ret) \
    do { \
        int retval = ret; \
        if (retval != 0) return (retval); \
    } while(0)

#define MPST_RET_IF_EQ_INT(real, expected, errcode) \
    do { \
        if ((real) == (expected)) return (errcode); \
    } while(0)

#define MPST_RET_IF_EQ(real, expected) \
    MPST_RET_IF_EQ_INT(real, expected, EIJK_INVALID_STATE)
log_ctx* g_plogctx = NULL;
conf_ctx* g_pconf_ctx = NULL;
SDL_mutex* g_pmutex = NULL;// = SDL_CreateMutex();
inline static void ijkmp_destroy(IjkMediaPlayer *mp)
{
    if (!mp)
        return;

    ffp_destroy_p(&mp->ffplayer);
    if (mp->msg_thread) {
        SDL_WaitThread(mp->msg_thread, NULL);
        mp->msg_thread = NULL;
    }

    pthread_mutex_destroy(&mp->mutex);

    freep((void**)&mp->data_source);
    memset(mp, 0, sizeof(IjkMediaPlayer));
    freep((void**)&mp);
}

inline static void ijkmp_destroy_p(IjkMediaPlayer **pmp)
{
    if (!pmp)
        return;

    ijkmp_destroy(*pmp);
    *pmp = NULL;
}

void ijkmp_global_init()
{
    ffp_global_init();
}
void ijkmp_global_init_log(const char* path, int log_level, int log_output_mode)
{
    sv_init_log(path, log_level, log_output_mode, IJK_PLAYER_VERSION_MAYJOR, IJK_PLAYER_VERSION_MINOR, IJK_PLAYER_VERSION_MICRO, IJK_PLAYER_VERSION_TINY);
    clean_overdur_log(g_plogctx, 1);
    av_log_set_callback(ffmpeg_log_callback);
#ifdef ENABLE_SDK_CONFIG_FILE
    if(get_log_path())
    {
        char path[256];
        sprintf(path, "%s/%s", get_log_path(), "config.ini");
        g_pconf_ctx = load_config(path, NULL, NULL);
    }
#endif
}
void ijkmp_global_uninit()
{
    ffp_global_uninit();
    sv_deinit_log();
}

void ijkmp_global_set_log_report(int use_report)
{
    ffp_global_set_log_report(use_report);
}

/*void ijkmp_global_set_log_level(int log_level)
{
    ffp_global_set_log_level(log_level);
}*/

void ijkmp_global_set_inject_callback(ijk_inject_callback cb)
{
    ffp_global_set_inject_callback(cb);
}

const char *ijkmp_version()
{
    
    return IJKPLAYER_VERSION;
}

void ijkmp_io_stat_register(void (*cb)(const char *url, int type, int bytes))
{
    ffp_io_stat_register(cb);
}

void ijkmp_io_stat_complete_register(void (*cb)(const char *url,
                                                int64_t read_bytes, int64_t total_size,
                                                int64_t elpased_time, int64_t total_duration))
{
    ffp_io_stat_complete_register(cb);
}

void ijkmp_change_state_l(IjkMediaPlayer *mp, int new_state)
{
    mp->mp_state = new_state;
    ffp_notify_msg2(mp->ffplayer, FFP_MSG_PLAYBACK_STATE_CHANGED, mp->mp_state);
    //ffp_notify_msg1(mp->ffplayer, FFP_MSG_PLAYBACK_STATE_CHANGED);
}

IjkMediaPlayer *ijkmp_create(int (*msg_loop)(void*))
{
    IjkMediaPlayer *mp = (IjkMediaPlayer *) mallocz(sizeof(IjkMediaPlayer));
    if (!mp)
        goto fail;

    mp->ffplayer = ffp_create();
    if (!mp->ffplayer)
        goto fail;

    mp->msg_loop = msg_loop;

    ijkmp_inc_ref(mp);
    //pthread_mutex_init(&mp->mutex, NULL);
    pthread_mutexattr_t attr;
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mp->mutex, &attr);
    return mp;

    fail:
    ijkmp_destroy_p(&mp);
    return NULL;
}

void *ijkmp_set_inject_opaque(IjkMediaPlayer *mp, void *opaque)
{
    check_pointer(mp, NULL);

    MPTRACE("%s(%p)\n", __func__, opaque);
    void *prev_weak_thiz = ffp_set_inject_opaque(mp->ffplayer, opaque);
    MPTRACE("%s()=void\n", __func__);
    return prev_weak_thiz;
}

void ijkmp_set_frame_at_time(IjkMediaPlayer *mp, const char *path, int64_t start_time, int64_t end_time, int num, int definition)
{
    if(NULL == mp) return;

    MPTRACE("%s(%s,%lld,%lld,%d,%d)\n", __func__, path, start_time, end_time, num, definition);
    ffp_set_frame_at_time(mp->ffplayer, path, start_time, end_time, num, definition);
    MPTRACE("%s()=void\n", __func__);
}


void *ijkmp_set_ijkio_inject_opaque(IjkMediaPlayer *mp, void *opaque)
{
    if(NULL == mp) return NULL;

    MPTRACE("%s(%p)\n", __func__, opaque);
    void *prev_weak_thiz = ffp_set_ijkio_inject_opaque(mp->ffplayer, opaque);
    MPTRACE("%s()=void\n", __func__);
    return prev_weak_thiz;
}

void ijkmp_set_option(IjkMediaPlayer *mp, int opt_category, const char *name, const char *value)
{
    if(NULL == mp) return;

    // MPTRACE("%s(%s, %s)\n", __func__, name, value);
    pthread_mutex_lock(&mp->mutex);
    ffp_set_option(mp->ffplayer, opt_category, name, value);
    pthread_mutex_unlock(&mp->mutex);
    // MPTRACE("%s()=void\n", __func__);
}

void ijkmp_set_option_int(IjkMediaPlayer *mp, int opt_category, const char *name, int64_t value)
{
    if(NULL == mp) return;

    // MPTRACE("%s(%s, %"PRId64")\n", __func__, name, value);
    pthread_mutex_lock(&mp->mutex);
    ffp_set_option_int(mp->ffplayer, opt_category, name, value);
    pthread_mutex_unlock(&mp->mutex);
    // MPTRACE("%s()=void\n", __func__);
}

int ijkmp_get_video_codec_info(IjkMediaPlayer *mp, char **codec_info)
{
    check_pointer(mp, -1);

    MPTRACE("%s\n", __func__);
    pthread_mutex_lock(&mp->mutex);
    int ret = ffp_get_video_codec_info(mp->ffplayer, codec_info);
    pthread_mutex_unlock(&mp->mutex);
    MPTRACE("%s()=void\n", __func__);
    return ret;
}

int ijkmp_get_audio_codec_info(IjkMediaPlayer *mp, char **codec_info)
{
    check_pointer(mp, -1);

    MPTRACE("%s\n", __func__);
    pthread_mutex_lock(&mp->mutex);
    int ret = ffp_get_audio_codec_info(mp->ffplayer, codec_info);
    pthread_mutex_unlock(&mp->mutex);
    MPTRACE("%s()=void\n", __func__);
    return ret;
}

void ijkmp_set_playback_rate(IjkMediaPlayer *mp, float rate)
{
    if(NULL == mp) return;

    MPTRACE("%s(%f)\n", __func__, rate);
    pthread_mutex_lock(&mp->mutex);
    ffp_set_playback_rate(mp->ffplayer, rate);
    pthread_mutex_unlock(&mp->mutex);
    MPTRACE("%s()=void\n", __func__);
}

void ijkmp_set_playback_volume(IjkMediaPlayer *mp, float volume)
{
    if(NULL == mp) return;

    MPTRACE("%s(%f)\n", __func__, volume);
    pthread_mutex_lock(&mp->mutex);
    ffp_set_playback_volume(mp->ffplayer, volume);
    pthread_mutex_unlock(&mp->mutex);
    MPTRACE("%s()=void\n", __func__);
}

int ijkmp_set_stream_selected(IjkMediaPlayer *mp, int stream, int selected)
{
    check_pointer(mp, -1);

    MPTRACE("%s(%d, %d)\n", __func__, stream, selected);
    pthread_mutex_lock(&mp->mutex);
    int ret = ffp_set_stream_selected(mp->ffplayer, stream, selected);
    pthread_mutex_unlock(&mp->mutex);
    MPTRACE("%s(%d, %d)=%d\n", __func__, stream, selected, ret);
    return ret;
}

float ijkmp_get_property_float(IjkMediaPlayer *mp, int id, float default_value)
{
    check_pointer(mp, -1);

    pthread_mutex_lock(&mp->mutex);
    float ret = ffp_get_property_float(mp->ffplayer, id, default_value);
    pthread_mutex_unlock(&mp->mutex);
    return ret;
}

void ijkmp_set_property_float(IjkMediaPlayer *mp, int id, float value)
{
    if(NULL == mp) return;

    pthread_mutex_lock(&mp->mutex);
    ffp_set_property_float(mp->ffplayer, id, value);
    pthread_mutex_unlock(&mp->mutex);
}

int64_t ijkmp_get_property_int64(IjkMediaPlayer *mp, int id, int64_t default_value)
{
    check_pointer(mp, -1);

    pthread_mutex_lock(&mp->mutex);
    int64_t ret = ffp_get_property_int64(mp->ffplayer, id, default_value);
    pthread_mutex_unlock(&mp->mutex);
    return ret;
}

void ijkmp_set_property_int64(IjkMediaPlayer *mp, int id, int64_t value)
{
    if(NULL == mp) return;

    pthread_mutex_lock(&mp->mutex);
    ffp_set_property_int64(mp->ffplayer, id, value);
    pthread_mutex_unlock(&mp->mutex);
}

IjkMediaMeta *ijkmp_get_meta_l(IjkMediaPlayer *mp)
{
    if(NULL == mp) return NULL;

    MPTRACE("%s\n", __func__);
    IjkMediaMeta *ret = ffp_get_meta_l(mp->ffplayer);
    MPTRACE("%s()=void\n", __func__);
    return ret;
}

void ijkmp_shutdown_l(IjkMediaPlayer *mp)
{
    if(NULL == g_pmutex)
    {
        g_pmutex = SDL_CreateMutex();
    }
    SDL_LockMutex(g_pmutex);
    MPTRACE("ijkmp_shutdown_l() begin, mp:%p\n", mp);
    if (mp->ffplayer) {
        ffp_stop_l(mp->ffplayer);
        ffp_wait_stop_l(mp->ffplayer);
    }
    SDL_UnlockMutex(g_pmutex);
    MPTRACE("ijkmp_shutdown_l() end\n");
}

void ijkmp_shutdown(IjkMediaPlayer *mp)
{
    ijkmp_stop(mp);
    return ijkmp_shutdown_l(mp);
}

void ijkmp_inc_ref(IjkMediaPlayer *mp)
{
    if(NULL == mp) return;
    __sync_fetch_and_add(&mp->ref_count, 1);
}

void ijkmp_dec_ref(IjkMediaPlayer *mp)
{
    if (!mp)
        return;

    int ref_count = __sync_sub_and_fetch(&mp->ref_count, 1);
    if (ref_count == 0) {
        MPTRACE("ijkmp_dec_ref(): ref=0\n");
        ijkmp_shutdown(mp);
        ijkmp_destroy_p(&mp);
    }
}

void ijkmp_dec_ref_p(IjkMediaPlayer **pmp)
{
    if (!pmp)
        return;

    ijkmp_dec_ref(*pmp);
    *pmp = NULL;
}

static int ijkmp_set_data_source_l(IjkMediaPlayer *mp, const char *url)
{
    check_pointer(mp, -1);
    //assert(url);
    mp->ffplayer->llstart_time = av_gettime();
    // MPST_RET_IF_EQ(mp->mp_state, MP_STATE_IDLE);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_INITIALIZED);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_ASYNC_PREPARING);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_PREPARED);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_STARTED);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_PAUSED);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_COMPLETED);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_STOPPED);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_ERROR);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_END);
#ifdef ENABLE_SDK_CONFIG_FILE
    if(NULL == url)
    {
        char rec_path[256];
        if(get_default_string_config("player_record_name", rec_path, 256) > 0)
        {
            mp->pmux_handle = ipc_record_muxer_open(get_format_by_log_path(rec_path), 4);
            //mp->prec_mux = vava_hs_create_mux_record(get_format_by_log_path(rec_path), 4, 1, av_codec_id_h264, av_codec_id_aac, 15, 16000, 0);
            //lbtrace("mp->prec_mux:%p = vava_hs_create_mux_record(path:%s, 4, 1, av_codec_id_h264, av_codec_id_aac, 15, 16000, 0)\n", mp->prec_mux, path);
        }
    }
    //lbtrace("mp->prec_mux:%p\n", mp->prec_mux);
#endif
    freep((void**)&mp->data_source);
    if(url)
    {
        mp->data_source = strdup(url);
        if (!mp->data_source)
            return EIJK_OUT_OF_MEMORY;
    }
    ijkmp_change_state_l(mp, MP_STATE_INITIALIZED);
    return 0;
}

int ijkmp_set_data_source(IjkMediaPlayer *mp, const char *url)
{
    check_pointer(mp, -1);
    const char* pplayurl = url;
    if(url && 0 == strcmp(url, "live"))
    {
        pplayurl = NULL;
        mp->live = 1;
        //mp->ffplayer->av_sync_type = AV_SYNC_EXTERNAL_CLOCK;
        mp->ffplayer->blive = 1;
        mp->ffplayer->packet_buffering = 0;
        //mp->ffplayer->packet_buffering = 0;
        //is->av_sync_type  = ffp->av_sync_type;
    }
    else
    {
        mp->live = 0;
        mp->ffplayer->blive = 0;
    }
    //assert(url);
    mp->ffplayer->llstart_time = av_gettime();
    MPTRACE("ijkmp_set_data_source(url=\"%s\")\n", url);
    pthread_mutex_lock(&mp->mutex);
    int retval = ijkmp_set_data_source_l(mp, pplayurl);
    pthread_mutex_unlock(&mp->mutex);
    MPTRACE("ijkmp_set_data_source(url=\"%s\")=%d\n", pplayurl, retval);
    return retval;
}

static int ijkmp_msg_loop(void *arg)
{
    IjkMediaPlayer *mp = arg;
    int ret = mp->msg_loop(arg);
    return ret;
}

static int ijkmp_prepare_async_l(IjkMediaPlayer *mp)
{
    check_pointer(mp, -1);

    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_IDLE);
    // MPST_RET_IF_EQ(mp->mp_state, MP_STATE_INITIALIZED);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_ASYNC_PREPARING);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_PREPARED);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_STARTED);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_PAUSED);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_COMPLETED);
    // MPST_RET_IF_EQ(mp->mp_state, MP_STATE_STOPPED);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_ERROR);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_END);

    //assert(mp->data_source);

    ijkmp_change_state_l(mp, MP_STATE_ASYNC_PREPARING);

    msg_queue_start(&mp->ffplayer->msg_queue);

    // released in msg_loop
    ijkmp_inc_ref(mp);
    mp->msg_thread = SDL_CreateThreadEx(&mp->_msg_thread, ijkmp_msg_loop, mp, "ff_msg_loop");
    // msg_thread is detached inside msg_loop
    // TODO: 9 release weak_thiz if pthread_create() failed;

    int retval = ffp_prepare_async_l(mp->ffplayer, mp->data_source);
    if (retval < 0) {
        ijkmp_change_state_l(mp, MP_STATE_ERROR);
        return retval;
    }

    return 0;
}

int ijkmp_prepare_async(IjkMediaPlayer *mp)
{
    check_pointer(mp, -1);
    MPTRACE("ijkmp_prepare_async()\n");
    pthread_mutex_lock(&mp->mutex);
    int retval = ijkmp_prepare_async_l(mp);
    pthread_mutex_unlock(&mp->mutex);
    MPTRACE("ijkmp_prepare_async()=%d\n", retval);
    return retval;
}

static int ikjmp_chkst_start_l(int mp_state)
{
    MPST_RET_IF_EQ(mp_state, MP_STATE_IDLE);
    MPST_RET_IF_EQ(mp_state, MP_STATE_INITIALIZED);
    MPST_RET_IF_EQ(mp_state, MP_STATE_ASYNC_PREPARING);
    // MPST_RET_IF_EQ(mp_state, MP_STATE_PREPARED);
    // MPST_RET_IF_EQ(mp_state, MP_STATE_STARTED);
    // MPST_RET_IF_EQ(mp_state, MP_STATE_PAUSED);
    // MPST_RET_IF_EQ(mp_state, MP_STATE_COMPLETED);
    MPST_RET_IF_EQ(mp_state, MP_STATE_STOPPED);
    MPST_RET_IF_EQ(mp_state, MP_STATE_ERROR);
    MPST_RET_IF_EQ(mp_state, MP_STATE_END);

    return 0;
}

static int ijkmp_start_l(IjkMediaPlayer *mp)
{
    check_pointer(mp, -1);

    MP_RET_IF_FAILED(ikjmp_chkst_start_l(mp->mp_state));

    ffp_remove_msg(mp->ffplayer, FFP_REQ_START);
    ffp_remove_msg(mp->ffplayer, FFP_REQ_PAUSE);
    ffp_notify_msg1(mp->ffplayer, FFP_REQ_START);

    return 0;
}

int ijkmp_start(IjkMediaPlayer *mp)
{
    check_pointer(mp, -1);
    MPTRACE("ijkmp_start()\n");
    pthread_mutex_lock(&mp->mutex);
    int retval = ijkmp_start_l(mp);
    pthread_mutex_unlock(&mp->mutex);
    MPTRACE("ijkmp_start()=%d\n", retval);
    return retval;
}

static int ikjmp_chkst_pause_l(int mp_state)
{
    MPST_RET_IF_EQ(mp_state, MP_STATE_IDLE);
    MPST_RET_IF_EQ(mp_state, MP_STATE_INITIALIZED);
    MPST_RET_IF_EQ(mp_state, MP_STATE_ASYNC_PREPARING);
    // MPST_RET_IF_EQ(mp_state, MP_STATE_PREPARED);
    // MPST_RET_IF_EQ(mp_state, MP_STATE_STARTED);
    // MPST_RET_IF_EQ(mp_state, MP_STATE_PAUSED);
    // MPST_RET_IF_EQ(mp_state, MP_STATE_COMPLETED);
    MPST_RET_IF_EQ(mp_state, MP_STATE_STOPPED);
    MPST_RET_IF_EQ(mp_state, MP_STATE_ERROR);
    MPST_RET_IF_EQ(mp_state, MP_STATE_END);

    return 0;
}

static int ijkmp_pause_l(IjkMediaPlayer *mp)
{
    check_pointer(mp, -1);

    MP_RET_IF_FAILED(ikjmp_chkst_pause_l(mp->mp_state));

    ffp_remove_msg(mp->ffplayer, FFP_REQ_START);
    ffp_remove_msg(mp->ffplayer, FFP_REQ_PAUSE);
    ffp_notify_msg1(mp->ffplayer, FFP_REQ_PAUSE);

    return 0;
}

int ijkmp_pause(IjkMediaPlayer *mp)
{
    check_pointer(mp, -1);
    MPTRACE("ijkmp_pause()\n");
    pthread_mutex_lock(&mp->mutex);
    int retval = ijkmp_pause_l(mp);
    pthread_mutex_unlock(&mp->mutex);
    MPTRACE("ijkmp_pause()=%d\n", retval);
    return retval;
}

static int ijkmp_stop_l(IjkMediaPlayer *mp)
{
    check_pointer(mp, -1);

    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_IDLE);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_INITIALIZED);
    // MPST_RET_IF_EQ(mp->mp_state, MP_STATE_ASYNC_PREPARING);
    // MPST_RET_IF_EQ(mp->mp_state, MP_STATE_PREPARED);
    // MPST_RET_IF_EQ(mp->mp_state, MP_STATE_STARTED);
    // MPST_RET_IF_EQ(mp->mp_state, MP_STATE_PAUSED);
    // MPST_RET_IF_EQ(mp->mp_state, MP_STATE_COMPLETED);
    // MPST_RET_IF_EQ(mp->mp_state, MP_STATE_STOPPED);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_ERROR);
    MPST_RET_IF_EQ(mp->mp_state, MP_STATE_END);

    ffp_remove_msg(mp->ffplayer, FFP_REQ_START);
    ffp_remove_msg(mp->ffplayer, FFP_REQ_PAUSE);
    int retval = ffp_stop_l(mp->ffplayer);
    //int retval = ffp_wait_stop_l(mp->ffplayer);
    if (retval < 0) {
        return retval;
    }
    ijkmp_change_state_l(mp, MP_STATE_STOPPED);
    
#ifdef ENABLE_SDK_CONFIG_FILE
    if(mp->pmux_handle)
    {
        ipc_record_muxer_close(&mp->pmux_handle);
    }
#endif
    //lbtrace("ijkmp_stop_l end\n");
    return 0;
}

int ijkmp_stop(IjkMediaPlayer *mp)
{
    check_pointer(mp, -1);
    int64_t begin = get_sys_time();
    sv_info("ijkmp_stop()\n");
    mp->stop_deliver = 1;
    ijkmp_stop_record(mp, 0);
    pthread_mutex_lock(&mp->mutex);
    int retval = ijkmp_stop_l(mp);
    pthread_mutex_unlock(&mp->mutex);
    MPTRACE("ijkmp_stop()=%d, spend time:%"PRId64"\n", retval, get_sys_time() - begin);
    return retval;
}

bool ijkmp_is_playing(IjkMediaPlayer *mp)
{
    if(NULL == mp) return false;
    if (mp->mp_state == MP_STATE_PREPARED ||
        mp->mp_state == MP_STATE_STARTED) {
        return true;
    }

    return false;
}

bool ijkmp_can_deliver(IjkMediaPlayer *mp)
{
    if(NULL == mp) return false;
    if (mp->mp_state == MP_STATE_PREPARED ||
        mp->mp_state == MP_STATE_STARTED || mp->mp_state == MP_STATE_ASYNC_PREPARING) {
        return true;
    }

    return false;
}

static int ikjmp_chkst_seek_l(int mp_state)
{
    MPST_RET_IF_EQ(mp_state, MP_STATE_IDLE);
    MPST_RET_IF_EQ(mp_state, MP_STATE_INITIALIZED);
    MPST_RET_IF_EQ(mp_state, MP_STATE_ASYNC_PREPARING);
    // MPST_RET_IF_EQ(mp_state, MP_STATE_PREPARED);
    // MPST_RET_IF_EQ(mp_state, MP_STATE_STARTED);
    // MPST_RET_IF_EQ(mp_state, MP_STATE_PAUSED);
    // MPST_RET_IF_EQ(mp_state, MP_STATE_COMPLETED);
    MPST_RET_IF_EQ(mp_state, MP_STATE_STOPPED);
    MPST_RET_IF_EQ(mp_state, MP_STATE_ERROR);
    MPST_RET_IF_EQ(mp_state, MP_STATE_END);

    return 0;
}

int ijkmp_seek_to_l(IjkMediaPlayer *mp, long msec)
{
    check_pointer(mp, -1);

    MP_RET_IF_FAILED(ikjmp_chkst_seek_l(mp->mp_state));

    mp->seek_req = 1;
    mp->seek_msec = msec;
    ffp_remove_msg(mp->ffplayer, FFP_REQ_SEEK);
    ffp_notify_msg2(mp->ffplayer, FFP_REQ_SEEK, (int)msec);
    // TODO: 9 64-bit long?

    return 0;
}

int ijkmp_seek_to(IjkMediaPlayer *mp, long msec)
{
    check_pointer(mp, -1);
    if(NULL == mp->data_source)
    {
        MPTRACE("living play, seeking not avaiable!\n");
        return -1;
    }
    MPTRACE("ijkmp_seek_to(%ld)\n", msec);
    pthread_mutex_lock(&mp->mutex);
    int retval = ijkmp_seek_to_l(mp, msec);
    pthread_mutex_unlock(&mp->mutex);
    MPTRACE("ijkmp_seek_to(%ld)=%d\n", msec, retval);

    return retval;
}

int ijkmp_get_state(IjkMediaPlayer *mp)
{
    return mp->mp_state;
}

static long ijkmp_get_current_position_l(IjkMediaPlayer *mp)
{
    if (mp->seek_req)
        return mp->seek_msec;
    return ffp_get_current_position_l(mp->ffplayer);
}

long ijkmp_get_current_position(IjkMediaPlayer *mp)
{
    check_pointer(mp, -1);
    pthread_mutex_lock(&mp->mutex);
    long retval;
    if (mp->seek_req)
        retval = mp->seek_msec;
    else
        retval = ijkmp_get_current_position_l(mp);
    pthread_mutex_unlock(&mp->mutex);
    sv_trace("get current pos:%lfms", (double)retval/1000);
    return retval;
}

long ijkmp_get_current_video_timestamp(IjkMediaPlayer *mp)
{
    long pos = -1;
    check_pointer(mp, -1);
    pthread_mutex_lock(&mp->mutex);
    if(mp->ffplayer && mp->ffplayer->is && mp->ffplayer->is->pcur_frame)
    {
        AVFrame* pframe = mp->ffplayer->is->pcur_frame;
        pos = pframe->pkt_pos/1000;
    }
    pthread_mutex_unlock(&mp->mutex);
    lbdebug("current video pts:%ld", pos);
    return pos;
}

static long ijkmp_get_duration_l(IjkMediaPlayer *mp)
{
    return ffp_get_duration_l(mp->ffplayer);
}

long ijkmp_get_duration(IjkMediaPlayer *mp)
{
    check_pointer(mp, -1);
    pthread_mutex_lock(&mp->mutex);
    long retval = ijkmp_get_duration_l(mp);
    pthread_mutex_unlock(&mp->mutex);
    return retval;
}

static long ijkmp_get_playable_duration_l(IjkMediaPlayer *mp)
{
    return ffp_get_playable_duration_l(mp->ffplayer);
}

long ijkmp_get_playable_duration(IjkMediaPlayer *mp)
{
    check_pointer(mp, -1);
    pthread_mutex_lock(&mp->mutex);
    long retval = ijkmp_get_playable_duration_l(mp);
    pthread_mutex_unlock(&mp->mutex);
    return retval;
}

void ijkmp_set_loop(IjkMediaPlayer *mp, int loop)
{
    if(NULL == mp) return;
    pthread_mutex_lock(&mp->mutex);
    ffp_set_loop(mp->ffplayer, loop);
    pthread_mutex_unlock(&mp->mutex);
}

int ijkmp_get_loop(IjkMediaPlayer *mp)
{
    check_pointer(mp, -1);
    pthread_mutex_lock(&mp->mutex);
    int loop = ffp_get_loop(mp->ffplayer);
    pthread_mutex_unlock(&mp->mutex);
    return loop;
}

void *ijkmp_get_weak_thiz(IjkMediaPlayer *mp)
{
    return mp->weak_thiz;
}

void *ijkmp_set_weak_thiz(IjkMediaPlayer *mp, void *weak_thiz)
{
    void *prev_weak_thiz = mp->weak_thiz;

    mp->weak_thiz = weak_thiz;

    return prev_weak_thiz;
}

/* need to call msg_free_res for freeing the resouce obtained in msg */
int ijkmp_get_msg(IjkMediaPlayer *mp, AVMessage *msg, int block)
{
    check_pointer(mp, -1);
    while (1) {
        int continue_wait_next_msg = 0;
        int retval = msg_queue_get(&mp->ffplayer->msg_queue, msg, block);
        if (retval <= 0)
            return retval;

        switch (msg->what) {
        case FFP_MSG_PREPARED:
            MPTRACE("ijkmp_get_msg: FFP_MSG_PREPARED\n");
            pthread_mutex_lock(&mp->mutex);
            if (mp->mp_state == MP_STATE_ASYNC_PREPARING) {
                ijkmp_change_state_l(mp, MP_STATE_PREPARED);
            } else {
                // FIXME: 1: onError() ?
                av_log(mp->ffplayer, AV_LOG_DEBUG, "FFP_MSG_PREPARED: expecting mp_state==MP_STATE_ASYNC_PREPARING\n");
            }
            if (!mp->ffplayer->start_on_prepared) {
                ijkmp_change_state_l(mp, MP_STATE_PAUSED);
            }
            pthread_mutex_unlock(&mp->mutex);
            break;

        case FFP_MSG_COMPLETED:
            MPTRACE("ijkmp_get_msg: FFP_MSG_COMPLETED\n");

            pthread_mutex_lock(&mp->mutex);
            mp->restart = 1;
            mp->restart_from_beginning = 1;
            ijkmp_change_state_l(mp, MP_STATE_COMPLETED);
            pthread_mutex_unlock(&mp->mutex);
            break;

        case FFP_MSG_SEEK_COMPLETE:
            MPTRACE("ijkmp_get_msg: FFP_MSG_SEEK_COMPLETE\n");

            pthread_mutex_lock(&mp->mutex);
            mp->seek_req = 0;
            mp->seek_msec = 0;
            pthread_mutex_unlock(&mp->mutex);
            break;

        case FFP_REQ_START:
            MPTRACE("ijkmp_get_msg: FFP_REQ_START\n");
            continue_wait_next_msg = 1;
            pthread_mutex_lock(&mp->mutex);
            if (0 == ikjmp_chkst_start_l(mp->mp_state)) {
                // FIXME: 8 check seekable
                if (mp->restart) {
                    if (mp->restart_from_beginning) {
                        av_log(mp->ffplayer, AV_LOG_DEBUG, "ijkmp_get_msg: FFP_REQ_START: restart from beginning\n");
                        retval = ffp_start_from_l(mp->ffplayer, 0);
                        if (retval == 0)
                            ijkmp_change_state_l(mp, MP_STATE_STARTED);
                    } else {
                        av_log(mp->ffplayer, AV_LOG_DEBUG, "ijkmp_get_msg: FFP_REQ_START: restart from seek pos\n");
                        retval = ffp_start_l(mp->ffplayer);
                        if (retval == 0)
                            ijkmp_change_state_l(mp, MP_STATE_STARTED);
                    }
                    mp->restart = 0;
                    mp->restart_from_beginning = 0;
                } else {
                    av_log(mp->ffplayer, AV_LOG_DEBUG, "ijkmp_get_msg: FFP_REQ_START: start on fly\n");
                    retval = ffp_start_l(mp->ffplayer);
                    if (retval == 0)
                        ijkmp_change_state_l(mp, MP_STATE_STARTED);
                }
            }
            pthread_mutex_unlock(&mp->mutex);
            break;

        case FFP_REQ_PAUSE:
            MPTRACE("ijkmp_get_msg: FFP_REQ_PAUSE\n");
            continue_wait_next_msg = 1;
            pthread_mutex_lock(&mp->mutex);
            if (0 == ikjmp_chkst_pause_l(mp->mp_state)) {
                int pause_ret = ffp_pause_l(mp->ffplayer);
                if (pause_ret == 0)
                    ijkmp_change_state_l(mp, MP_STATE_PAUSED);
            }
            pthread_mutex_unlock(&mp->mutex);
            break;

        case FFP_REQ_SEEK:
            MPTRACE("ijkmp_get_msg: FFP_REQ_SEEK\n");
            continue_wait_next_msg = 1;

            pthread_mutex_lock(&mp->mutex);
            if (0 == ikjmp_chkst_seek_l(mp->mp_state)) {
                mp->restart_from_beginning = 0;
                if (0 == ffp_seek_to_l(mp->ffplayer, msg->arg1)) {
                    av_log(mp->ffplayer, AV_LOG_DEBUG, "ijkmp_get_msg: FFP_REQ_SEEK: seek to %d\n", (int)msg->arg1);
                }
            }
            pthread_mutex_unlock(&mp->mutex);
            break;
        }
        if (continue_wait_next_msg) {
            msg_free_res(msg);
            continue;
        }

        return retval;
    }

    return -1;
}

int  ijkmp_set_playback_mute(IjkMediaPlayer *mp, int muted)
{
    sv_trace("ffp_set_playback_mute(mp:%p, muted:%d)\n", mp, muted);
    if(!mp)
    {
        return -1;
    }
    
    return ffp_set_playback_mute(mp->ffplayer, muted);
}

int  ijkmp_get_playback_mute(IjkMediaPlayer *mp)
{
    if(!mp)
    {
        return -1;
    }
    
    return ffp_get_playback_mute(mp->ffplayer);
}

int ijkmp_get_sdk_version(char* pversion, int len)
{
    
    char ver[256] = {0};
    sprintf(ver, "%d.%d.%d.%d", IJK_PLAYER_VERSION_MAYJOR, IJK_PLAYER_VERSION_MINOR, IJK_PLAYER_VERSION_MICRO, IJK_PLAYER_VERSION_TINY);
    if(NULL == pversion || len <= strlen(ver))
    {
        return 0;
    }
    strcpy(pversion, ver);
    return strlen(ver);
}

/*int ijkmp_deliver_packet(IjkMediaPlayer *mp, int mediatype, char* pdata, int data_len, int64_t pts)
{
    sv_info("mt:%d, pdata:%p, data_len:%d, pts_in_ms:%"PRId64"", mediatype, pdata, data_len, pts);
    check_pointer(mp, -1);

    pthread_mutex_lock(&mp->mutex);
    int ret = ffp_deliver_packet(mp->ffplayer, mediatype, pdata, data_len, pts);
    if(mp->mux_ctx && avmuxer_is_running(mp->mux_ctx))
    {
        ret = avmuxer_deliver_packet(mp->mux_ctx, mediatype, pdata, data_len, pts);
        //lbtrace("ret:%d = avmuxer_deliver_packet(mp->mux_ctx:%p, mediatype:%d, pdata:%p, data_len:%d, pts:%ld)\n", ret, mp->mux_ctx, mediatype, pdata, data_len, pts);
    }

    pthread_mutex_unlock(&mp->mutex);
    //sv_trace("ijkmp_deliver_packet()=%d\n", ret);
    return ret;
}*/
#define AVRECORD_DELIVER_PACKET
int record_deliver_packet(IjkMediaPlayer *mp, int codec_id, char* pdata, int data_len, int64_t pts, long framenum)
{
    int ret = 0;
    MPTRACE("%s(mp:%p, codec_id:%d, pdata:%p,  data_len:%d, pts:%" PRId64 ")\n", __func__, mp, codec_id, pdata,  data_len, pts);
#ifdef AVRECORD_DELIVER_PACKET
    if(mp->prec_ctx && avrecord_is_running(mp->prec_ctx))
    {
        ret = avrecord_deliver_packet(mp->prec_ctx, codec_id, pdata, data_len, pts, framenum);
    }
#else
    if(mp->mux_ctx && avmuxer_is_running(mp->mux_ctx))
    {
        ret = avmuxer_deliver_packet(mp->mux_ctx, codec_id, pdata, data_len, pts);
    }
#endif
    
    return ret;
}

int record_start(IjkMediaPlayer *mp, const char* purl, const char* ptmp_url)
{
    int ret = -1;
#ifdef AVRECORD_DELIVER_PACKET
    if(NULL == mp->prec_ctx)
    {
        mp->prec_ctx = avrecord_open_context(NULL, purl, "mov", ptmp_url);
        if(NULL == mp->prec_ctx)
        {
            lberror("mp->prec_ctx:%p = avrecord_open_context(NULL, purl:%s, mov, ptmp_url:%s) failed\n", mp->prec_ctx, purl, ptmp_url);
            return ret;
        }
        avrecord_set_callback(mp->prec_ctx, ffp_muxer_msg_notify, mp->ffplayer);
        ret = avrecord_start(mp->prec_ctx, 0);
        //ret = avrecord_deliver_packet(mp->prec_ctx, codec_id, pdata, data_len, pts);
    }
#else
    do{
        if(mp->mux_ctx && avmuxer_is_running(mp->mux_ctx))
        {
            MPTRACE("ijk record allready start\n", mp);
            ret = 1;
            break;
        }
        mp->mux_ctx = avmuxer_open_context();
        //MPTRACE("mp->mux_ctx:%p = avmuxer_open_context()\n", mp->mux_ctx);
        ret = avmuxer_set_sink(mp->mux_ctx, purl, ptmp_url, "mov");
        //lbtrace("ret:%d = avmuxer_set_sink(mp->mux_ctx, purl, NULL)\n", ret);
        if(ret < 0)
        {
            MPTRACE("avmuxer_set_sink failed, ret:%d\n", ret);
            break;
        }
        avmuxer_set_callback(mp->mux_ctx, ffp_muxer_msg_notify, mp->ffplayer);
        ret = avmuxer_start(mp->mux_ctx);
        if(ret < 0)
        {
            MPTRACE("avmuxer_start failed, ret:%d\n", ret);
            //return -1;
            break;
        }
    }while(0);
#endif
    return ret;
}

void record_stop(IjkMediaPlayer *mp, int bcancel)
{
    
#ifdef AVRECORD_DELIVER_PACKET
    if(mp->prec_ctx)
    {
        lbtrace("avrecord_stop begin\n");
        avrecord_stop(mp->prec_ctx, bcancel);
        lbtrace("avrecord_stop(mp->prec_ctx:%p, bcancel:%d)\n", mp->prec_ctx, bcancel);
        avrecord_close_contextp(&mp->prec_ctx);
        lbtrace("avrecord_close_contextp(&mp->prec_ctx:%p)\n", mp->prec_ctx);
    }
#else
    if(mp->mux_ctx)
    {
        lbtrace("avmuxer_stop begin\n");
        avmuxer_stop(mp->mux_ctx, bcancel);
        lbtrace("avmuxer_stop(mp->mux_ctx:%p, bcancel:%d)\n", mp->mux_ctx, bcancel);
        avmuxer_close_contextp(&mp->mux_ctx);
        lbtrace("avmuxer_close_contextp(&mp->mux_ctx:%p)\n", mp->mux_ctx);
    }
#endif
}

int ijkmp_deliver_packet(IjkMediaPlayer *mp, int codec_id, char* pdata, int data_len, int64_t pts, long framenum)
{
    sv_info("codec_id:%d, pdata:%p, data_len:%d, pts_in_ms:%"PRId64", framenum:%d\n", codec_id, pdata, data_len, pts, framenum);
    check_pointer(mp, -1);
    if(!ijkmp_can_deliver(mp))
    {
        sv_trace("player is not in playing state, frame (codec_id:%d, pdata:%p, data_len:%d, pts_in_ms:%"PRId64", framenum:%d) will be drop\n", codec_id, pdata, data_len, pts, framenum);
        return -1;
    }
#ifdef ENABLE_SDK_CONFIG_FILE
    if(mp->pmux_handle && pdata)
    {
        //int codec_id = mediatype == 0 ? av_codec_id_h264 : av_codec_id_aac;
        int key_flag = 1;
        int ipc_codec_id = -1;
        if(codec_id < 10000)
        {
            key_flag = is_idr_frame(AV_CODEC_ID_H264 == codec_id ? 4 : 5, pdata, data_len);
            if(key_flag)
            {
                //sv_trace("sps and pps:\n");
                sv_memory(3, pdata, 32, "sps and pps:\n");
            }
            if(AV_CODEC_ID_H264 == codec_id)
            {
                ipc_codec_id = eipc_codec_id_h264;
            }
            else
            {
                ipc_codec_id = eipc_codec_id_h265;
            }
        }
        else
        {
            ipc_codec_id = eipc_codec_id_aac;
        }
        ipc_record_muxer_write_packet(mp->pmux_handle, ipc_codec_id, pdata, data_len, pts/1000, key_flag);
        //MPTRACE("vava_hs_write_mux_record(mp->prec_mux:%p, codec_id:%d, pdata:%p, data_len:%d, key_flag:%d, pts:%" PRId64 ")\n", mp->prec_mux, codec_id, pdata, data_len, key_flag, pts);
    }
#endif
    pthread_mutex_lock(&mp->mutex);
    int ret = ffp_deliver_packet(mp->ffplayer, codec_id, pdata, data_len, pts, framenum);
    ret = record_deliver_packet(mp, codec_id, pdata, data_len, pts, framenum);
    /*if(mp->mux_ctx && avmuxer_is_running(mp->mux_ctx))
    {
        ret = avmuxer_deliver_packet(mp->mux_ctx, codec_id, pdata, data_len, pts);
        //lbtrace("ret:%d = avmuxer_deliver_packet(mp->mux_ctx:%p, mediatype:%d, pdata:%p, data_len:%d, pts:%ld)\n", ret, mp->mux_ctx, mediatype, pdata, data_len, pts);
    }*/

    pthread_mutex_unlock(&mp->mutex);
    //sv_trace("ijkmp_deliver_packet()=%d\n", ret);
    return ret;
}

int ijkmp_get_image_jpg(IjkMediaPlayer *mp, char* pdata, int data_len, int64_t* ppts)
{
    check_pointer(mp, -1);
    sv_trace("ijkmp_get_image_jpg()\n");
    pthread_mutex_lock(&mp->mutex);
    int ret = ffp_get_image_jpg(mp->ffplayer, pdata, data_len, ppts);
    pthread_mutex_unlock(&mp->mutex);
    sv_trace("ijkmp_get_image_jpg()=%d\n", ret);
    return ret;
}

int ijkmp_save_image_jpg(IjkMediaPlayer *mp, const char* ppath)
{
    int data_len = 1024*1024;
    char* pdata = malloc(data_len);
    int ret = -1;
    data_len = ijkmp_get_image_jpg(mp, pdata, data_len, NULL);
    if(data_len > 0)
    {
        FILE* pfile = fopen(ppath, "wb");
        if(pfile)
        {
            fwrite(pdata, 1, data_len, pfile);
            fclose(pfile);
        }
        ret = 0;
    }
    else
    {
        MPTRACE("ijkmp_save_image_jpg() failed ret:%d\n", ret);
    }
    free(pdata);
    pdata = NULL;
    return ret;
}

int  ijkmp_start_record(IjkMediaPlayer *mp, const char* purl, const char* ptmp_url)
{
    if(!mp || !mp->ffplayer || !mp->ffplayer->aout)
    {
        MPTRACE("Invalid parameter mp:%p\n", mp);
        return -1;
    }
    int ret = -1;
    double adelay = SDL_AoutGetLatencySeconds(mp->ffplayer->aout);
    int min_buf_size = SDL_AoutGetMinBufferSize(mp->ffplayer->aout);
    sv_trace("ijkmp_start_record(purl:%s, ptmp_url:%s) begin\nadelay:%lf, min_buf_size:%d, mp->ffplayer->aout:%p\n", purl, ptmp_url, adelay, min_buf_size, mp->ffplayer->aout);
    pthread_mutex_lock(&mp->mutex);
    ret = record_start(mp, purl, ptmp_url);
    sv_trace("ret:%d = record_start(mp:%p, purl:%s, ptmp_url:%s)\n", ret, mp, purl, ptmp_url);
    /*do{
        if(mp->mux_ctx && avmuxer_is_running(mp->mux_ctx))
        {
            MPTRACE("ijk record allready start\n", mp);
            ret = 1;
            break;
        }
        mp->mux_ctx = avmuxer_open_context();
        //MPTRACE("mp->mux_ctx:%p = avmuxer_open_context()\n", mp->mux_ctx);
        ret = avmuxer_set_sink(mp->mux_ctx, purl, ptmp_url, "mov");
        //lbtrace("ret:%d = avmuxer_set_sink(mp->mux_ctx, purl, NULL)\n", ret);
        if(ret < 0)
        {
            MPTRACE("avmuxer_set_sink failed, ret:%d\n", ret);
            break;
        }
        avmuxer_set_callback(mp->mux_ctx, ffp_muxer_msg_notify, mp->ffplayer);
        ret = avmuxer_start(mp->mux_ctx);
        if(ret < 0)
        {
            MPTRACE("avmuxer_start failed, ret:%d\n", ret);
            //return -1;
            break;
        }
    }while(0);*/
    pthread_mutex_unlock(&mp->mutex);
    sv_info("ijkmp_start_record end, mp->mux_ctx:%p\n", mp->mux_ctx);
    return ret;
}

void ijkmp_stop_record(IjkMediaPlayer *mp, int bcancel)
{
    sv_info("ijkmp_stop_record begin\n");
    pthread_mutex_lock(&mp->mutex);
    //MPTRACE("after pthread_mutex_lock\n");
    
    record_stop(mp, bcancel);
    //MPTRACE("before pthread_mutex_unlock\n");
    pthread_mutex_unlock(&mp->mutex);
    MPTRACE("ijkmp_stop_record bcancel:%d\n", bcancel);
}
bool ijkmp_is_recording(IjkMediaPlayer *mp)
{
    if(!mp || !mp->mux_ctx)
    {
        MPTRACE("Invalid parameter mp:%p, mp->mux_ctx:%p\n", mp, mp ? mp->mux_ctx : NULL);
        return false;
    }
    
    return (bool)avmuxer_is_running(mp->mux_ctx);
}

int ijkmp_start_echo_cancel(IjkMediaPlayer *mp, int channel, int samplerate, int sampleformat, int nbsamples, int usdelay)
{
    if(!mp || !mp->ffplayer)
    {
        MPTRACE("Invalid parameter mp:%p, mp->ffplayer:%p\n", mp, mp ? mp->ffplayer : NULL);
        return -1;
    }
    int ret = -1;
    double delay = SDL_AoutGetLatencySeconds(mp->ffplayer);
    if(usdelay <= 0)
    {
        usdelay = ijkmp_get_estimate_delay(mp);
    }
    MPTRACE("ijkmp_start_echo_cancel(mp:%p, channel:%d, samplerate:%d, sampleformat:%d, nbsamples:%d, usdelay:%d, delay:%lf)\n", mp, channel, samplerate, sampleformat, nbsamples, usdelay, delay);
    mp->papc = lbaudio_proc_open_context(channel, samplerate, sampleformat, nbsamples);
    MPTRACE("mp->papc:%p = lbaudio_proc_open_context, lbfar_audio_callback:%p\n", mp->papc, lbfar_audio_callback);
    //ffp_set_play_audio_callback(mp->ffplayer, NULL, NULL);
    ffp_set_play_audio_callback(mp->ffplayer, mp->papc, lbfar_audio_callback);
    MPTRACE("after ffp_set_play_audio_callback");
    laddd_aec_filterex(mp->papc, usdelay/1000);
    //get_log_path();
    int enable_nr = 0;
    int enable_agc = 0;
    const char* pnr_log_path = NULL;
    const char* pagc_log_path = NULL;
    if(g_pconf_ctx)
    {
        const char* penable_nr = get_conf_value_by_tag(g_pconf_ctx, "enable_nr");
        if(0 == strcmp(penable_nr, "on") || 0 == strcmp(penable_nr, "ON"))
        {
            enable_nr = 1;
        }

        const char* penable_agc = get_conf_value_by_tag(g_pconf_ctx, "enable_agc");
        if(0 == strcmp(penable_agc, "on") || 0 == strcmp(penable_agc, "ON"))
        {
            enable_agc = 1;
        }

        pnr_log_path = get_conf_value_by_tag(g_pconf_ctx, "log_nr_output_audio");
        pagc_log_path = get_conf_value_by_tag(g_pconf_ctx, "log_agc_output_audio");
        MPTRACE("load_config from ini, penable_nr:%s, penable_agc:%s", penable_nr, penable_agc);


    }
    MPTRACE("enable_nr:%d, pnr_log_path:%s, enable_agc:%d, pagc_log_path:%s", enable_nr, pnr_log_path, enable_agc, pagc_log_path);
    if(enable_nr)
    {
        char nr_log_path[256];
        if(pnr_log_path)
        {
            sprintf(nr_log_path, "%s/%s", get_log_path(), pnr_log_path);
            pnr_log_path = nr_log_path;
        }
        ret = lbadd_noise_reduce_filter(mp->papc, 1, pnr_log_path);
        MPTRACE("ret:%d = lbadd_noise_reduce_filter(mp->papc:%p, 1, pnr_log_path:%s)\n", ret, mp->papc, pnr_log_path);
    }
    
    if(enable_agc)
    {
        char agc_log_path[256];
        if(pagc_log_path)
        {
            sprintf(agc_log_path, "%s/%s", get_log_path(), pagc_log_path);
            pagc_log_path = agc_log_path;
        }
        ret = lbadd_agc_filter(mp->papc, 3, pagc_log_path);
        MPTRACE("ret:%d = lbadd_agc_filter(mp->papc:%p, 3, pnr_log_path:%s)\n", ret, mp->papc, pagc_log_path);
    }

    MPTRACE("ijkmp_start_echo_cancel end\n");
    return 0;
}

#define MIN_BUFFER_SIZE 3072
int ijkmp_add_far_buffer(IjkMediaPlayer *mp, char* pdata, int len, int64_t timestamp, int channel, int samplerate, int sampleformat)
{
    if(!mp || !mp->papc)
    {
        MPTRACE("Invalid parameter mp:%p, mp->papc:%p\n", mp, mp ? mp->papc : NULL);
        return -1;
    }
    
    int ret = lbfar_audio_callback(mp->papc, pdata, len, timestamp, channel, samplerate, sampleformat);
    lbtrace("ret:%d = ijkmp_add_far_buffer(mp:%p, pdata:%p, len:%d, timestamp:%" PRId64 ", channel:%d, samplerate:%d, sampleformat:%d)\n", ret, mp, pdata, len, timestamp, channel, samplerate, sampleformat);
    return ret;
}

int ijkmp_get_estimate_delay(IjkMediaPlayer *mp)
{
    FFPlayer* ffp = mp ? mp->ffplayer:NULL;
    SDL_Aout* paout = ffp ? ffp->aout : NULL;
    if(NULL == mp || !ffp || !ffp->aout)
    {
        lbtrace("mp:%p, ffp:%p, paout:%p", mp, ffp,  paout);
        return -1;
    }
    
    int min_buf_size = SDL_AoutGetMinBufferSize(paout);
    /*int adelay = SDL_AoutGetLatencySeconds(paout)*1000;
    int mic_buf_size = 1280;
    int extra_buf_size = min_buf_size/2;//min_buf_size > MIN_BUFFER_SIZE ? (min_buf_size - MIN_BUFFER_SIZE)  : 0;
    int delay_ms = (min_buf_size + mic_buf_size + extra_buf_size)*1000 / (ffp->is->audio_tgt.channels * ffp->is->audio_tgt.freq * av_get_bytes_per_sample(ffp->is->audio_tgt.fmt));
    int nfix_delay = 50;
    if(min_buf_size > 5*1024)
    {
        nfix_delay = 150;
    }
    int sum_delay = min_buf_size * 2 * 1000 / (ffp->is->audio_tgt.channels * ffp->is->audio_tgt.freq * av_get_bytes_per_sample(ffp->is->audio_tgt.fmt)) + 15;//1.132 * min_buf_size/32 + 156.6 + 10;//adelay + delay_ms + nfix_delay;
    lbtrace("min_buf_size:%d, extra_buf_size:%d, audio delay:%d, mic_buf_size:%d, delay_ms:%d, sum_delay:%d, adelay:%d, nfix_delay:%d\n", min_buf_size, extra_buf_size, adelay, mic_buf_size, delay_ms, sum_delay, adelay, nfix_delay);
    */
    int sum_delay = (min_buf_size * 3 / 2 ) * 1000 / (ffp->is->audio_tgt.channels * ffp->is->audio_tgt.freq * av_get_bytes_per_sample(ffp->is->audio_tgt.fmt)) + 10;
    return sum_delay*1000;
}

int ijkmp_echo_cancel(IjkMediaPlayer *mp, char* pdata, char* pout, int len)
{
    if(!mp)
    {
        MPTRACE("Invalid parameter mp:%p\n", mp);
        return -1;
    }
    int64_t ts = lbget_system_timestamp_in_us();
    int ret = lbaudio_proc_process(mp->papc, pdata, pout, len, ts);
    MPTRACE("ret:%d = lbaudio_proc_process(mp->papc:%p, pdata:%p, pout:%p, len:%d, ts:%" PRId64 ")\n", ret, mp->papc, pdata, pout, len, ts);
    return 0;
}

int ijkmp_echo_cancel_deliver_data(IjkMediaPlayer *mp, char* pdata, char* pfar, char* pout, int len)
{
    if(!mp)
    {
        MPTRACE("Invalid parameter mp:%p\n", mp);
        return -1;
    }
    
    return lbaec_proc(mp->papc, pdata, pfar, pout, len);
}

int ijkmp_stop_echo_cancel(IjkMediaPlayer *mp)
{
    lbtrace("ijkmp_stop_echo_cancel(mp:%p)\n", mp);
    if(!mp || !mp->ffplayer)
    {
        MPTRACE("Invalid parameter mp:%p, mp->ffplayer:%p\n", mp, mp->ffplayer);
        return -1;
    }
    lbtrace("before ffp_set_play_audio_callback\n");
    ffp_set_play_audio_callback(mp->ffplayer, NULL, NULL);
    lbtrace("after ffp_set_play_audio_callback\n");
    //SDL_UnlockMutex(mp->ffplayer->af_mutex);
    lbaudio_proc_close_contextp(&mp->papc);
    lbtrace("ijkmp_stop_echo_cancel(mp:%p) end\n", mp);
    return 0;
}

enum codec_id
{
    codec_id_none = -1,
    codec_id_h264 = 0,
    codec_id_h265 = 1,
    codec_id_mp3  = 2,
    codec_id_aac  = 8
};

typedef struct _rechead{
  unsigned int tag;                // sync 0xEB0000AA
  unsigned int size;            // frame size
  unsigned int type;            // frame type: 0, P frame, 1: I frame, 8: audio frame
  unsigned int fps;                // frame rate
  unsigned int time_sec;          // timestamp in second
  unsigned int time_usec;      // timestamp in microsecond
}VAVA_RecHead;

//video record head
typedef struct _recinfo{
  char tag;                    // 0:create record, 1.complete record
  char v_encode;              // video codec, h264 0
  char a_encode;              // audio codec, aac 3
  char res;                    // video resolution
  char fps;                    //֡ video frame rate
  char encrypt;              // encrypt mode: 0, no encrypt, 1.aes encrypt
  unsigned short vframe;     // video frame count
  int size;                    // video record size
  int time;                    // video record duration
}VAVA_RecInfo;

typedef struct record_demux
{
    FILE* pfile;
    VAVA_RecInfo recinfo;
    struct _rechead record_header;
    int has_read_header;
    long vframe_num;
    long aframe_num;
} VAVA_record_demux;
#ifdef ENABLE_RECORD_DEMUX
#include "rec_demux.h"
void* record_open(char* purl)
{
    VAVA_HS_REC_CTX* prec_demux = vava_hs_open_record(purl);
    return prec_demux;
}
int read_frame(void* powner, int* pmt, char* pdata, int* psize, int64_t* ppts, long* pframe_num)
{
    VAVA_HS_REC_CTX* prec_demux = (VAVA_HS_REC_CTX*)powner;
    IPC_PACKET_HDR pkthdr;
    int ret = vava_hs_read_packet(prec_demux, &pkthdr, pdata, *psize);
    if(ret < 0)
    {
        lberror("read record packet failed, ret:%d\n", ret);
        return ret;
    }
    if(pmt)
    {
        *pmt = pkthdr.codec_id;
    }
    if(pframe_num)
    {
        *pframe_num = pkthdr.frame_num;
    }

    if(psize)
    {
        *psize = pkthdr.size;
    }

    if(ppts)
    {
        *ppts = pkthdr.pts;
    }
    return pkthdr.size;
}

void record_close(void* powner)
{
    vava_hs_close_record((VAVA_HS_REC_CTX**)&powner);
}
#else
void* record_open(char* purl)
{
    struct record_demux* precord_demux = (void*)malloc(sizeof(struct record_demux));
    memset(precord_demux, 0, sizeof(struct record_demux));
    precord_demux->pfile = fopen(purl, "rb");
    fread(&precord_demux->recinfo, 1, sizeof(precord_demux->recinfo), precord_demux->pfile);

    return precord_demux;
}
int read_frame(void* powner, int* pmt, char* pdata, int* psize, int64_t* ppts)
{
    if(NULL == powner  || NULL == psize)
    {
        return -1;
    }
    int ret = 0;
    struct record_demux* precord_demux = (struct record_demux*)powner;
    if(!precord_demux)
    {
        return -1;
    }
    if(!precord_demux->has_read_header)
    {

        ret = (int)fread(&precord_demux->record_header, 1, sizeof(precord_demux->record_header), precord_demux->pfile);
        if(ret <= 0)
        {
            //assert(0);
            return -1;
        }
        
    }
    if(pdata || *psize < precord_demux->record_header.size)
    {
        ret = (int)fread(pdata, 1, precord_demux->record_header.size, precord_demux->pfile);
    }
    
    *psize = precord_demux->record_header.size;
    
    if(pmt)
    {
        *pmt = precord_demux->record_header.type == 8 ? 1 : 0;
    }
    
    if(ppts)
    {
        *ppts = (int64_t)precord_demux->record_header.time_sec * 1000 + (int64_t)precord_demux->record_header.time_usec;
    }
    
    return ret;
}

void record_close(void* powner)
{
    struct record_demux* precord_demux = (struct record_demux*)powner;
    if(precord_demux && precord_demux->pfile)
    {
        fclose(precord_demux->pfile);
    }
    free(precord_demux);
}
#endif

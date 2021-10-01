/*
 * ffmpeg_api_jni.c
 *
 * Copyright (c) 2014 Bilibili
 * Copyright (c) 2014 Zhang Rui <bbcallen@gmail.com>
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

#include "record_api_jni.h"
#include "common_tools_jni.h"
#include <assert.h>
#include <string.h>
#include <jni.h>
#include "../ff_ffinc.h"
#include "ijksdl/ijksdl_log.h"
#include "ijksdl/android/ijksdl_android_jni.h"
#include "j4a/class/tv/danmaku/ijk/media/player/IjkMediaPlayer.h"
//#include "ijkMediaPlayer.h"
//#include "j4a_base.h"
//tv/danmaku/ijk/media/player/IjkMediaPlayer
#define JNI_CLASS_RECORD_API "tv/danmaku/ijk/media/recorder/VavaRecordDemux"

#define JAVA_FUNC_NAME "nativeEventNotify"
#define JAVA_FUNC_PARAM "(III)V"

extern JavaVM* g_jvm;
typedef struct record_api_fields_t {
    jclass clazz;
} record_api_fields_t;
static record_api_fields_t g_record_clazz;
static JNIEnv* g_penv = NULL;
#define JAVA_NATIVE_MEDIARECORD_FIELD_NAME		"mNativeMediaRecord"

void Record_demux
void LoadJavaEventNotify(JavaVM* vm)
{
	if(vm == NULL)
	{
		assert(vm);
		return -1;
	}

	JNIEnv* env = NULL;
	if((*vm)->GetEnv(vm, (void**)&env,JNI_VERSION_1_4)!=JNI_OK){
		lberror("(*vm)->GetEnv(vm, (void**)&env,JNI_VERSION_1_4)!=JNI_OK failed\n");
        return JNI_ERR;
    }

	g_VavaRecord_class_id = (*env)->FindClass(env, JNI_CLASS_RECORD_API);
	lbdebug("g_VavaRecord_class_id:%p = (*env)->FindClass(env, JNI_CLASS_RECORD_API)\n", g_VavaRecord_class_id);
	if(0 == g_VavaRecord_class_id)
	{
		assert(g_VavaRecord_class_id);
		return -1;
	}
	g_VavaRecord_global_class_id = (*env)->NewGlobalRef(env, g_VavaRecord_class_id);
	lbdebug("g_VavaRecord_global_class_id:%p = (*env)->FindClass(env, JNI_CLASS_RECORD_API)\n", g_VavaRecord_global_class_id);
	if(0 == g_VavaRecord_global_class_id)
	{
		assert(g_VavaRecord_global_class_id);
		return -1;
	}

	g_NativeEventNotify = (*env)->GetMethodID(env, g_VavaRecord_global_class_id, JAVA_FUNC_NAME, JAVA_FUNC_PARAM);
	if(0 == g_NativeEventNotify)
	{
		assert(g_NativeEventNotify);
		return -1;
	}
	return 0;
}

int event_notify(void* powner, int what, int arg1, int arg2)
{
	lbdebug("%s(powner:%p, what:%d, arg1:%d, arg2:%d)\n", __func__, powner, what, arg1, arg2);
	int status = 0;
    bool isAttached = false;
	JNIEnv* env = NULL;
    status = (*g_jvm)->GetEnv(g_jvm, (void**)&env, JNI_VERSION_1_4);
	lbdebug("status:%d = (*g_jvm)->GetEnv(g_jvm:%p, (void**)&env:%p, JNI_VERSION_1_4)\n", status, g_jvm, env);
    if (status < 0) {
        if ((*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL))//将当前线程注册到虚拟机中
        {
			lberror("g_jvm->AttachCurrentThread(&env, NULL) failed\n");
            return -1;
        }
		lbdebug("(*g_jvm)->AttachCurrentThread(g_jvm:%p, &env:%p, NULL)\n", g_jvm, env);
        isAttached = true;
    }

	(*env)->CallVoidMethod(env, (jobject)powner, g_NativeEventNotify, what, arg1, arg2);
	lbdebug("(*env)->CallVoidMethod(env:%p, powner:%p, g_NativeEventNotify:%p, what:%d, arg1:%d, arg2:%d)\n", env, powner, g_NativeEventNotify, what, arg1, arg2);
	if (isAttached) {
        (*g_jvm)->DetachCurrentThread(g_jvm);
    }
	lbdebug("event_notify end\n");
	return 0;
}

jint MediaRecord_Set_ID(JNIEnv *env, jobject thiz, struct avrecord_context* prc)
{
	jint ret = 0;

	lbdebug("%s(env:%p, thiz:%p, prc:%p) g_penv:%p, begin\n", __func__, env, thiz, prc, g_penv);
	g_penv = env;
	jfieldID field_record_id = lbjni_get_field_id_from_java(env, thiz, JAVA_NATIVE_MEDIARECORD_FIELD_NAME, "J", 0);
	if(NULL == field_record_id)
	{
		lberror("field_record_id:%d = lbjni_get_field_id_from_java(env, thiz, JAVA_NATIVE_MEDIARECORD_FIELD_NAME, J) failed\n", field_record_id);
		return -1;
	}
	ret = lbjni_set_long_field_id_from_java(env, thiz, field_record_id, (jlong)prc);
	lbtrace("ret:%d = lbjni_set_long_field_id_from_java(env:%p, jobject thiz:%p, field_record_id:%d, (jlong)prc:%p)\n", ret, env, thiz, field_record_id, prc);
	return ret;
}

struct avrecord_context* MediaRecord_Get_ID(JNIEnv *env, jobject thiz)
{
	struct avrecord_context* prc = NULL;
	jfieldID field_record_id = lbjni_get_field_id_from_java(env, thiz, JAVA_NATIVE_MEDIARECORD_FIELD_NAME, "J", 0);
	if(NULL == field_record_id)
	{
		lberror("field_record_id:%d = lbjni_get_field_id_from_java(env, thiz, JAVA_NATIVE_MEDIARECORD_FIELD_NAME, J) failed\n", field_record_id);
		return -1;
	}
	prc = (struct avrecord_context*)lbjni_get_long_field_id_from_java(env, thiz, field_record_id);
	lbdebug("prc:%p = lbjni_get_long_field_id_from_java(env:%p, thiz:%p, field_record_id:%d)\n", prc, env, thiz, field_record_id);
	return prc;
}

static int post_event(void* powner, int what, int arg1, int arg2)
{
	int ret = 0;
	static int attach = 0;
    static jmethodID method_id = 0;
	static jclass jcla = 0;
	const char* pmethod_name = "recordEventFromNative";
	const char* pclass = JNI_CLASS_RECORD_API;
	const char* pmethod_type = "(Ljava/lang/Object;III)I";
	JNIEnv* env = NULL;
    lbdebug("%s(g_jvm:%p, powner:%p, pmethod_name:%s, pclass:%s, pmethod_type:%s)\n", __func__, g_jvm, powner, pmethod_name, pclass, pmethod_type);
    if(NULL == g_jvm || NULL == pmethod_name || NULL == pclass || NULL == pmethod_type)
    {
        lberror("%s invalid parameter, g_jvm:%p, pmethod_name:%s, pclass:%p, pmethod_type:%s\n", __func__, g_jvm, pmethod_name, pclass, pmethod_type);
        return 0;
    }
	
	
	if(method_id == 0 && 0 == what)
	{
		jcla = (*g_penv)->FindClass(g_penv, pclass);
		lbdebug("cla:%p = (*g_penv)->FindClass(g_penv:%p, pclass:%s)\n", jcla, g_penv, pclass);
		if(NULL == jcla)
		{
			assert(jcla);
			lberror("jcla:%p = *g_penv)->FindClass(g_penv:%p, pclass:%s)\n", jcla, g_penv, pclass);
			return 0;
		}
		method_id = (*g_penv)->GetStaticMethodID(g_penv, jcla, pmethod_name, pmethod_type);
		lbdebug("method_id:%ld = (*g_penv)->GetMethodID(g_penv:%p, jcla:%p, pmethod_name:%s, pmethod_type:%s)\n", method_id , g_penv, jcla, pmethod_name, pmethod_type);
		if(NULL == method_id)
		{
			lbdebug("method_id:%ld = (*g_penv)->GetMethodID(g_penv:%p, jcla:%p, pmethod_name:%s, pmethod_type:%s) failed\n", method_id , g_penv, jcla, pmethod_name, pmethod_type);
			//lberror("%s method_id:%d = lbjni_get_methodid_from_java(g_penv:%p, jcla:%p, event_notify, (Ljava/lang/Object;III)I) failed\n", __func__, method_id, g_penv, jcla);
			assert(method_id);
			return -1;
		}
		return 0;
	}
	if(!attach)
	{
		ret = (*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL);
		if (ret != 0) {
			lberror("ret:%d = (*g_jvm)->AttachCurrentThread(g_jvm:%p, &env:%p, NULL) failed\n", ret, g_jvm, env);
			assert(ret == 0);
			return -1;
		}
		attach = 1;
	}
    
	lbdebug("before(*env)->CallStaticIntMethod(env:%p, jcla:%d, method_id:%d, powner:%p, what:%d, arg1:%d, arg2:%d)\n", env, jcla, method_id, powner, what, arg1, arg2);
	ret = (*env)->CallStaticIntMethod(env, jcla, method_id, (jobject)powner, what, arg1, arg2);
	lbdebug("ret:%d = (*env)->CallStaticIntMethod(env:%p, jcla:%p, method_id:%d, (jobject)powner:%p, what:%d, arg1:%d, arg2:%d)\n", ret, env, jcla, method_id, (jobject)powner, what, arg1, arg2);
    //(*g_jvm)->DetachCurrentThread(g_jvm);
	//lbdebug("(*g_jvm)->DetachCurrentThread(g_jvm:%p)\n", g_jvm);
	return 0;
}

static jobject g_obj = 0;

static void InitObject(JNIEnv *env, const char *path)
{
	lbdebug("%s(env:%p, path:%s) begin\n", __func__, env, path);
	jclass cls = (*env)->FindClass(env, path);
	lbdebug("%s cls:%p = (*env)->FindClass(env:%p, path:%s)\n", __func__, cls, env, path);
    if(!cls) {
		lberror("cls:%p = (*env)->FindClass(env:%p, path:%s) failed\n", cls, env, path);
        return;
    }
    jmethodID constr = (*env)->GetStaticMethodID(env, cls, "<init>", "()V");
	lbdebug("%s cls:%p = (*env)->GetStaticMethodID(env:%p, cls:%p, recordEventFromNative, (Ljava/lang/Object;III)I)\n", __func__, constr, env, cls);
    if(!constr) {
        return;
    }
    jobject obj = (*env)->NewObject(env, cls, constr);
	lbdebug("%s obj:%p = (*env)->NewObject(env, cls:%p, constr:%p)\n", __func__, obj, cls, constr);
    if(!obj) {
        return;
    }
    g_obj = (*env)->NewGlobalRef(env, obj);
	lbdebug("%s g_obj:%p = (*env)->NewGlobalRef(env, obj:%p)\n", __func__, g_obj, obj);
}
static int post_eventex(void* powner, int what, int arg1, int arg2)
{
	JNIEnv* env = NULL;
	int status = 0;
	int ret = 0;
	int attach = 0;
	const char* pmethod_name = "recordEventFromNative";
	const char* pclass = JNI_CLASS_RECORD_API;
	const char* pmethod_type = "(Ljava/lang/Object;III)I";
	if(NULL == g_jvm)
	{
		lberror("%s NULL == g_jvm\n", __func__);
		return -1;
	}

	status = (*g_jvm)->GetEnv(g_jvm, &env, JNI_VERSION_1_6);
	if(status < 0)
	{
		status = (*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL);
		if(status < 0)
		{
			lberror("status:%d = (*g_jvm)->AttachCurrentThread(&env:%p, NULL) failed\n",  status, env);
			return -1;
		}
		attach = 1;
	}

	jclass cla = (*env)->GetObjectClass(env, g_obj);
	lbdebug("cla:%p = (*env)->GetObjectClass(env:%p, g_obj:%p)\n", cla, env, g_obj);
	if(cla == 0)
	{
		lberror("%s cla == 0\n", __func__);
		return -1;
	}

	jmethodID method_id = (*env)->GetStaticMethodID(env, cla, pmethod_name, pmethod_type);
	lbdebug("%s method_id:%d = (*env)->GetStaticMethodID(env:%p, pmethod_name:%s, pmethod_type:%s)\n", __func__, method_id, env, pmethod_name, pmethod_type);
	if(0 == method_id)
	{
		lberror("%s method_id:%d = (*env)->GetStaticMethodID(env:%p, pmethod_name:%s, pmethod_type:%s) failed\n", __func__, method_id, env, pmethod_name, pmethod_type);
		return -1;
	}

	ret = (*env)->CallStaticIntMethod(cla, method_id, powner, what, arg1, arg2);
	lbdebug("ret:%d = (*env)->CallStaticIntMethod(cla:%d, method_id:%d, powner:%p, what:%d, arg1:%d, arg2:%d)\n", ret, cla, method_id, powner, what, arg1, arg2);
	if(attach)
	{
		lbdebug("before (*g_jvm)->DetachCurrentThread(g_jvm)\n");
		(*g_jvm)->DetachCurrentThread(g_jvm);
		lbdebug("after (*g_jvm)->DetachCurrentThread(g_jvm:%p)\n", g_jvm);
	}

	return ret;
}
/*static int post_event(void* powner, int what, int arg1, int arg2)
{
	lbdebug("%s(g_penv:%p, powner:%p, what:%d, arg1:%d, arg2:%d)\n", __func__, g_penv, powner, what, arg1, arg2);
    jmethodID method_id =  lbjni_get_static_methodid_from_java(g_penv, "recordEventFromNative", JNI_CLASS_RECORD_API, "(Ljava/lang/Object;III)I");//lbjni_get_methodid_from_java(g_penv, (jobject)powner, "recordEventFromNative", "(Ljava/lang/Object;III)I", 1);
	if(NULL == method_id)
	{
		lberror("method_id:%d = lbjni_get_methodid_from_java(g_penv:%p, weakpowner_this:%p, event_notify, (Ljava/lang/Object;III)I) failed\n", method_id, g_penv, powner);
		return -1;
	}
	// get class ptr
    jclass jcla = (*g_penv)->GetObjectClass(g_penv, powner);
	lbdebug("jcla = (*env)->GetObjectClass(g_penv:%p, powner:%p)\n", g_penv, powner);
    if(NULL == jcla)
    {
        lberror("jcla:%p = GetObjectClass failed\n", jcla);
        return 0;
    }
	jint ret = (*g_penv)->CallStaticIntMethod(g_penv, jcla, method_id, (jobject)powner, what, arg1, arg2);
	lbtrace("ret:%d = CallObjectMethod(g_penv:%p, powner:%p, method_id:%d, what:%d, arg1:%d, arg2:%d)\n", ret, g_penv, powner, method_id, what, arg1, arg2);
	return ret;
}*/

static jint MediaRecord_Open(JNIEnv *env, jobject thiz, /* jobject weakthiz,*/ jstring srcurl, jstring sinkurl, jstring tmpurl)
{
	char* psrcurl = NULL;
	char* psinkurl = NULL;
	char* ptmpurl = NULL;
	struct avrecord_context* prc = NULL;
	lbtrace("%s(srcurl:%p, sinkurl:%p, tmpurl:%p) begin\n", __func__, srcurl, sinkurl, tmpurl, sizeof(int), sizeof(struct avrecord_context*));
	if(srcurl)
	{
		psrcurl = (*env)->GetStringUTFChars(env, srcurl, NULL);
		//JNI_CHECK_GOTO(psinkurl, env, "java/lang/OutOfMemoryError", "mpjni: monstartup: psrcurl.string oom", LABEL_RETURN);
	}

    psinkurl = (*env)->GetStringUTFChars(env, sinkurl, NULL);
	if(NULL == psinkurl || strlen(psinkurl) <= 0)
	{
		lberror("psinkurl:%s = (*env)->GetStringUTFChars(env, sinkurl, NULL) failed\n", psinkurl);
		return -1;
	}

	if(tmpurl)
	{
		ptmpurl = (*env)->GetStringUTFChars(env, tmpurl, NULL);
	}

	jint id = avrecord_open_contextex(psrcurl, psinkurl, "mov", ptmpurl);
	struct avrecord_context* prc = (struct avrecord_context* prc)id;
    //prc = avrecord_open_context(psrcurl, psinkurl, "mov", ptmpurl);
	lbtrace("prc:%p = avrecord_open_context(psrcurl:%s, psinkurl:%s, mov, ptmpurl:%s), id:%ld\n", prc, psrcurl, psinkurl, ptmpurl, id);
    if(NULL == prc)
    {
    	lberror("prc:%p = avrecord_open_context(psrcurl:%s, psinkurl:%s, mov, ptmpurl:%s) failed\n", prc, psrcurl, psinkurl, ptmpurl);
    	return -1;
    }
	//prc = (struct avrecord_context*)id;
	lbtrace("prc:%p = (struct avrecord_context*)id:%ld\n", prc, id);
	MediaRecord_Set_ID(env, thiz, prc);
	jobject weak_thiz = (*env)->NewGlobalRef(env, thiz);
	lbtrace("weak_thiz:%p = (*env)->NewWeakGlobalRef(env:%p, thiz:%p)\n", weak_thiz, env, thiz);

	avrecord_set_callback(prc, event_notify, (void*)weak_thiz);
	lbtrace("avrecord_set_callback(prc:%p, event_notify:%p, (void*)weak_thiz:%p)\n", prc, event_notify, (void*)weak_thiz);
    
    return prc ? 0 : -1;
}

static jint MediaRecord_Start(JNIEnv *env, jobject thiz, jlong pts)
{
	lbtrace("%s(env:%p, thiz:%p, pts:%ld)\n", __func__, env, thiz, pts);
	struct avrecord_context* prc = MediaRecord_Get_ID(env, thiz);
	if(NULL == prc)
	{
		lbdebug("%s() invalid record prc:%p\n", __func__, prc);
		return -1;
	}
	int ret = avrecord_start(prc, pts);
	lbtrace("%s() ret:%d = avrecord_start(prc, pts:%ld)\n", __func__, ret, pts);
	return ret;
}

static jint MediaRecord_Deliver_Packet(JNIEnv *env, jobject thiz, jint codec_id, jbyteArray data, jlong pts, jlong frame_num)
{
	lbtrace("%s(env:%p, thiz:%p, codec_id:%d, data:%p, pts:%" PRId64 ", frame_num:%ld)\n", __func__, env, thiz, codec_id, data, pts, frame_num);
	struct avrecord_context* prc = MediaRecord_Get_ID(env, thiz);
	if(NULL == prc)
	{
		lbdebug("%s() invalid record prc:%p\n", __func__, prc);
		return -1;
	}
	int isCopy = 0;
	int datalen = 0;
	jbyte* pdata = NULL;
	if(data)
	{
		datalen = (*env)->GetArrayLength(env, data);
		pdata = (*env)->GetByteArrayElements(env, data, &isCopy);
	}
	
	int ret = avrecord_deliver_packet(prc, codec_id, pdata, datalen, pts*1000, frame_num);
	lbtrace("%s ret:%d = avrecord_deliver_packet(prc:%p, codec_id:%d, pdata:%p, datalen:%d, pts:%" PRId64 ", frame_num:%ld)\n", __func__, ret, prc, codec_id, pdata, datalen, pts, frame_num);
	if(pdata)
	{
		(*env)->ReleaseByteArrayElements(env, data, pdata, 0);
	}
	return ret;
}

static void MediaRecord_Stop(JNIEnv *env, jobject thiz, jint bcancel)
{
	lbtrace("%s(bcancel:%d)\n", __func__, bcancel);
	struct avrecord_context* prc = MediaRecord_Get_ID(env, thiz);
	if(NULL == prc)
	{
		lbdebug("%s() invalid record prc:%p\n", __func__, prc);
		return ;
	}
	avrecord_stop(prc, bcancel);
	lbdebug("avrecord_stop(prc:%p, bcancel:%d)\n", prc, bcancel);
	jobject weak_thiz = (jobject)avrecord_set_callback(prc, post_event, NULL);
	lbdebug("weak_thiz:%p = (jobject)avrecord_set_callback(prc, post_event, NULL)\n", weak_thiz);
	if(weak_thiz)
	{
		(*env)->DeleteGlobalRef(env, weak_thiz);
		lbdebug("after (*env)->DeleteGlobalRef(env, weak_thiz)\n");
	}
	
	avrecord_close_contextp(&prc);
	MediaRecord_Set_ID(env, thiz, prc);
	lbtrace("%s() avrecord_Stop(prc:%p, bcancel:%d)\n", __func__, prc, bcancel);
}

static jint MediaRecord_Get_Progress(JNIEnv *env, jobject thiz)
{
	struct avrecord_context* prc = MediaRecord_Get_ID(env, thiz);
	if(NULL == prc)
	{
		lbdebug("%s() invalid record prc:%p\n", __func__, prc);
		return -1;
	}
	jint per = avrecord_get_percent(prc);
	lbtrace("%s per:%d\n", __func__, per);
	return per;
}

static jboolean MediaRecord_Is_EOF(JNIEnv *env, jobject thiz)
{
	struct avrecord_context* prc = MediaRecord_Get_ID(env, thiz);
	if(NULL == prc)
	{
		lbdebug("%s() invalid record prc:%p\n", __func__, prc);
		return -1;
	}
	jboolean ret = (jboolean)avrecord_eof(prc);
	lbtrace("%s ret:%d = (jboolean)avrecord_eof(prc:%p)\n", __func__, (jint)ret, prc);
	return ret;
}

static JNINativeMethod g_record_methods[] = {
    {"open", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I", (void *) MediaRecord_Open},
	{"start", "(J)I", (void *) MediaRecord_Start},
	{"deliverPacket", "(I[BJJ)I", (void *) MediaRecord_Deliver_Packet},
	{"stop", "(I)V", (void *) MediaRecord_Stop},
	{"getPercent", "()I", (void *) MediaRecord_Get_Progress},
	{"recordIsEOF", "()Z", (void *) MediaRecord_Is_EOF},
	// read record function api
	//{"record_open"}

};

int int Record_Demux_global_init(JNIEnv *env)(JNIEnv *env)
{
    int ret = 0;
	lbtrace("%s(env:%p) begin\n", __func__, env);

	//memset(&g_pprec_jvm, 0, sizeof(record_jvm_t)* MAX_RECORD_NUM);
    IJK_FIND_JAVA_CLASS(env, g_record_clazz.clazz, JNI_CLASS_RECORD_API);
    (*env)->RegisterNatives(env, g_record_clazz.clazz, g_record_methods, NELEM(g_record_methods));
	lbtrace("%s(env:%p) end\n", __func__, env);
	LoadJavaEventNotify(g_jvm);
    return ret;
}

void Record_Demux_global_deinit(JNIEnv *env)
{
	lbtrace("Record_API_global_deinit(env:%p) begin\n", env);
	/*for(int i = 0; i < ; i++)
	{
		if(g_pprec_jvm[i])
		{
			free(g_pprec_jvm[i]);
			g_pprec_jvm[i] = NULL;
		}
	}*/
	(*env)->NewGlobalRef(env, g_VavaRecord_class_id);
	//(*env)->DeleteGlobalRef(env, g_obj);
	lbtrace("Record_API_global_deinit(env:%p) end\n", env);
}
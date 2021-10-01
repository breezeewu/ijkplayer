/*
 * common_func_jni.h
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

#ifndef LAZY_ANDROID_COMMON_FUNC_JNI_H
#include <jni.h>
#include <android/log.h>
#include "lazylog.h"
#ifndef lbdebug
#define lbdebug(...)  sv_trace(__VA_ARGS__)//__android_log_print(ANDROID_LOG_INFO, "IJKMedia", __VA_ARGS__)
#endif
#ifndef lbtrace
#define lbtrace(...)  sv_trace(__VA_ARGS__)//__android_log_print(ANDROID_LOG_INFO, "IJKMedia", __VA_ARGS__)
#endif
#ifndef lberror
#define lberror(...)  sv_error(__VA_ARGS__)//__android_log_print(ANDROID_LOG_ERROR, "IJKMedia", __VA_ARGS__)
#endif
// operate fieldid from java
jfieldID lbjni_get_field_id_from_java(JNIEnv *env, jobject thiz, const char* pfield_name, const char* pfield_type, int bis_static);

jint lbjni_set_string_field_id_from_java(JNIEnv *env, jobject thiz, jfieldID field_id, const char* pstr_val);

jint lbjni_set_Int_field_id_from_java(JNIEnv *env, jobject thiz, jfieldID field_id, jint val);

jint lbjni_set_long_field_id_from_java(JNIEnv *env, jobject thiz, jfieldID field_id, jlong val);

jint lbjni_get_string_field_id_from_java(JNIEnv *env, jobject thiz, jfieldID field_id, char* pstr, int len);

jint lbjni_get_int_field_id_from_java(JNIEnv *env, jobject thiz, jfieldID field_id);

jlong lbjni_get_long_field_id_from_java(JNIEnv *env, jobject thiz, jfieldID field_id);

// env->CallObjectMethod(thiz, methodid, ...);
jmethodID lbjni_get_methodid_from_java(JNIEnv *env, jobject thiz, const char* pmethod_name, const char* pmethod_type, int bis_static);

jmethodID lbjni_get_static_methodid_from_java(JNIEnv *env, const char* pmethod_name, const char* pclass, const char* pmethod_type);

#endif

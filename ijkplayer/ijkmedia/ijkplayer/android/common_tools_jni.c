/*
 *   common_func_jni.c
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

#include "common_tools_jni.h"
#include <assert.h>
#include <string.h>
#include <jni.h>

jfieldID lbjni_get_field_id_from_java(JNIEnv *env, jobject thiz, const char* pfield_name, const char* pfield_type, int bis_static)
{
    jfieldID field_id = 0;
    lbdebug("%s(env:%p, thiz:%p, pfield_name:%s, pfield_type:%s, bis_static:%d)\n", __func__, env, thiz, pfield_name, pfield_type, (int)bis_static);
    if(NULL == env || NULL == pfield_name || NULL == pfield_type)
    {
        lberror("%s Invalid parameter, env:%p, pfield_name:%s, pfield_type:%s\n", __func__, env, pfield_name, pfield_type);
        return 0;
    }

    // get class ptr
    jclass jcla = (*env)->GetObjectClass(env, thiz);
    if(NULL == jcla)
    {
        lberror("%s jcla:%p = GetObjectClass failed\n", __func__, jcla);
        return 0;
    }

    // get variable field id
    if(bis_static)
    {
        field_id = (*env)->GetStaticFieldID(env, jcla, pfield_name, pfield_type);
        lbdebug("field_id:%ld = (*env)->GetStaticFieldID(env:%p, jcla:%p, pfield_name:%s, pfield_type:%s)\n", field_id, env, jcla, pfield_name, pfield_type);
    }
    else
    {
        field_id = (*env)->GetFieldID(env, jcla, pfield_name, pfield_type);
        lbdebug("field_id:%ld = (*env)->GetFieldID(env:%p, jcla:%p, pfield_name:%s, pfield_type:%s)\n", field_id, env, jcla, pfield_name, pfield_type);
    }

    return field_id;
}

jint lbjni_set_string_field_id_from_java(JNIEnv *env, jobject thiz, jfieldID field_id, const char* pstr_val)
{
    jstring jstr_val;
    lbdebug("%s(env:%p, thiz:%p, field_id:%ld, pstr_val:%s)\n", __func__, env, thiz, field_id, pstr_val);
    if(NULL == env || NULL == field_id)
    {
        lberror("%s invalid parameter, env:%p, field_id:%ld\n", __func__, env, field_id);
        return -1;
    }
    /*// get class ptr
    jclass jcla = (*env)->GetObjectClass(env, thiz);
    if(NULL == jcla)
    {
        lberror("jcla:%p = GetObjectClass failed\n", jcla);
        return -1;
    }*/

    if(pstr_val)
    {
        jstr_val = (*env)->NewStringUTF(env, pstr_val);
    }
    
    (*env)->SetObjectField(env, thiz, field_id, jstr_val);
    lbdebug("env->SetObjectField(thiz, field_id:%ld, pstr_val:%s)\n", field_id, pstr_val);
    return 0;
}

jint lbjni_set_Int_field_id_from_java(JNIEnv *env, jobject thiz, jfieldID field_id, jint val)
{
    lbdebug("%s(env:%p, thiz:%p, field_id:%ld, val:%ld)\n", __func__, env, thiz, field_id, val);
    if(NULL == env || NULL == field_id)
    {
        lberror("%s invalid parameter, env:%p, field_id:%ld\n", __func__, env, field_id);
        return -1;
    }
    /*// get class ptr
    jclass jcla = (*env)->GetObjectClass(env, thiz);
    if(NULL == jcla)
    {
        lberror("jcla:%p = GetObjectClass failed\n", jcla);
        return -1;
    }*/
    
    (*env)->SetIntField(env, thiz, field_id, val);
    lbdebug("env->SetIntField(thiz, field_id:%ld, val:%ld)\n", field_id, val);
    return 0;
}

jint lbjni_set_long_field_id_from_java(JNIEnv *env, jobject thiz, jfieldID field_id, jlong val)
{
    lbdebug("%s(env:%p, thiz:%p, field_id:%ld, val:%ld)\n", __func__, env, thiz, field_id, val);
    if(NULL == env || NULL == field_id)
    {
        lberror("%s invalid parameter, env:%p, field_id:%ld\n", __func__, env, field_id);
        return -1;
    }
    /*// get class ptr
    jclass jcla = (*env)->GetObjectClass(env, thiz);
    if(NULL == jcla)
    {
        lberror("jcla:%p = GetObjectClass failed\n", jcla);
        return -1;
    }*/
    
    (*env)->SetLongField(env, thiz, field_id, val);
    lbdebug("(*env)->SetLongField(env, thiz, field_id:%ld, val:%ld)\n", field_id, val);
    return 0;
}

jint lbjni_get_string_field_id_from_java(JNIEnv *env, jobject thiz, jfieldID field_id, char* pstr, int len)
{
    jstring jstr_val;
    int str_len = 0;
    lbdebug("%s(env:%p, thiz:%p, field_id:%ld)\n", __func__, env, thiz, field_id);
    if(NULL == env || NULL == field_id)
    {
        lberror("%s invalid parameter, env:%p, field_id:%ld\n", __func__, env, field_id);
        return -1;
    }
    /*// get class ptr
    jclass jcla = (*env)->GetObjectClass(env, thiz);
    if(NULL == jcla)
    {
        lberror("jcla:%p = GetObjectClass failed\n", jcla);
        return -1;
    }*/

    
    jstr_val = (*env)->GetObjectField(env, thiz, field_id);
    //lbdebug("(*env)->GetObjectField(env, thiz, field_id:%ld)\n", field_id);

    const char* pstr_val = (*env)->GetStringUTFChars(env, jstr_val, NULL);
    //lbdebug("pstr_val:%s = (*env)->GetStringUTFChars(env, jstr_val, NULL)\n", pstr_val);
    if(pstr_val)
    {
        str_len = strlen(pstr_val) + 1;
        if(str_len <= len)
        {
            memcpy(pstr, pstr_val, str_len);
        }
        else
        {
            lberror("not enought memory buffer for string copy, need:%d, have:%d\n", str_len, len);
            str_len = 0;
        }
        (*env)->ReleaseStringUTFChars(env, jstr_val, pstr_val);
    }
    
    return str_len;
}

jint lbjni_get_int_field_id_from_java(JNIEnv *env, jobject thiz, jfieldID field_id)
{
    jint val = 0;
    lbdebug("%s(env:%p, thiz:%p, field_id:%ld)\n", __func__, env, thiz, field_id);
    if(NULL == env || NULL == field_id)
    {
        lberror("%s invalid parameter, env:%p, field_id:%ld\n", __func__, env, field_id);
        return 0;
    }
    /*// get class ptr
    jclass jcla = (*env)->GetObjectClass(env, thiz);
    if(NULL == jcla)
    {
        lberror("jcla:%p = GetObjectClass failed\n", jcla);
        return 0;
    }*/
    
    val = (*env)->GetIntField(env, thiz, field_id);
    lbdebug("val:%d = env->GetIntField(thiz, thiz, field_id:%ld)\n", val, field_id);
    
    return val;
}

jlong lbjni_get_long_field_id_from_java(JNIEnv *env, jobject thiz, jfieldID field_id)
{
    jlong val = 0;
    lbdebug("%s(env:%p, thiz:%p, field_id:%ld)\n", __func__, env, thiz, field_id);
    if(NULL == env || NULL == field_id)
    {
        lberror("%s invalid parameter, env:%p, field_id:%ld\n", __func__, env, field_id);
        return 0;
    }
    /*// get class ptr
    jclass jcla = (*env)->GetObjectClass(env, thiz);
    if(NULL == jcla)
    {
        lberror("jcla:%p = GetObjectClass failed\n", jcla);
        return 0;
    }*/

    
    val = (*env)->GetLongField(env, thiz, field_id);
    lbdebug("val:%ld = env->GetLongField(thiz, field_id:%ld)\n", val, field_id);
    
    return val;
}

jmethodID lbjni_get_methodid_from_java(JNIEnv *env, jobject thiz, const char* pmethod_name, const char* pmethod_type, int bis_static)
{
    jmethodID method_id = 0;
    lbdebug("%s(env:%p, thiz:%p, pmethod_name:%s, pmethod_type:%s, bis_static:%d)\n", __func__, env, thiz, pmethod_name, pmethod_type, bis_static);
    if(NULL == env || NULL == pmethod_name || NULL == pmethod_type)
    {
        lberror("%s invalid parameter, env:%p, pmethod_name:%s, pmethod_type:%s\n", __func__, env, pmethod_name, pmethod_type);
        return 0;
    }

    // get class ptr
    jclass jcla = (*env)->GetObjectClass(env, thiz);
    if(NULL == jcla)
    {
        lberror("jcla:%p = GetObjectClass failed\n", jcla);
        return 0;
    }

    if(bis_static)
    {
        //jclass clazz = (*env)->FindClass(env, class_sign);
        lbdebug("(*env)-> GetStaticMethodID(env:%p, jcla:%p, pmethod_name:%s, pmethod_type:%s)\n", env, jcla, pmethod_name, pmethod_type);
        method_id = (*env)->GetStaticMethodID(env, jcla, pmethod_name, pmethod_type);
    }
    else
    {
        lbdebug("(*env)->GetMethodID(env:%p, thiz:%p, pmethod_name:%s, pmethod_type:%s)\n", env, thiz, pmethod_name, pmethod_type);
        method_id = (*env)->GetMethodID(env, thiz, pmethod_name, pmethod_type);
    }
    
    lbdebug("method_id:%ld = (*env)->GetMethodID(env:%p, thiz:%p, pmethod_name:%s, pmethod_type:%s)\n", method_id , env, thiz, pmethod_name, pmethod_type);
    
    return method_id;
}

jmethodID lbjni_get_static_methodid_from_java(JNIEnv *env, const char* pmethod_name, const char* pclass, const char* pmethod_type)
{
    jmethodID method_id = 0;
    lbdebug("%s(env:%p, pmethod_name:%s, pclass:%s, pmethod_type:%s)\n", __func__, env, pmethod_name, pclass, pmethod_type);
    if(NULL == env || NULL == pmethod_name || NULL == pclass || NULL == pmethod_type)
    {
        lberror("%s invalid parameter, env:%p, pmethod_name:%s, pclass:%p, pmethod_type:%s\n", __func__, env, pmethod_name, pclass, pmethod_type);
        return 0;
    }

    jclass jcla = (*env)->FindClass(env, pclass);
    lbdebug("cla:%p = (*env)->FindClass(env:%p, pclass:%s)\n", jcla, env, pclass);
    if(NULL == jcla)
    {
        lberror("jcla:%p = *env)->FindClass(env:%p, pclass:%s)\n", jcla, env, pclass);
        return 0;
    }
    method_id = (*env)->GetStaticMethodID(env, jcla, pmethod_name, pmethod_type);
    lbdebug("method_id:%ld = (*env)->GetStaticMethodID(env:%p, jcla:%p, pmethod_name:%s, pmethod_type:%s)\n", method_id , env, jcla, pmethod_name, pclass);

    return method_id;
}
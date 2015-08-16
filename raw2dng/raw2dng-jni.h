/* Copyright (C) 2015 Fimagena

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#pragma once


#include <jni.h>
//#include <android/log.h>
//#define  LOG(MSG_STRING, ...)  __android_log_print(ANDROID_LOG_ERROR, MSG_STRING, __VA_ARGS__)

extern "C" {
    JNIEXPORT void JNICALL Java_com_fimagena_raw2dng_RawConverter_registerListener(JNIEnv *jEnv, jobject jObject, jstring jCallbackMethodName);
    JNIEXPORT void JNICALL Java_com_fimagena_raw2dng_RawConverter_deRegisterListener(JNIEnv *jEnv, jobject jObject);
    void sendJNIProgressUpdate(const char *message);

    JNIEXPORT void JNICALL Java_com_fimagena_raw2dng_RawConverter_raw2dng
        (JNIEnv *jEnv, jobject jObject, jstring jRawFilename, jstring jOutFilename, jstring jDcpFilename, jboolean jEmbedOriginal);
    JNIEXPORT void JNICALL Java_com_fimagena_raw2dng_RawConverter_raw2tiff
        (JNIEnv *jEnv, jobject jObject, jstring jRawFilename, jstring jOutFilename, jstring jDcpFilename);
    JNIEXPORT void JNICALL Java_com_fimagena_raw2dng_RawConverter_raw2jpeg
        (JNIEnv *jEnv, jobject jObject, jstring jRawFilename, jstring jOutFilename, jstring jDcpFilename);
}

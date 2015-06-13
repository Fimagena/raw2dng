/* This library is free software; you can redistribute it and/or
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

#include <string>
#include <exception>

void raw2dng(std::string rawFilename, std::string dngFilename, std::string dcpFilename, bool embedOriginal = false);


#ifdef ANDROID

#include <jni.h>
#include <android/log.h>
#define  LOG(MSG_STRING, ...)  __android_log_print(ANDROID_LOG_ERROR, MSG_STRING, __VA_ARGS__)

JNIEnv *m_jEnvironment;
jobject m_jCaller;
jmethodID m_jMethodId = NULL;

extern "C" {
    JNIEXPORT void JNICALL Java_com_fimagena_raw2dng_DngConverter_registerListener(
            JNIEnv *jEnv, jobject jObject, jstring jCallbackMethodName) {
        m_jCaller = jEnv->NewGlobalRef(jObject);

        const char *callbackMethodName = jEnv->GetStringUTFChars(jCallbackMethodName, NULL);
        m_jMethodId = jEnv->GetMethodID(jEnv->GetObjectClass(m_jCaller), callbackMethodName, "(Ljava/lang/String;)V");
        jEnv->ReleaseStringUTFChars(jCallbackMethodName, callbackMethodName);

        if (m_jMethodId == NULL) jEnv->ThrowNew(jEnv->FindClass("java/lang/Exception"), "Could not find callback method signature!");
    }
    JNIEXPORT void JNICALL Java_com_fimagena_raw2dng_DngConverter_deRegisterListener(JNIEnv *jEnv, jobject jObject) {
        m_jMethodId = NULL;
        jEnv->DeleteGlobalRef(m_jCaller);
    }

    void sendJNIProgressUpdate(const char *message) {
        if (m_jMethodId == NULL) return;
        jstring jMessage = m_jEnvironment->NewStringUTF(message);
        m_jEnvironment->CallVoidMethod(m_jCaller, m_jMethodId, jMessage);
    }

    JNIEXPORT void JNICALL Java_com_fimagena_raw2dng_DngConverter_raw2dng(JNIEnv *jEnv, jobject jObject,
            jstring jRawFilename, jstring jDngFilename, jstring jDcpFilename, jboolean jEmbedOriginal) {
        m_jEnvironment = jEnv;

        const char *rawString = jEnv->GetStringUTFChars(jRawFilename, NULL);
        const char *dngString = jEnv->GetStringUTFChars(jDngFilename, NULL);
        const char *dcpString = jEnv->GetStringUTFChars(jDcpFilename, NULL);

        std::string rawFilename(rawString);
        std::string dngFilename(dngString);
        std::string dcpFilename(dcpString);

        jEnv->ReleaseStringUTFChars(jRawFilename, rawString);
        jEnv->ReleaseStringUTFChars(jDngFilename, dngString);
        jEnv->ReleaseStringUTFChars(jDcpFilename, dcpString);

        try {
            raw2dng(rawFilename, dngFilename, dcpFilename, jEmbedOriginal);
        }
        catch (std::exception& e) {
            jEnv->ThrowNew(jEnv->FindClass("java/lang/Exception"), e.what());
        }
    }
}
#endif

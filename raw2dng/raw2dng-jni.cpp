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

#include "raw2dng-jni.h"
#include "rawConverter.h"

#include <jni.h>
#include <cstddef>
#include <string>


JNIEnv *m_jEnvironment;
jobject m_listenerObject = NULL;
jmethodID m_listenerMethod = NULL;

void publishProgressUpdate(const char *message) {sendJNIProgressUpdate(message);}

extern "C" {
    JNIEXPORT void JNICALL Java_com_fimagena_raw2dng_RawConverter_registerListener(
            JNIEnv *jEnv, jobject jObject, jstring jCallbackMethodName) {
        m_listenerObject = jEnv->NewGlobalRef(jObject);

        const char *callbackMethodName = jEnv->GetStringUTFChars(jCallbackMethodName, NULL);
        m_listenerMethod = jEnv->GetMethodID(jEnv->GetObjectClass(m_listenerObject), callbackMethodName, "(Ljava/lang/String;)V");
        jEnv->ReleaseStringUTFChars(jCallbackMethodName, callbackMethodName);

        if (m_listenerMethod == NULL) jEnv->ThrowNew(jEnv->FindClass("java/lang/Exception"), "Could not find callback method signature!");
    }
    JNIEXPORT void JNICALL Java_com_fimagena_raw2dng_RawConverter_deRegisterListener(JNIEnv *jEnv, jobject jObject) {
        m_listenerMethod = NULL;
        jEnv->DeleteGlobalRef(m_listenerObject);
    }
    void sendJNIProgressUpdate(const char *message) {
        if (m_listenerMethod == NULL) return;
        jstring jMessage = m_jEnvironment->NewStringUTF(message);
        m_jEnvironment->CallVoidMethod(m_listenerObject, m_listenerMethod, jMessage);
    }

    void convertFile(int, JNIEnv*, jobject, jstring, jstring, jstring, jboolean = false);

    JNIEXPORT void JNICALL Java_com_fimagena_raw2dng_RawConverter_raw2dng
        (JNIEnv *jEnv, jobject jObject, jstring jRawFilename, jstring jOutFilename, jstring jDcpFilename, jboolean jEmbedOriginal) {
        convertFile(0, jEnv, jObject, jRawFilename, jOutFilename, jDcpFilename, jEmbedOriginal);
    }
    JNIEXPORT void JNICALL Java_com_fimagena_raw2dng_RawConverter_raw2tiff
        (JNIEnv *jEnv, jobject jObject, jstring jRawFilename, jstring jOutFilename, jstring jDcpFilename) {
        convertFile(1, jEnv, jObject, jRawFilename, jOutFilename, jDcpFilename);
    }
    JNIEXPORT void JNICALL Java_com_fimagena_raw2dng_RawConverter_raw2jpeg
        (JNIEnv *jEnv, jobject jObject, jstring jRawFilename, jstring jOutFilename, jstring jDcpFilename) {
        convertFile(2, jEnv, jObject, jRawFilename, jOutFilename, jDcpFilename);
    }


    void convertFile(int type,
        JNIEnv *jEnv, jobject jObject, jstring jRawFilename, jstring jOutFilename, jstring jDcpFilename, jboolean jEmbedOriginal) {

        m_jEnvironment = jEnv;

        const char *rawString = jEnv->GetStringUTFChars(jRawFilename, NULL);
        const char *outString = jEnv->GetStringUTFChars(jOutFilename, NULL);
        const char *dcpString = jEnv->GetStringUTFChars(jDcpFilename, NULL);

        std::string rawFilename(rawString);
        std::string outFilename(outString);
        std::string dcpFilename(dcpString);

        jEnv->ReleaseStringUTFChars(jRawFilename, rawString);
        jEnv->ReleaseStringUTFChars(jOutFilename, outString);
        jEnv->ReleaseStringUTFChars(jDcpFilename, dcpString);

        RawConverter converter;
        try {
            converter.openRawFile(rawFilename);
            converter.buildNegative(dcpFilename);
            if (jEmbedOriginal) converter.embedRaw(rawFilename);
            converter.renderImage();

            switch (type) {
                default:
                case 0: {
                    AutoPtr<dng_preview_list> previews(converter.renderPreviews());
                    converter.writeDng(outFilename, previews.Get());
                    // FIXME: check: are we leaking the previews?
                    break;
                }
                case 1: {
                    AutoPtr<dng_preview_list> previews(converter.renderPreviews());
                    converter.writeTiff(outFilename, dynamic_cast<const dng_jpeg_preview*>(&previews->Preview(1)));
                    // FIXME: check: are we leaking the previews?
                    break;
                }
                case 2: 
                    converter.writeJpeg(outFilename);
                    break;
            }
        }
        catch (std::exception& e) {
            jEnv->ThrowNew(jEnv->FindClass("java/lang/Exception"), e.what());
        }
    }
}
/*
 * Copyright 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <jni.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <android/log.h>

// parselib includes
#include <stream/MemInputStream.h>
#include <wav/WavStreamReader.h>

#include <player/OneShotSampleSource.h>
#include <player/SimpleMultiPlayer.h>

#include "minimp3.h"
#include "minimp3_ex.h"

static const char* TAG = "DrumPlayerJNI";

// JNI functions are "C" calling convention
#ifdef __cplusplus
extern "C" {
#endif

using namespace iolib;
using namespace parselib;

static SimpleMultiPlayer sDTPlayer;

/**
 * Native (JNI) implementation of DrumPlayer.setupAudioStreamNative()
 */
JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_setupAudioStreamNative(
        JNIEnv* env, jobject, jint numChannels) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "%s", "init()");
    sDTPlayer.setupAudioStream(numChannels);  // Pass audioSessionId to SimpleMultiPlayer
}

JNIEXPORT void JNICALL
Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_startAudioStreamNative(
        JNIEnv *env, jobject thiz) {
    sDTPlayer.startStream();
}

/**
 * Native (JNI) implementation of DrumPlayer.teardownAudioStreamNative()
 */
JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_teardownAudioStreamNative(JNIEnv* , jobject) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "%s", "deinit()");

    // we know in this case that the sample buffers are all 1-channel, 44.1K
    sDTPlayer.teardownAudioStream();
}

/**
 * Native (JNI) implementation of DrumPlayer.allocSampleDataNative()
 */
/**
 * Native (JNI) implementation of DrumPlayer.loadWavAssetNative()
 */
JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_loadWavAssetNative(
        JNIEnv* env, jobject, jbyteArray bytearray, jint index, jfloat pan) {
    int len = env->GetArrayLength (bytearray);

    unsigned char* buf = new unsigned char[len];
    env->GetByteArrayRegion (bytearray, 0, len, reinterpret_cast<jbyte*>(buf));

    MemInputStream stream(buf, len);

    WavStreamReader reader(&stream);
    reader.parse();

    reader.getNumChannels();

    SampleBuffer* sampleBuffer = new SampleBuffer();
    sampleBuffer->loadSampleData(&reader);

    OneShotSampleSource* source = new OneShotSampleSource(sampleBuffer, pan);
    sDTPlayer.addSampleSource(source, sampleBuffer);

    delete[] buf;
}

JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_loadMp3AssetNative(
        JNIEnv *env, jobject, jstring filePath, jint index, jfloat pan) {
    const char *nativeFilePath = env->GetStringUTFChars(filePath, nullptr);

    mp3dec_t mp3d;
    mp3dec_file_info_t info;
    mp3dec_init(&mp3d);

    if (mp3dec_load(&mp3d, nativeFilePath, &info, NULL, NULL)) {
        __android_log_print(ANDROID_LOG_ERROR, "SimpleMultiPlayer", "Failed to load MP3 file");
        env->ReleaseStringUTFChars(filePath, nativeFilePath);
        return;
    }

    SampleBuffer* sampleBuffer = new SampleBuffer();
    sampleBuffer->loadRawSampleData(info.buffer, info.samples / info.channels, info.channels, info.hz);

    OneShotSampleSource* source = new OneShotSampleSource(sampleBuffer, pan);
    sDTPlayer.addSampleSource(source, sampleBuffer);

    free(info.buffer);
    env->ReleaseStringUTFChars(filePath, nativeFilePath);
}

/**
 * Native (JNI) implementation of DrumPlayer.unloadWavAssetsNative()
 */
JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_unloadWavAssetsNative(JNIEnv* env, jobject) {
    sDTPlayer.unloadSampleData();
}

/**
 * Native (JNI) implementation of DrumPlayer.trigger()
 */
JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_trigger(JNIEnv* env, jobject, jint index) {
    sDTPlayer.triggerDown(index);
}

/**
 * Native (JNI) implementation of DrumPlayer.trigger()
 */
JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_stopTrigger(JNIEnv* env, jobject, jint index) {
    sDTPlayer.triggerUp(index);
}

/**
 * Native (JNI) implementation of DrumPlayer.getOutputReset()
 */
JNIEXPORT jboolean JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_getOutputReset(JNIEnv*, jobject) {
    return sDTPlayer.getOutputReset();
}

/**
 * Native (JNI) implementation of DrumPlayer.clearOutputReset()
 */
JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_clearOutputReset(JNIEnv*, jobject) {
    sDTPlayer.clearOutputReset();
}

/**
 * Native (JNI) implementation of DrumPlayer.restartStream()
 */
JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_restartStream(JNIEnv*, jobject) {
    sDTPlayer.resetAll();
    if (sDTPlayer.openStream() && sDTPlayer.startStream()){
        __android_log_print(ANDROID_LOG_INFO, TAG, "openStream successful");
    } else {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "openStream failed");
    }
}

JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_setPan(
        JNIEnv *env, jobject thiz, jint index, jfloat pan) {
    sDTPlayer.setPan(index, pan);
}

JNIEXPORT jfloat JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_getPan(
        JNIEnv *env, jobject thiz, jint  index) {
    return sDTPlayer.getPan(index);
}

JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_setGain(
        JNIEnv *env, jobject thiz, jint  index, jfloat gain) {
    sDTPlayer.setGain(index, gain);
}

JNIEXPORT jfloat JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_getGain(
        JNIEnv *env, jobject thiz, jint index) {
    return sDTPlayer.getGain(index);
}

JNIEXPORT jint JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_getCurrentSampleIndex(
        JNIEnv *env, jobject thiz, jint index) {
    return sDTPlayer.getCurrentSampleIndex(index);
}

JNIEXPORT jfloat JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_getCurrentTimeInSeconds(
        JNIEnv *env, jobject thiz, jint index) {
    return sDTPlayer.getCurrentTimeInSeconds(index);
}

JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_setPlaybackTimeInSeconds(
        JNIEnv *env, jobject thiz, jfloat newTime) {
    sDTPlayer.setCurrentTimeInSeconds(newTime);
}

JNIEXPORT jfloat JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_getTotalLengthInSeconds(
        JNIEnv *env, jobject thiz, jint index) {
    return sDTPlayer.getTotalLengthInSeconds(index);
}

JNIEXPORT jboolean JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_isSampleSourcePlaying(
        JNIEnv* env, jobject, jint index) {
    if (index < sDTPlayer.mNumSampleBuffers) {
        return sDTPlayer.mSampleSources[index]->isPlaying();
    }
    return JNI_FALSE;
}

// JNI method to set the tempo
JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_setTempoNative(
        JNIEnv* env, jobject, jfloat tempo) {
    sDTPlayer.setTempo(tempo);
}

// JNI method to set the pitch
JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_setPitchSemiTonesNative(
        JNIEnv* env, jobject, jfloat pitch) {
    sDTPlayer.setPitchSemiTones(pitch);
}

// JNI method to get the current tempo
JNIEXPORT jfloat JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_getTempoNative(
        JNIEnv* env, jobject) {
    return sDTPlayer.getTempo();
}

// JNI method to get the current pitch
JNIEXPORT jfloat JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_getPitchSemiTonesNative(
        JNIEnv* env, jobject) {
    return sDTPlayer.getPitchSemiTones();
}

JNIEXPORT void JNICALL Java_com_stephanduechtel_multitrackplayer_PlayerViewModel_setIgnorePitchIndexesNative(
        JNIEnv* env, jobject, jintArray indexes) {
    jsize length = env->GetArrayLength(indexes);
    jint* elements = env->GetIntArrayElements(indexes, 0);

    std::vector<int32_t> ignorePitchIndexes(elements, elements + length);
    sDTPlayer.setIgnorePitchIndexes(ignorePitchIndexes);

    env->ReleaseIntArrayElements(indexes, elements, 0);
}


#ifdef __cplusplus
}
#endif

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

#include <android/log.h>

// parselib includes
#include <stream/MemInputStream.h>
#include <wav/WavStreamReader.h>

// local includes
#include "OneShotSampleSource.h"
#include "SimpleMultiPlayer.h"

#include <atomic>
#include <vector>
#include <thread>
#include <chrono>

static const char* TAG = "SimpleMultiPlayer";

using namespace oboe;
using namespace parselib;

namespace iolib {

constexpr int32_t kBufferSizeInBursts = 32; //32; // Use 2 bursts as the buffer size (double buffer)

SimpleMultiPlayer::SimpleMultiPlayer()
  : mChannelCount(0), mOutputReset(false), mSampleRate(0), mNumSampleBuffers(0), mInitialGains({})
{}

DataCallbackResult SimpleMultiPlayer::MyDataCallback::onAudioReady(AudioStream *oboeStream,
                                                                   void *audioData,
                                                                   int32_t numFrames) {

    auto result = oboeStream->getXRunCount();
    if (result) { // Check if the result is successful
        int32_t currentXRunCount = result.value();
        if (currentXRunCount != mPreviousXRunCount) {
            LOGD("oboeStream->getXRunCount(): %d", currentXRunCount);
            mPreviousXRunCount = currentXRunCount;
        }
    }


    StreamState streamState = oboeStream->getState();
    if (streamState != StreamState::Open && streamState != StreamState::Started) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "  streamState:%d", streamState);
    }
    if (streamState == StreamState::Disconnected) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "  streamState::Disconnected");
    }

    memset(audioData, 0, static_cast<size_t>(numFrames) * static_cast<size_t>
            (mParent->mChannelCount) * sizeof(float));

    // Adjust Pitch if needed
    if (mParent->mCurrentPitch != mParent->mSampleSources[0]->currentPitch) {
        int32_t referenceSampleIndex = mParent->mSampleSources[0]->getCurrentSampleIndex() - (mParent->mSampleSources[0]->mSoundTouch.numSamples()*2);
        for(int32_t index = 0; index < mParent->mNumSampleBuffers; index++) {
            int32_t sampleChannels = mParent->mSampleBuffers[index]->getProperties().channelCount;
            if (sampleChannels == 1){
                referenceSampleIndex = referenceSampleIndex / 2;
            }
            mParent->mSampleSources[index]->currentPitch = mParent->mCurrentPitch;
            mParent->mSampleSources[index]->mSoundTouch.clear();
            mParent->mSampleSources[index]->setCurrentSampleIndex(referenceSampleIndex);
            mParent->mSampleSources[index]->mSoundTouch.setPitchSemiTones(mParent->mCurrentPitch);
        }

    }


    // OneShotSampleSource* sources = mSampleSources.get();
    for(int32_t index = 0; index < mParent->mNumSampleBuffers; index++) {
        if (mParent->mSampleSources[index]->isPlaying()) {
            mParent->mSampleSources[index]->mixAudio((float*)audioData, mParent->mChannelCount, numFrames);
        }
    }

    if (mParent->mLatencyTuner) {
        mParent->mLatencyTuner->tune();
    }

    return DataCallbackResult::Continue;
}

void SimpleMultiPlayer::MyErrorCallback::onErrorAfterClose(AudioStream *oboeStream, Result error) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "==== onErrorAfterClose() error:%d", error);

    mParent->resetAll();
    if (mParent->openStream() && mParent->startStream()) {
        mParent->mOutputReset = true;
    }
}

bool SimpleMultiPlayer::openStream() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "openStream()");

    // Use shared_ptr to prevent use of a deleted callback.
    mDataCallback = std::make_shared<MyDataCallback>(this);
    mErrorCallback = std::make_shared<MyErrorCallback>(this);

    // Create an audio stream
    AudioStreamBuilder builder;
    builder.setChannelCount(mChannelCount);
    // we will resample source data to device rate, so take default sample rate
    builder.setDataCallback(mDataCallback);
    builder.setErrorCallback(mErrorCallback);
    builder.setPerformanceMode(PerformanceMode::LowLatency);
    builder.setSharingMode(SharingMode::Exclusive);
//    builder.setBufferCapacityInFrames(32768);
//    builder.setFramesPerCallback(480);
    builder.setSampleRateConversionQuality(SampleRateConversionQuality::Medium);

    Result result = builder.openStream(mAudioStream);
    if (result != Result::OK){
        __android_log_print(
                ANDROID_LOG_ERROR,
                TAG,
                "openStream failed. Error: %s", convertToText(result));
        return false;
    } else {
        int32_t framesPerCallback = mAudioStream->getFramesPerCallback();
        LOGD("Default FramesPerCallback: %d", framesPerCallback);
    }

    // Reduce stream latency by setting the buffer size to a multiple of the burst size
    // Note: this will fail with ErrorUnimplemented if we are using a callback with OpenSL ES
    // See oboe::AudioStreamBuffered::setBufferSizeInFrames
    LOGD("setFramesPerBurst(): %d", mAudioStream->getFramesPerBurst());
    LOGD("setBufferSizeInFrames: %d", mAudioStream->getFramesPerBurst() * kBufferSizeInBursts);
    result = mAudioStream->setBufferSizeInFrames(mAudioStream->getFramesPerBurst() * kBufferSizeInBursts);
//    result = mAudioStream->setBufferSizeInFrames(32768);
    if (result != Result::OK) {
        __android_log_print(
                ANDROID_LOG_WARN,
                TAG,
                "setBufferSizeInFrames failed. Error: %s", convertToText(result));
    }

    mSampleRate = mAudioStream->getSampleRate();

    mLatencyTuner = std::make_unique<LatencyTuner>(*mAudioStream);

    return true;
}

bool SimpleMultiPlayer::startStream() {
    int tryCount = 0;
    while (tryCount < 3) {
        bool wasOpenSuccessful = true;
        // Assume that apenStream() was called successfully before startStream() call.
        if (tryCount > 0) {
            usleep(20 * 1000); // Sleep between tries to give the system time to settle.
            wasOpenSuccessful = openStream(); // Try to open the stream again after the first try.
        }
        if (wasOpenSuccessful) {
            Result result = mAudioStream->requestStart();
            if (result != Result::OK){
                __android_log_print(
                        ANDROID_LOG_ERROR,
                        TAG,
                        "requestStart failed. Error: %s", convertToText(result));
                mAudioStream->close();
                mAudioStream.reset();
            } else {
                return true;
            }
        }
        tryCount++;
    }

    return false;
}

    void SimpleMultiPlayer::setupAudioStream(int32_t channelCount) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "setupAudioStream()");
        mChannelCount = channelCount;

        openStream();
    }

void SimpleMultiPlayer::teardownAudioStream() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "teardownAudioStream()");
    // tear down the player
    if (mAudioStream) {
        mAudioStream->stop();
        mAudioStream->close();
        mAudioStream.reset();
    }
}

void SimpleMultiPlayer::addSampleSource(SampleSource* source, SampleBuffer* buffer) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "+++ addSampleSource");
    buffer->resampleData(mSampleRate);

    mSampleBuffers.push_back(buffer);
    mSampleSources.push_back(source);
    mNumSampleBuffers++;
    __android_log_print(ANDROID_LOG_INFO, TAG, "+++ addSampleSource DONE");
}

void SimpleMultiPlayer::unloadSampleData() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "unloadSampleData()");
    resetAll();

    for (int32_t bufferIndex = 0; bufferIndex < mNumSampleBuffers; bufferIndex++) {
        delete mSampleBuffers[bufferIndex];
        delete mSampleSources[bufferIndex];
    }

    mSampleBuffers.clear();
    mSampleSources.clear();

    mNumSampleBuffers = 0;
}

void SimpleMultiPlayer::triggerDown(int32_t index) {
    LOGD("triggerDown");
    int32_t referenceSampleIndex = mSampleSources[0]->getCurrentSampleIndex();  //mSampleRate * 10;
    std::thread([this, referenceSampleIndex]() {
        for (int32_t i = 0; i < mNumSampleBuffers; ++i) {
            LOGD("totalLenght: trigger");
            mSampleSources[i]->setPlayMode(referenceSampleIndex);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
//        fadeGain(1.0f, 180);
        fadeGainToStored(180);
    }).detach();
}


void SimpleMultiPlayer::triggerUp(int32_t index) {
    LOGD("triggerUp");
    std::thread([this]() {
        fadeGain(0.0f, 180);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        for (int32_t i = 0; i < mNumSampleBuffers; ++i) {
            mSampleSources[i]->setStopMode();
        }
    }).detach();

}

    void SimpleMultiPlayer::fadeGain(float targetGain, int durationMs) {
        mFading.store(true);
        mFadeDurationMs.store(durationMs);

        // Store initial gains before fading to 0.0f
        if (targetGain == 0.0f) {
            mInitialGains.clear();
            for (int i = 0; i < mNumSampleBuffers; ++i) {
                mInitialGains.push_back(getGain(i));
            }
        }

        std::thread([this, targetGain]() {
            int steps = 300;
            int stepDuration = mFadeDurationMs.load() / steps;
            std::vector<float> initialGains(mNumSampleBuffers);

            for (int i = 0; i < mNumSampleBuffers; ++i) {
                initialGains[i] = getGain(i);
            }

            for (int step = 0; step < steps; ++step) {
                float progress = static_cast<float>(step) / steps;
                for (int i = 0; i < mNumSampleBuffers; ++i) {
                    float newGain = initialGains[i] + progress * (targetGain - initialGains[i]);
                    setGainFromFade(i, newGain);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(stepDuration));
            }

            for (int i = 0; i < mNumSampleBuffers; ++i) {
                setGainFromFade(i, targetGain);
            }
            mFading.store(false);
        }).detach();
    }

    void SimpleMultiPlayer::fadeGainToStored(int durationMs) {
        mFading.store(true);
        mFadeDurationMs.store(durationMs);

        std::thread([this]() {
            int steps = 300;
            int stepDuration = mFadeDurationMs.load() / steps;

            for (int step = 0; step < steps; ++step) {
                float progress = static_cast<float>(step) / steps;
                for (int i = 0; i < mNumSampleBuffers; ++i) {
                    float initialGain = (i < mInitialGains.size()) ? mInitialGains[i] : 1.0f;
                    float newGain = initialGain * progress;
                    setGainFromFade(i, newGain);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(stepDuration));
            }

            for (int i = 0; i < mNumSampleBuffers; ++i) {
                float initialGain = (i < mInitialGains.size()) ? mInitialGains[i] : 1.0f;
                setGainFromFade(i, initialGain);
            }
            mInitialGains.clear();
            mFading.store(false);
        }).detach();
    }



    void SimpleMultiPlayer::resetAll() {
    for (int32_t bufferIndex = 0; bufferIndex < mNumSampleBuffers; bufferIndex++) {
        mSampleSources[bufferIndex]->setStopMode();
    }
}

void SimpleMultiPlayer::setPan(int index, float pan) {
    mSampleSources[index]->setPan(pan);
}

float SimpleMultiPlayer::getPan(int index) {
    return mSampleSources[index]->getPan();
}

void SimpleMultiPlayer::setGain(int index, float gain) {
    // Check if the index is within bounds
    if (index < 0 || index >= mSampleSources.size()) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Index out of bounds: %d", index);
        return; // or handle the error appropriately
    }
    // Check if the element at the index is not null
    if (mSampleSources[index] == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Sample source at index %d is null", index);
        return; // or handle the error appropriately
    }
    mSampleSources[index]->setGain(gain);
    // Check if the index exists in mInitialGains and update it if so
    if (index < mInitialGains.size()) {
        mInitialGains[index] = gain;
    }
}

    void SimpleMultiPlayer::setGainFromFade(int index, float gain) {
        // Check if the index is within bounds
        if (index < 0 || index >= mSampleSources.size()) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "Index out of bounds: %d", index);
            return; // or handle the error appropriately
        }
        // Check if the element at the index is not null
        if (mSampleSources[index] == nullptr) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "Sample source at index %d is null", index);
            return; // or handle the error appropriately
        }
        mSampleSources[index]->setGain(gain);
    }

float SimpleMultiPlayer::getGain(int index) {
    return mSampleSources[index]->getGain();
}

int32_t SimpleMultiPlayer::getCurrentSampleIndex(int index) {
    return mSampleSources[index]->getCurrentSampleIndex();
}

float SimpleMultiPlayer::getCurrentTimeInSeconds(int index) {
    return mSampleSources[index]->getCurrentTimeInSeconds();
}

    void SimpleMultiPlayer::setCurrentTimeInSeconds(float newTime) {
        for (int32_t i = 0; i < mNumSampleBuffers; ++i) {
            mSampleSources[i]->setCurrentTimeInSeconds(newTime);
        }
    }

    float SimpleMultiPlayer::getTotalLengthInSeconds(int index) {
        return mSampleSources[index]->getTotalLengthInSeconds();
    }

    void SimpleMultiPlayer::setTempo(float tempo) {
        mCurrentTempo = tempo;
        for(int32_t index = 0; index < mNumSampleBuffers; index++) {
            mSampleSources[index]->setTempo(tempo);
        }
        __android_log_print(ANDROID_LOG_INFO, TAG, "Tempo set to: %f", tempo);
    }

    void SimpleMultiPlayer::setPitchSemiTones(float pitch) {
        mCurrentPitch = pitch;
        int32_t referenceSampleIndex = mSampleSources[0]->getCurrentTimeInSeconds() + 0.5;
        for(int32_t index = 0; index < mNumSampleBuffers; index++) {
//            mSampleSources[index]->setCurrentTimeInSeconds(referenceSampleIndex);
//            mSampleSources[index]->setPitchSemiTones(pitch);
        }
        __android_log_print(ANDROID_LOG_INFO, TAG, "Pitch set to: %f", pitch);
    }

    float SimpleMultiPlayer::getTempo() const {
        return mCurrentTempo;
    }

    float SimpleMultiPlayer::getPitchSemiTones() const {
        return mCurrentPitch;
    }


}

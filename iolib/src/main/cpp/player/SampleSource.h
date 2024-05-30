/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef _PLAYER_SAMPLESOURCE_
#define _PLAYER_SAMPLESOURCE_

#include <cstdint>
#include <android/log.h> // Include the Android logging header

#include "DataSource.h"

#include "SampleBuffer.h"

#include "SoundTouch.h"

#define LOG_TAG "SampleSource"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

namespace iolib {

/**
 * Defines an interface for audio data provided to a player object.
 * Concrete examples include OneShotSampleBuffer. One could imagine a LoopingSampleBuffer.
 * Supports stereo position via mPan member.
 */
class SampleSource: public DataSource {
public:
    // Pan position of the audio in a stereo mix
    // [left:-1.0f] <- [center: 0.0f] -> -[right: 1.0f]
    static constexpr float PAN_HARDLEFT = -1.0f;
    static constexpr float PAN_HARDRIGHT = 1.0f;
    static constexpr float PAN_CENTER = 0.0f;

    SampleSource(SampleBuffer *sampleBuffer, float pan)
     : mSampleBuffer(sampleBuffer), mCurSampleIndex(0), mIsPlaying(false), mGain(1.0f) {
        setPan(pan);
        mSoundTouch.setSampleRate(mSampleBuffer->getSampleRate());
        mSoundTouch.setChannels(mSampleBuffer->getChannelCount());
    }
    virtual ~SampleSource() {}

    void setTempo(float tempo) {
        mSoundTouch.setTempo(tempo);
    }

    void setPitchSemiTones(float pitch) {
        mSoundTouch.setPitchSemiTones(pitch);
    }

    //void setPlayMode() { mCurSampleIndex = 0; mIsPlaying = true; }
    void setPlayMode(int32_t sampleIndex) {
        // Adjust the sampleIndex for ClickTrack (Mono) because sampleIndex is take from the reference track which is stereo
        int32_t channelCount = mSampleBuffer->getChannelCount();
        if (channelCount == 1) {
            sampleIndex = sampleIndex / 2;
        }
        // Set the current sample index and playback flag.
        mSoundTouch.clear();
        mCurSampleIndex = sampleIndex;
        mIsPlaying = true;
    }
    void setStopMode() {
        mIsPlaying = false;
        mSoundTouch.clear();
    }

    bool isPlaying() { return mIsPlaying; }

    void setPan(float pan) {
        if (pan < PAN_HARDLEFT) {
            mPan = PAN_HARDLEFT;
        } else if (pan > PAN_HARDRIGHT) {
            mPan = PAN_HARDRIGHT;
        } else {
            mPan = pan;
        }
        calcGainFactors();
    }

    float getPan() {
        return mPan;
    }

    void setGain(float gain) {
        mGain = gain;
        calcGainFactors();
    }

    float getGain() {
        return mGain;
    }

    int32_t getCurrentSampleIndex() {
        return mCurSampleIndex;
    }

    void setCurrentSampleIndex(int32_t sampleIndex) {
        int32_t numSamples = mSampleBuffer->getNumSamples();
        int32_t maxSampleIndex = numSamples - 1;
        mSoundTouch.clear();
        if (sampleIndex < 0) {
            mCurSampleIndex = 0;
        } else if (sampleIndex > maxSampleIndex) {
            mCurSampleIndex = maxSampleIndex;
        } else {
            mCurSampleIndex = sampleIndex;
        }
    }

    void setCurrentTimeInSeconds(float seconds) {
        int32_t sampleRate = mSampleBuffer->getSampleRate() * mSampleBuffer->getChannelCount();
        if (sampleRate > 0) {
            int32_t sampleIndex = static_cast<int32_t>(seconds * sampleRate);
            setCurrentSampleIndex(sampleIndex);
        } else {
            LOGD("Invalid sample rate: %d", sampleRate);
        }
    }

    float getCurrentTimeInSeconds() {
        int32_t sampleRate = mSampleBuffer->getSampleRate() * mSampleBuffer->getChannelCount();
        if (sampleRate > 0) {
            return static_cast<float>(mCurSampleIndex) / sampleRate;
        } else {
            LOGD("Invalid sample rate: %d", sampleRate);
            return 0.0f;
        }
    }

    float getTotalLengthInSeconds() {
        int32_t sampleRate = mSampleBuffer->getSampleRate() * mSampleBuffer->getChannelCount();
        int32_t numSamples = mSampleBuffer->getNumSamples();
        if (sampleRate > 0) {
            return static_cast<float>(numSamples) / sampleRate;
        } else {
            LOGD("Invalid sample rate: %d", sampleRate);
            return 0.0f;
        }
    }




protected:
    SampleBuffer    *mSampleBuffer;

    int32_t mCurSampleIndex;

    bool mIsPlaying;

    // Logical pan value
    float mPan;

    // precomputed channel gains for pan
    float mLeftGain;
    float mRightGain;

    // Overall gain
    float mGain;

    soundtouch::SoundTouch mSoundTouch;


private:
    void calcGainFactors() {
        // useful panning information: http://www.cs.cmu.edu/~music/icm-online/readings/panlaws/
        float rightPan = (mPan * 0.5) + 0.5;
        mRightGain = rightPan * mGain;
        mLeftGain = (1.0 - rightPan) * mGain;    }
};

} // namespace wavlib

#endif //_PLAYER_SAMPLESOURCE_

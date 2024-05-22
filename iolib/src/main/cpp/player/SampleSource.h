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

#include <SoundTouch.h>
//#include <../rubberband/RubberBandStretcher.h>

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
        LOGD("mSampleBuffer->getSampleRate(): %d", mSampleBuffer->getSampleRate());
        mSoundTouch.setSampleRate(mSampleBuffer->getSampleRate());
        mSoundTouch.setChannels(mSampleBuffer->getChannelCount());
        //mSoundTouch.setPitch(0.8f);
        mSoundTouch.setTempo(0.7f); // changing the tempo in any way makes audio crack and delays start/stop times
        //mSoundTouch.setTempo(1.0f);
    }
    virtual ~SampleSource() {}

    //void setPlayMode() { mCurSampleIndex = 0; mIsPlaying = true; }
    void setPlayMode(int32_t sampleIndex) {
        // Ensure that the sample index is within the valid range.
        if (sampleIndex < 0) {
            sampleIndex = 0;
        }
        // Set the current sample index and playback flag.
        mCurSampleIndex = sampleIndex;
        mIsPlaying = true;
    }
    void setStopMode() { mIsPlaying = false; mCurSampleIndex = 0; }

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
        LOGD("Current Sample Index: %d", mCurSampleIndex);
        return mCurSampleIndex;
    }

    float getCurrentTimeInSeconds() {
        int32_t sampleRate = mSampleBuffer->getSampleRate() * 2;
        LOGD("sampleRate: %d", sampleRate);
        LOGD("mCurSampleIndex: %d", mCurSampleIndex);
        if (sampleRate > 0) {
            return static_cast<float>(mCurSampleIndex) / sampleRate;
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

    //float mSpeed;
    soundtouch::SoundTouch mSoundTouch;
    //RubberBand::RubberBandStretcher mRubberBandStretcher;

private:
    void calcGainFactors() {
        // useful panning information: http://www.cs.cmu.edu/~music/icm-online/readings/panlaws/
        float rightPan = (mPan * 0.5) + 0.5;
        mRightGain = rightPan * mGain;
        mLeftGain = (1.0 - rightPan) * mGain;    }
};

} // namespace wavlib

#endif //_PLAYER_SAMPLESOURCE_

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

#ifndef _PLAYER_SIMPLEMULTIPLAYER_H_
#define _PLAYER_SIMPLEMULTIPLAYER_H_

#include <vector>

#include <oboe/Oboe.h>
#include <stdint.h>

#include "OneShotSampleSource.h"
#include "SampleBuffer.h"

#include <SoundTouch.h>

#include <oboe/LatencyTuner.h>


namespace iolib {

/**
 * A simple streaming player for multiple SampleBuffers.
 */
class SimpleMultiPlayer  {
public:
    SimpleMultiPlayer();

    void setupAudioStream(int32_t channelCount);
    void teardownAudioStream();

    bool openStream();
    bool startStream();

    int getSampleRate() { return mSampleRate; }

    // Wave Sample Loading...
    /**
     * Adds the SampleSource/SampleBuffer pair to the list of source channels.
     * Transfers ownership of those objects so that they can be deleted/unloaded.
     * The indexes associated with each source channel is the order in which they
     * are added.
     */
    void addSampleSource(SampleSource* source, SampleBuffer* buffer);
    /**
     * Deallocates and deletes all added source/buffer (see addSampleSource()).
     */
    void unloadSampleData();

    void triggerDown(int32_t index);
    void triggerUp(int32_t index);

    void resetAll();

    bool getOutputReset() { return mOutputReset; }
    void clearOutputReset() { mOutputReset = false; }

    void setPan(int index, float pan);
    float getPan(int index);

    void setGain(int index, float gain);
    float getGain(int index);

    int32_t getCurrentSampleIndex(int index);
    float getCurrentTimeInSeconds(int index);
    void setCurrentTimeInSeconds(float newTime);
    float getTotalLengthInSeconds(int index);

    float mCurrentTempo = 1.0f;
    float mCurrentPitch = 0.0f;

    void setTempo(float tempo);
    void setPitchSemiTones(float pitch);

    float getTempo() const;
    float getPitchSemiTones() const;

    std::atomic<bool> mFading{false};
    std::atomic<int> mFadeDurationMs{500};

    void fadeGain(float targetGain, int durationMs);

    std::unique_ptr<oboe::LatencyTuner> mLatencyTuner;

    soundtouch::SoundTouch mSoundTouch;

// Sample Data
int32_t mNumSampleBuffers;
    std::vector<SampleSource*>  mSampleSources;
private:
    class MyDataCallback : public oboe::AudioStreamDataCallback {
    public:
        MyDataCallback(SimpleMultiPlayer *parent) : mParent(parent), mPreviousXRunCount(0) {}

        oboe::DataCallbackResult onAudioReady(
                oboe::AudioStream *audioStream,
                void *audioData,
                int32_t numFrames) override;

    private:
        SimpleMultiPlayer *mParent;
        int32_t mPreviousXRunCount;
    };

    class MyErrorCallback : public oboe::AudioStreamErrorCallback {
    public:
        MyErrorCallback(SimpleMultiPlayer *parent) : mParent(parent) {}

        virtual ~MyErrorCallback() {
        }

        void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) override;

    private:
        SimpleMultiPlayer *mParent;
    };

    // Oboe Audio Stream
    std::shared_ptr<oboe::AudioStream> mAudioStream;

    // Playback Audio attributes
    int32_t mChannelCount;
    int32_t mSampleRate;


    std::vector<SampleBuffer*>  mSampleBuffers;

    bool mOutputReset;

    std::shared_ptr<MyDataCallback> mDataCallback;
    std::shared_ptr<MyErrorCallback> mErrorCallback;

    bool stopFeeding;

};

}
#endif //_PLAYER_SIMPLEMULTIPLAYER_H_

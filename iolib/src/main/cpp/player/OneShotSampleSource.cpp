/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <string.h>
#include <vector>
#include <cmath>

#include "wav/WavStreamReader.h"

#include "OneShotSampleSource.h"

namespace iolib {

void OneShotSampleSource::mixAudio(float* outBuff, int numChannels, int32_t numFrames) {
    int32_t numSamples = mSampleBuffer->getNumSamples();
    int32_t sampleChannels = mSampleBuffer->getProperties().channelCount;
    int32_t samplesLeft = numSamples - mCurSampleIndex;
    int32_t numWriteFrames = mIsPlaying
                         ? std::min(numFrames, samplesLeft / sampleChannels)
                         : 0;

    if (numWriteFrames != 0) {

        int32_t adjustedWriteFrames = static_cast<int32_t>(std::round(numWriteFrames / mSoundTouch.getInputOutputSampleRatio()));

        // TODO make sure this is triggered for all samples sources otherwise they get out of sync! check if mSoundTouch.numSamples() can vary between the sample sources
        int32_t desiredSoundTouchBuffer = 4000;
        if (mSoundTouch.numSamples() < desiredSoundTouchBuffer) {
            int32_t framesToAdd = desiredSoundTouchBuffer - mSoundTouch.numSamples();
            if (framesToAdd < (samplesLeft / sampleChannels)) {
                adjustedWriteFrames = adjustedWriteFrames + framesToAdd;
                LOGD("added %d frames to the buffer", framesToAdd);
            }
        }

        if (mSoundTouch.numSamples() < numWriteFrames) {
            __android_log_print(ANDROID_LOG_ERROR, "OneShotSampleSource", "not enough frames from SoundTouch");
            int extraFrames = numWriteFrames - mSoundTouch.numSamples();
            LOGD("extraFrames: %d", extraFrames);
        }

        // Check if samplesLeft / sampleChannels is less than adjustedWriteFrames
        if ((samplesLeft / sampleChannels) < adjustedWriteFrames) {
            adjustedWriteFrames = samplesLeft / sampleChannels;
            LOGD("adjustedWriteFrames set to samplesLeft / sampleChannels: %d", adjustedWriteFrames);
        }

        const float* data  = mSampleBuffer->getSampleData();

        // Buffer to hold processed samples
        std::vector<float> processedSamples(numWriteFrames * sampleChannels);
        // Feed the required number of samples to SoundTouch
        mSoundTouch.putSamples(data + mCurSampleIndex, adjustedWriteFrames);
        // Calculate the actual number of processed frames
        mSoundTouch.receiveSamples(processedSamples.data(), numWriteFrames);


        if ((sampleChannels == 1) && (numChannels == 1)) {
            // MONO output from MONO samples
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++) {
                outBuff[frameIndex] += processedSamples[frameIndex] * mGain;
            }
        } else if ((sampleChannels == 1) && (numChannels == 2)) {
            // STEREO output from MONO samples
            int dstSampleIndex = 0;
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++) {
                outBuff[dstSampleIndex++] += processedSamples[frameIndex] * mLeftGain;
                outBuff[dstSampleIndex++] += processedSamples[frameIndex] * mRightGain;
            }
        } else if ((sampleChannels == 2) && (numChannels == 1)) {
            // MONO output from STEREO samples
            int dstSampleIndex = 0;
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++) {
                outBuff[dstSampleIndex++] += processedSamples[frameIndex * 2] * mLeftGain +
                                             processedSamples[frameIndex * 2 + 1] * mRightGain;
            }
        } else if ((sampleChannels == 2) && (numChannels == 2)) {
            // STEREO output from STEREO samples
            int dstSampleIndex = 0;
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++) {
                outBuff[dstSampleIndex++] += processedSamples[frameIndex * 2] * mLeftGain;
                outBuff[dstSampleIndex++] += processedSamples[frameIndex * 2 + 1] * mRightGain;
            }
        }

        mCurSampleIndex += adjustedWriteFrames * sampleChannels;

        if (mCurSampleIndex >= numSamples) {
            LOGD("Reached End Of Song: mCurSampleIndex = %d, numSamples = %d", mCurSampleIndex, numSamples);
            mIsPlaying = false;
        }

    }  else {
        LOGD("No frames to write.");
    }

    // silence
    // no need as the output buffer would need to have been filled with silence
    // to be mixed into
}

} // namespace wavlib

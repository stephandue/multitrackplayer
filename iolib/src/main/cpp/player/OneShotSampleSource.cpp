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
        const float* data  = mSampleBuffer->getSampleData();
        float adjustedSampleIndex = mCurSampleIndex;
        LOGD("mixAudio: Starting mix, mCurSampleIndex = %d, numWriteFrames = %d, mTempo = %f", mCurSampleIndex, numWriteFrames, mTempo);

        if ((sampleChannels == 1) && (numChannels == 1)) {
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++, adjustedSampleIndex += mTempo) {
                outBuff[frameIndex] += data[static_cast<int32_t>(adjustedSampleIndex)] * mGain;
            }
        } else if ((sampleChannels == 1) && (numChannels == 2)) {
            int dstSampleIndex = 0;
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++, adjustedSampleIndex += mTempo) {
                outBuff[dstSampleIndex++] += data[static_cast<int32_t>(adjustedSampleIndex)] * mLeftGain;
                outBuff[dstSampleIndex++] += data[static_cast<int32_t>(adjustedSampleIndex)] * mRightGain;
            }
        } else if ((sampleChannels == 2) && (numChannels == 1)) {
            int dstSampleIndex = 0;
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++, adjustedSampleIndex += mTempo) {
                outBuff[dstSampleIndex++] += data[static_cast<int32_t>(adjustedSampleIndex)] * mLeftGain +
                                             data[static_cast<int32_t>(adjustedSampleIndex)] * mRightGain;
            }
        } else if ((sampleChannels == 2) && (numChannels == 2)) {
            int dstSampleIndex = 0;
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++, adjustedSampleIndex += mTempo) {
                outBuff[dstSampleIndex++] += data[static_cast<int32_t>(adjustedSampleIndex)] * mLeftGain;
                outBuff[dstSampleIndex++] += data[static_cast<int32_t>(adjustedSampleIndex)] * mRightGain;
            }
        }

        mCurSampleIndex = static_cast<int32_t>(adjustedSampleIndex);

        LOGD("mixAudio: mCurSampleIndex = %d, numSamples = %d, numWriteFrames = %d", mCurSampleIndex, numSamples, numWriteFrames);

        if (mCurSampleIndex >= numSamples) {
            LOGD("Reached End Of Song: mCurSampleIndex = %d, numSamples = %d", mCurSampleIndex, numSamples);
            mIsPlaying = false;
        }
    } else {
        LOGD("No frames to write.");
    }

    // silence
    // no need as the output buffer would need to have been filled with silence
    // to be mixed into
}

} // namespace wavlib

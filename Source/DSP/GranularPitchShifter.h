#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class GranularPitchShifter
{
public:
    GranularPitchShifter() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        sr = sampleRate;
        bufferSize = static_cast<int>(sampleRate * 0.2); // 200ms buffer
        if (bufferSize < 100) bufferSize = 100;

        delayBuffer.setSize(2, bufferSize);
        delayBuffer.clear();

        writePos = 0;
        grainPhaseL = 0.0f;
        grainPhaseR = 0.0f;
    }

    void reset()
    {
        delayBuffer.clear();
        writePos = 0;
        grainPhaseL = 0.0f;
        grainPhaseR = 0.0f;
    }

    // Shift in semitones: -24.0 to +24.0
    void process(juce::AudioBuffer<float>& buffer, float semitones)
    {
        if (std::abs(semitones) < 0.01f || bufferSize <= 0)
            return; // No shift needed

        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        float pitchRatio = std::pow(2.0f, semitones / 12.0f);
        float grainSizeSamples = static_cast<float>(sr * 0.05); // 50ms grain size
        if (grainSizeSamples < 64.0f) grainSizeSamples = 64.0f;

        float speed = pitchRatio - 1.0f;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float inVal = buffer.getSample(ch, sample);
                delayBuffer.setSample(ch, writePos, inVal);

                float& grainPhase = (ch == 0) ? grainPhaseL : grainPhaseR;

                // Two overlapping grains with Hanning window
                float phase1 = grainPhase;
                float phase2 = std::fmod(grainPhase + 0.5f, 1.0f);

                float offset1 = phase1 * grainSizeSamples;
                float offset2 = phase2 * grainSizeSamples;

                float readPos1 = static_cast<float>(writePos) - offset1;
                while (readPos1 < 0.0f) readPos1 += static_cast<float>(bufferSize);

                float readPos2 = static_cast<float>(writePos) - offset2;
                while (readPos2 < 0.0f) readPos2 += static_cast<float>(bufferSize);

                // Interpolated reads
                float s1 = getInterpolatedSample(ch, readPos1);
                float s2 = getInterpolatedSample(ch, readPos2);

                // Hanning windows
                float w1 = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase1));
                float w2 = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase2));

                float outVal = (s1 * w1) + (s2 * w2);
                buffer.setSample(ch, sample, juce::jlimit(-2.0f, 2.0f, outVal));

                grainPhase += (speed / grainSizeSamples);
                if (grainPhase >= 1.0f) grainPhase -= 1.0f;
                if (grainPhase < 0.0f) grainPhase += 1.0f;
            }

            writePos = (writePos + 1) % bufferSize;
        }
    }

private:
    float getInterpolatedSample(int ch, float pos) const
    {
        if (bufferSize <= 0) return 0.0f;

        int i1 = static_cast<int>(pos);
        i1 = ((i1 % bufferSize) + bufferSize) % bufferSize;
        int i2 = (i1 + 1) % bufferSize;

        float frac = pos - std::floor(pos);

        float s1 = delayBuffer.getSample(ch, i1);
        float s2 = delayBuffer.getSample(ch, i2);
        return s1 + frac * (s2 - s1);
    }

    double sr { 44100.0 };
    juce::AudioBuffer<float> delayBuffer;
    int bufferSize { 8820 };
    int writePos { 0 };
    float grainPhaseL { 0.0f };
    float grainPhaseR { 0.0f };
};

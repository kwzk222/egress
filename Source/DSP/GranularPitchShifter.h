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

        // Speed at which grain phase advances per sample
        float speed = (pitchRatio - 1.0f) / grainSizeSamples;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float inVal = buffer.getSample(ch, sample);
                delayBuffer.setSample(ch, writePos, inVal);

                float grainPhase = (ch == 0) ? grainPhaseL : grainPhaseR;

                // Two overlapping grains offset by 0.5 (180 degrees)
                float phase1 = grainPhase;
                float phase2 = std::fmod(grainPhase + 0.5f, 1.0f);

                // Offset calculation: (1.0 - phase) * grainSize ensure read pointer moves at pitchRatio speed
                float offset1 = (1.0f - phase1) * grainSizeSamples;
                float offset2 = (1.0f - phase2) * grainSizeSamples;

                float readPos1 = static_cast<float>(writePos) - offset1;
                while (readPos1 < 0.0f) readPos1 += static_cast<float>(bufferSize);

                float readPos2 = static_cast<float>(writePos) - offset2;
                while (readPos2 < 0.0f) readPos2 += static_cast<float>(bufferSize);

                // Interpolated reads
                float s1 = getInterpolatedSample(ch, readPos1);
                float s2 = getInterpolatedSample(ch, readPos2);

                // Constant-power Hanning windowing
                float w1 = std::sin(juce::MathConstants<float>::pi * phase1);
                float w2 = std::sin(juce::MathConstants<float>::pi * phase2);
                w1 *= w1;
                w2 *= w2;

                float outVal = (s1 * w1) + (s2 * w2);
                buffer.setSample(ch, sample, std::tanh(outVal));
            }

            // Advance phases once per sample frame
            grainPhaseL += speed;
            while (grainPhaseL >= 1.0f) grainPhaseL -= 1.0f;
            while (grainPhaseL < 0.0f) grainPhaseL += 1.0f;

            grainPhaseR += speed;
            while (grainPhaseR >= 1.0f) grainPhaseR -= 1.0f;
            while (grainPhaseR < 0.0f) grainPhaseR += 1.0f;

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

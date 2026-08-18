#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

#include "ParametricEQ.h"
#include "GranularPitchShifter.h"

enum class ReverbAlgorithm
{
    RoomChamber,
    HallPlate,
    AmbientShimmer,
    NonLinearGated,
    BlackoutBlackhole
};

enum class ReverbEra
{
    Era1970s,
    Era1980s,
    Era2000s
};

// Canonical Dattorro Allpass Filter
class DattorroAllpass
{
public:
    void prepare(int delaySamples, float feedbackGain, double sampleRate)
    {
        sr = sampleRate;
        baseDelay = delaySamples;
        g = feedbackGain;
        bufferSize = delaySamples + 500;
        buffer.setSize(1, bufferSize);
        buffer.clear();
        writePos = 0;
    }

    void reset()
    {
        buffer.clear();
        writePos = 0;
    }

    float process(float in, float modOffsetSamples = 0.0f)
    {
        if (bufferSize <= 0) return in;

        float readPos = static_cast<float>(writePos) - static_cast<float>(baseDelay) + modOffsetSamples;
        while (readPos < 0.0f) readPos += static_cast<float>(bufferSize);

        int i1 = static_cast<int>(readPos) % bufferSize;
        int i2 = (i1 + 1) % bufferSize;
        float frac = readPos - std::floor(readPos);

        float bufOut = buffer.getSample(0, i1) + frac * (buffer.getSample(0, i2) - buffer.getSample(0, i1));

        // Canonical Dattorro Allpass Equations:
        // v[n] = x[n] + g * bufOut
        // buffer_write = v[n]
        // y[n] = -g * v[n] + bufOut
        float v = in + g * bufOut;
        buffer.setSample(0, writePos, v);
        float out = -g * v + bufOut;

        writePos = (writePos + 1) % bufferSize;
        return out;
    }

private:
    juce::AudioBuffer<float> buffer;
    int bufferSize { 1000 };
    int baseDelay { 500 };
    int writePos { 0 };
    float g { 0.5f };
    double sr { 44100.0 };
};

// Simple Modulated Delay Line
class DattorroDelay
{
public:
    void prepare(int delaySamples)
    {
        baseDelay = delaySamples;
        bufferSize = delaySamples + 500;
        buffer.setSize(1, bufferSize);
        buffer.clear();
        writePos = 0;
    }

    void reset()
    {
        buffer.clear();
        writePos = 0;
    }

    void write(float sample)
    {
        if (bufferSize > 0)
        {
            buffer.setSample(0, writePos, sample);
            writePos = (writePos + 1) % bufferSize;
        }
    }

    float readTap(int tapSamples) const
    {
        if (bufferSize <= 0) return 0.0f;
        int readPos = writePos - 1 - tapSamples;
        while (readPos < 0) readPos += bufferSize;
        return buffer.getSample(0, readPos % bufferSize);
    }

    float readModulated(float modOffsetSamples = 0.0f) const
    {
        if (bufferSize <= 0) return 0.0f;
        float readPos = static_cast<float>(writePos - 1) - static_cast<float>(baseDelay) + modOffsetSamples;
        while (readPos < 0.0f) readPos += static_cast<float>(bufferSize);

        int i1 = static_cast<int>(readPos) % bufferSize;
        int i2 = (i1 + 1) % bufferSize;
        float frac = readPos - std::floor(readPos);

        return buffer.getSample(0, i1) + frac * (buffer.getSample(0, i2) - buffer.getSample(0, i1));
    }

    int getBaseDelay() const { return baseDelay; }

private:
    juce::AudioBuffer<float> buffer;
    int bufferSize { 1000 };
    int baseDelay { 500 };
    int writePos { 0 };
};

class ReverbEngine
{
public:
    ReverbEngine();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    void setParams(ReverbAlgorithm algo, ReverbEra era, float decaySec, float size,
                   float preDelayMs, float diffHigh, float diffLow, float modRate,
                   float modDepth, bool preEQ);

    ParametricEQ& getEQ() { return eq; }

    void process(juce::AudioBuffer<float>& buffer);

private:
    double currentSampleRate { 44100.0 };

    ReverbAlgorithm currentAlgo { ReverbAlgorithm::HallPlate };
    ReverbEra currentEra { ReverbEra::Era2000s };
    float decaySeconds { 3.0f };
    float sizeParam { 1.0f };
    float preDelayTimeMs { 20.0f };
    float diffusionHigh { 0.7f };
    float diffusionLow { 0.7f };
    float lfoRate { 1.0f };
    float lfoDepth { 0.3f };
    bool isPreEQ { false };

    // Dattorro Input Allpass Diffusers (4 in series)
    DattorroAllpass inAllpass1;
    DattorroAllpass inAllpass2;
    DattorroAllpass inAllpass3;
    DattorroAllpass inAllpass4;

    // Dattorro Left Tank Half
    DattorroAllpass loopAllpassL1;
    DattorroDelay   loopDelayL1;
    DattorroAllpass loopAllpassL2;
    DattorroDelay   loopDelayL2;

    // Dattorro Right Tank Half
    DattorroAllpass loopAllpassR1;
    DattorroDelay   loopDelayR1;
    DattorroAllpass loopAllpassR2;
    DattorroDelay   loopDelayR2;

    // Lowpass Damping States
    float dampStateL { 0.0f };
    float dampStateR { 0.0f };

    // LFO phases for delay modulation
    float lfoPhase1 { 0.0f };
    float lfoPhase2 { 0.0f };

    // Pitch shifter for Shimmer algorithm
    GranularPitchShifter shimmerPitchShifter;

    // Pre-delay buffer
    juce::AudioBuffer<float> preDelayBuffer;
    int preDelayWritePos { 0 };

    // EQ
    ParametricEQ eq;

    // LFO phase for era chorus modulation
    float eraLfoPhase { 0.0f };

    void applyEraTone(juce::AudioBuffer<float>& buffer);
};

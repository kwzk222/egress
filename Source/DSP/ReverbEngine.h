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

// High-quality Dattorro / Schroeder Allpass Filter for diffusion without metallic comb resonance
class AllpassDiffuser
{
public:
    void prepare(int delaySamples, float feedbackGain)
    {
        bufferSize = delaySamples + 2;
        delayLen = delaySamples;
        g = feedbackGain;
        buffer.setSize(1, bufferSize);
        buffer.clear();
        writePos = 0;
    }

    void reset()
    {
        buffer.clear();
        writePos = 0;
    }

    float process(float in)
    {
        if (bufferSize <= 0) return in;

        int readPos = writePos - delayLen;
        while (readPos < 0) readPos += bufferSize;

        float bufOut = buffer.getSample(0, readPos);
        float out = -g * in + bufOut;
        buffer.setSample(0, writePos, in + g * out);

        writePos = (writePos + 1) % bufferSize;
        return out;
    }

private:
    juce::AudioBuffer<float> buffer;
    int bufferSize { 100 };
    int delayLen { 50 };
    int writePos { 0 };
    float g { 0.6f };
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

    // Input Diffuser Network (4 Cascaded Allpass Stages)
    AllpassDiffuser inDiffL[4];
    AllpassDiffuser inDiffR[4];

    // Decay Loop Delay Lines
    juce::AudioBuffer<float> loopBufferL;
    juce::AudioBuffer<float> loopBufferR;
    int loopWriteL { 0 };
    int loopWriteR { 0 };
    int loopSizeL { 44100 };
    int loopSizeR { 44100 };

    // Nested Loop Allpass Filters for maximum echo density & smoothness
    AllpassDiffuser loopDiffL[2];
    AllpassDiffuser loopDiffR[2];

    // Lowpass Damping Filters
    float dampStateL { 0.0f };
    float dampStateR { 0.0f };

    // Pitch shifter for Shimmer algorithm
    GranularPitchShifter shimmerPitchShifter;

    // Pre-delay buffer
    juce::AudioBuffer<float> preDelayBuffer;
    int preDelayWritePos { 0 };

    // EQ
    ParametricEQ eq;

    // LFO phase for modulation
    float lfoPhase { 0.0f };

    void applyEraTone(juce::AudioBuffer<float>& buffer);
};

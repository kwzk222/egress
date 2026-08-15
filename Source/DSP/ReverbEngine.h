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

    // JUCE Reverb underlying engine
    juce::Reverb juceReverbEngine;
    juce::Reverb::Parameters reverbParams;

    // Pitch shifter for Shimmer algorithm
    GranularPitchShifter shimmerPitchShifter;

    // Pre-delay buffer
    juce::AudioBuffer<float> preDelayBuffer;
    int preDelayWritePos { 0 };

    // EQ
    ParametricEQ eq;

    // LFO phase
    float lfoPhase { 0.0f };

    void applyEraTone(juce::AudioBuffer<float>& buffer);
};

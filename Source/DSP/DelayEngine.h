#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "ParametricEQ.h"
#include "GranularPitchShifter.h"

enum class DelayPanMode
{
    Stereo,
    PingPong,
    LCR,
    MidSide
};

enum class DelayCharacterModel
{
    StudioDigital,
    AnalogBBD,
    TapeEcho,
    OilCan,
    Digital80s
};

class DelayEngine
{
public:
    DelayEngine();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    void setParams(float delayTimeMs, float feedback, DelayPanMode panMode,
                   DelayCharacterModel model, bool reverse, float pitchSemitones,
                   float duckingAmount, float saturationDrive, bool preEQ);

    void setPatternStep(int stepIndex, float pan, bool gated);
    void updateTempo(double bpm);

    ParametricEQ& getEQ() { return eq; }

    void process(juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>& dryInput);

private:
    double currentSampleRate { 44100.0 };
    double currentBpm { 120.0 };

    // Parameters
    float targetDelayTimeMs { 350.0f };
    float currentDelayTimeMs { 350.0f };
    float feedbackLevel { 0.3f };
    DelayPanMode currentPanMode { DelayPanMode::Stereo };
    DelayCharacterModel currentModel { DelayCharacterModel::StudioDigital };
    bool reverseMode { false };
    float feedbackPitchShift { 0.0f };
    float duckingDb { 0.0f };
    float saturationDrive { 0.0f };
    bool isPreEQ { false };

    // Sequencer
    std::array<float, 16> patternPan { 0.0f };
    std::array<bool, 16> patternGate { true };
    int currentStep { 0 };
    int stepSampleCounter { 0 };

    // Delay buffer
    juce::AudioBuffer<float> delayBuffer;
    int writePos { 0 };
    int maxDelaySamples { 192000 }; // 4s at 48k

    // Pitch shifter & EQ
    GranularPitchShifter pitchShifter;
    ParametricEQ eq;

    // Saturation & Filters for Character Models
    juce::dsp::StateVariableFilter::Filter<float> bbdFilterL;
    juce::dsp::StateVariableFilter::Filter<float> bbdFilterR;
    float wowFlutterPhase { 0.0f };

    float applySaturation(float input, float drive) const;
    void applyCharacterModel(float& left, float& right);
};

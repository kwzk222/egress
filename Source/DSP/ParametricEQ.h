#pragma once

#include <JuceHeader.h>

enum class FilterType
{
    LowCut,
    HighCut,
    Bell,
    LowShelf,
    HighShelf,
    Notch
};

struct EQBandConfig
{
    FilterType type { FilterType::Bell };
    float frequency { 1000.0f };
    float q { 0.707f };
    float gainDb { 0.0f };
    int slopeDbPerOct { 12 }; // 12, 24, 36, 48
    bool enabled { true };
};

class ParametricEQ
{
public:
    ParametricEQ();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setBandConfig(int bandIndex, const EQBandConfig& config);
    void process(juce::AudioBuffer<float>& buffer);

    // Spectrum magnitude calculator for visualizer
    float getMagnitudeForFrequency(float frequency, double sampleRate) const;

private:
    static constexpr int numBands = 6;
    EQBandConfig bandConfigs[numBands];

    // Each band has up to 4 cascaded 2nd-order IIR filters (for up to 48 dB/oct slope)
    std::array<std::array<juce::dsp::IIR::Filter<float>, 4>, numBands> filtersL;
    std::array<std::array<juce::dsp::IIR::Filter<float>, 4>, numBands> filtersR;

    double currentSampleRate { 44100.0 };

    void updateBandCoefficients(int bandIndex);
};

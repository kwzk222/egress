#include "ParametricEQ.h"

ParametricEQ::ParametricEQ()
{
    for (int i = 0; i < numBands; ++i)
    {
        bandConfigs[i] = EQBandConfig();
    }
}

void ParametricEQ::prepare(const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;

    for (int b = 0; b < numBands; ++b)
    {
        for (int stage = 0; stage < 4; ++stage)
        {
            filtersL[b][stage].prepare(spec);
            filtersR[b][stage].prepare(spec);
            filtersL[b][stage].reset();
            filtersR[b][stage].reset();
        }
        updateBandCoefficients(b);
    }
}

void ParametricEQ::reset()
{
    for (int b = 0; b < numBands; ++b)
    {
        for (int stage = 0; stage < 4; ++stage)
        {
            filtersL[b][stage].reset();
            filtersR[b][stage].reset();
        }
    }
}

void ParametricEQ::setBandConfig(int bandIndex, const EQBandConfig& config)
{
    if (bandIndex < 0 || bandIndex >= numBands) return;
    bandConfigs[bandIndex] = config;
    updateBandCoefficients(bandIndex);
}

void ParametricEQ::updateBandCoefficients(int bandIndex)
{
    if (bandIndex < 0 || bandIndex >= numBands) return;

    const auto& config = bandConfigs[bandIndex];
    if (!config.enabled || currentSampleRate <= 0.0) return;

    int numStages = juce::jlimit(1, 4, config.slopeDbPerOct / 12);
    juce::dsp::IIR::Coefficients<float>::Ptr coeffs;

    float freq = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.49), config.frequency);
    float q = juce::jlimit(0.1f, 10.0f, config.q);
    float gain = juce::Decibels::decibelsToGain(config.gainDb);

    switch (config.type)
    {
        case FilterType::LowCut:
            coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, freq, q);
            break;
        case FilterType::HighCut:
            coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, freq, q);
            break;
        case FilterType::Bell:
            coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, freq, q, gain);
            break;
        case FilterType::LowShelf:
            coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(currentSampleRate, freq, q, gain);
            break;
        case FilterType::HighShelf:
            coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, freq, q, gain);
            break;
        case FilterType::Notch:
            coeffs = juce::dsp::IIR::Coefficients<float>::makeNotch(currentSampleRate, freq, q);
            break;
    }

    if (coeffs != nullptr)
    {
        for (int stage = 0; stage < numStages; ++stage)
        {
            filtersL[bandIndex][stage].coefficients = coeffs;
            filtersR[bandIndex][stage].coefficients = coeffs;
        }
    }
}

void ParametricEQ::process(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int b = 0; b < numBands; ++b)
    {
        if (!bandConfigs[b].enabled) continue;

        int numStages = juce::jlimit(1, 4, bandConfigs[b].slopeDbPerOct / 12);

        for (int stage = 0; stage < numStages; ++stage)
        {
            if (numChannels > 0)
            {
                juce::dsp::AudioBlock<float> blockL(buffer.getArrayOfWritePointers(), 1, numSamples);
                juce::dsp::ProcessContextReplacing<float> contextL(blockL);
                filtersL[b][stage].process(contextL);
            }
            if (numChannels > 1)
            {
                float* rightChannelData[] = { buffer.getWritePointer(1) };
                juce::dsp::AudioBlock<float> blockR(rightChannelData, 1, numSamples);
                juce::dsp::ProcessContextReplacing<float> contextR(blockR);
                filtersR[b][stage].process(contextR);
            }
        }
    }
}

float ParametricEQ::getMagnitudeForFrequency(float frequency, double sampleRate) const
{
    float totalMag = 1.0f;

    for (int b = 0; b < numBands; ++b)
    {
        if (!bandConfigs[b].enabled) continue;

        int numStages = juce::jlimit(1, 4, bandConfigs[b].slopeDbPerOct / 12);
        auto coeffs = filtersL[b][0].coefficients;

        if (coeffs != nullptr)
        {
            float freq = bandConfigs[b].frequency;
            float gainDb = bandConfigs[b].gainDb;
            float q = bandConfigs[b].q;

            // Direct analytical magnitude estimation
            float ratio = frequency / std::max(1.0f, freq);
            float bandMag = 1.0f;

            if (bandConfigs[b].type == FilterType::Bell)
            {
                float dist = std::abs(std::log2(ratio));
                bandMag = juce::Decibels::decibelsToGain(gainDb / (1.0f + dist * q * 2.0f));
            }
            else if (bandConfigs[b].type == FilterType::LowCut)
            {
                bandMag = 1.0f / std::sqrt(1.0f + std::pow(freq / std::max(1.0f, frequency), 2.0f * numStages));
            }
            else if (bandConfigs[b].type == FilterType::HighCut)
            {
                bandMag = 1.0f / std::sqrt(1.0f + std::pow(frequency / std::max(1.0f, freq), 2.0f * numStages));
            }

            totalMag *= bandMag;
        }
    }

    return totalMag;
}

#include "ReverbEngine.h"

ReverbEngine::ReverbEngine()
{
}

void ReverbEngine::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juceReverbEngine.setSampleRate(sampleRate);
    juceReverbEngine.reset();

    shimmerPitchShifter.prepare(sampleRate, samplesPerBlock);

    preDelayBuffer.setSize(2, static_cast<int>(sampleRate * 1.0)); // Up to 1s pre-delay
    preDelayBuffer.clear();
    preDelayWritePos = 0;

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1 };
    eq.prepare(spec);
}

void ReverbEngine::reset()
{
    juceReverbEngine.reset();
    shimmerPitchShifter.reset();
    preDelayBuffer.clear();
    preDelayWritePos = 0;
    eq.reset();
}

void ReverbEngine::setParams(ReverbAlgorithm algo, ReverbEra era, float decaySec, float size,
                           float preDelayMs, float diffHigh, float diffLow, float modRate,
                           float modDepth, bool preEQ)
{
    currentAlgo = algo;
    currentEra = era;
    decaySeconds = decaySec;
    sizeParam = size;
    preDelayTimeMs = preDelayMs;
    diffusionHigh = diffHigh;
    diffusionLow = diffLow;
    lfoRate = modRate;
    lfoDepth = modDepth;
    isPreEQ = preEQ;

    // Configure JUCE reverb parameters cleanly.
    // roomSize controls comb filter delay lengths (physical room size: 0.5 to 0.98).
    // decaySec scales damping and wet feedback.
    float roomSizeValue = juce::jlimit(0.3f, 0.98f, 0.4f + sizeParam * 0.55f);
    float dampingValue = juce::jlimit(0.05f, 0.95f, 1.0f - diffHigh);

    reverbParams.roomSize = roomSizeValue;
    reverbParams.damping = dampingValue;
    reverbParams.wetLevel = 0.8f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = juce::jlimit(0.1f, 1.0f, sizeParam);
    reverbParams.freezeMode = (decaySec >= 59.0f) ? 1.0f : 0.0f;

    juceReverbEngine.setParameters(reverbParams);
}

void ReverbEngine::applyEraTone(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    switch (currentEra)
    {
        case ReverbEra::Era1970s:
            // Dark, lo-fi roll-off
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer(ch);
                for (int s = 0; s < numSamples; ++s)
                {
                    data[s] *= 0.85f; // Soft attenuation
                }
            }
            break;

        case ReverbEra::Era1980s:
        {
            // Sample-accurate smooth bright chorus modulation
            float phaseInc = (juce::MathConstants<float>::twoPi * lfoRate) / static_cast<float>(currentSampleRate);
            for (int s = 0; s < numSamples; ++s)
            {
                lfoPhase += phaseInc;
                if (lfoPhase > juce::MathConstants<float>::twoPi) lfoPhase -= juce::MathConstants<float>::twoPi;

                float mod = 1.0f + std::sin(lfoPhase) * lfoDepth * 0.05f;
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    buffer.setSample(ch, s, buffer.getSample(ch, s) * mod);
                }
            }
            break;
        }

        case ReverbEra::Era2000s:
            // Pristine, no tone alterations
            break;
    }
}

void ReverbEngine::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels < 2) return;

    if (isPreEQ)
    {
        eq.process(buffer);
    }

    // Pre-delay buffer processing
    int preDelaySamples = static_cast<int>(preDelayTimeMs * 0.001 * currentSampleRate);
    int bufferSize = preDelayBuffer.getNumSamples();

    if (preDelaySamples > 0 && bufferSize > 0)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            int readPos = preDelayWritePos - preDelaySamples;
            while (readPos < 0) readPos += bufferSize;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float inSample = buffer.getSample(ch, s);
                preDelayBuffer.setSample(ch, preDelayWritePos, inSample);
                buffer.setSample(ch, s, preDelayBuffer.getSample(ch, readPos));
            }
            preDelayWritePos = (preDelayWritePos + 1) % bufferSize;
        }
    }

    // Core algorithmic reverb process
    if (numChannels == 2)
    {
        juceReverbEngine.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), numSamples);
    }

    // Shimmer Pitch Shifting in feedback for AmbientShimmer algorithm (+12 semitones / octave up)
    if (currentAlgo == ReverbAlgorithm::AmbientShimmer)
    {
        shimmerPitchShifter.process(buffer, 12.0f);
    }
    else if (currentAlgo == ReverbAlgorithm::BlackoutBlackhole)
    {
        shimmerPitchShifter.process(buffer, -12.0f); // Octave down deep ambient
    }

    applyEraTone(buffer);

    // Soft limiting and NaN sanitization
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int s = 0; s < numSamples; ++s)
        {
            if (std::isnan(data[s]) || std::isinf(data[s]))
            {
                data[s] = 0.0f;
            }
            else
            {
                data[s] = std::tanh(data[s] * 0.8f);
            }
        }
    }

    if (!isPreEQ)
    {
        eq.process(buffer);
    }
}

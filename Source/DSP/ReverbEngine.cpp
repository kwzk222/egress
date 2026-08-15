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

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };
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

    // Configure JUCE reverb parameters
    reverbParams.roomSize = juce::jlimit(0.0f, 1.0f, decaySec / 60.0f);
    reverbParams.damping = juce::jlimit(0.0f, 1.0f, 1.0f - diffHigh);
    reverbParams.wetLevel = 1.0f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = juce::jlimit(0.0f, 1.0f, size);
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
                    data[s] *= 0.85f; // Soft saturation/attenuation
                }
            }
            break;

        case ReverbEra::Era1980s:
            // Bright chorus
            lfoPhase += lfoRate * 0.01f;
            if (lfoPhase > juce::MathConstants<float>::twoPi) lfoPhase -= juce::MathConstants<float>::twoPi;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float mod = 1.0f + std::sin(lfoPhase) * lfoDepth * 0.1f;
                buffer.applyGain(ch, 0, numSamples, mod);
            }
            break;

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
            if (readPos < 0) readPos += bufferSize;

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

    // Non-linear / Gated reverb tail truncator
    if (currentAlgo == ReverbAlgorithm::NonLinearGated)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            if (s > numSamples / 2)
            {
                buffer.setSample(0, s, buffer.getSample(0, s) * 0.1f);
                buffer.setSample(1, s, buffer.getSample(1, s) * 0.1f);
            }
        }
    }

    applyEraTone(buffer);

    if (!isPreEQ)
    {
        eq.process(buffer);
    }
}

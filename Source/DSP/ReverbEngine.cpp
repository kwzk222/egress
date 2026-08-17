#include "ReverbEngine.h"

ReverbEngine::ReverbEngine()
{
}

void ReverbEngine::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Prepare Dattorro input allpass diffusers with prime delay lengths
    int diffDelaysL[4] = { 142, 107, 379, 277 };
    int diffDelaysR[4] = { 149, 113, 389, 281 };

    for (int i = 0; i < 4; ++i)
    {
        inDiffL[i].prepare(static_cast<int>(diffDelaysL[i] * sampleRate / 44100.0), 0.65f);
        inDiffR[i].prepare(static_cast<int>(diffDelaysR[i] * sampleRate / 44100.0), 0.65f);
    }

    // Prepare loop delays (approx 150ms to 200ms prime lengths)
    loopSizeL = static_cast<int>(sampleRate * 0.142);
    loopSizeR = static_cast<int>(sampleRate * 0.163);

    loopBufferL.setSize(1, loopSizeL + 10);
    loopBufferR.setSize(1, loopSizeR + 10);
    loopBufferL.clear();
    loopBufferR.clear();
    loopWriteL = 0;
    loopWriteR = 0;

    // Prepare nested loop allpasses
    loopDiffL[0].prepare(static_cast<int>(672 * sampleRate / 44100.0), 0.5f);
    loopDiffL[1].prepare(static_cast<int>(1800 * sampleRate / 44100.0), 0.5f);

    loopDiffR[0].prepare(static_cast<int>(908 * sampleRate / 44100.0), 0.5f);
    loopDiffR[1].prepare(static_cast<int>(2656 * sampleRate / 44100.0), 0.5f);

    dampStateL = 0.0f;
    dampStateR = 0.0f;

    shimmerPitchShifter.prepare(sampleRate, samplesPerBlock);

    preDelayBuffer.setSize(2, static_cast<int>(sampleRate * 1.0)); // Up to 1s pre-delay
    preDelayBuffer.clear();
    preDelayWritePos = 0;

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1 };
    eq.prepare(spec);
}

void ReverbEngine::reset()
{
    for (int i = 0; i < 4; ++i)
    {
        inDiffL[i].reset();
        inDiffR[i].reset();
    }

    loopBufferL.clear();
    loopBufferR.clear();
    loopWriteL = 0;
    loopWriteR = 0;

    for (int i = 0; i < 2; ++i)
    {
        loopDiffL[i].reset();
        loopDiffR[i].reset();
    }

    dampStateL = 0.0f;
    dampStateR = 0.0f;

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
}

void ReverbEngine::applyEraTone(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    switch (currentEra)
    {
        case ReverbEra::Era1970s:
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer(ch);
                for (int s = 0; s < numSamples; ++s)
                {
                    data[s] *= 0.85f;
                }
            }
            break;

        case ReverbEra::Era1980s:
        {
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
    int pBufSize = preDelayBuffer.getNumSamples();

    if (preDelaySamples > 0 && pBufSize > 0)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            int readPos = preDelayWritePos - preDelaySamples;
            while (readPos < 0) readPos += pBufSize;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float inSample = buffer.getSample(ch, s);
                preDelayBuffer.setSample(ch, preDelayWritePos, inSample);
                buffer.setSample(ch, s, preDelayBuffer.getSample(ch, readPos));
            }
            preDelayWritePos = (preDelayWritePos + 1) % pBufSize;
        }
    }

    // Calculate feedback decay gain based on decaySeconds parameter
    // RT60 = -3 * T_loop / log10(g_fb) => g_fb = 10^(-3 * T_loop / RT60)
    float tLoop = 0.15f; // Average loop delay
    float decayFactor = std::pow(10.0f, (-3.0f * tLoop) / std::max(0.1f, decaySeconds));
    decayFactor = juce::jlimit(0.10f, 0.94f, decayFactor); // Strictly capped < 0.95 to prevent runaway

    float dampCoeff = juce::jlimit(0.1f, 0.8f, diffusionHigh);

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = buffer.getSample(0, s);
        float inR = buffer.getSample(1, s);

        // 1. Pass input through cascaded Allpass diffusers (diffuses sharp transients into smooth noise)
        float diffL = inL;
        float diffR = inR;
        for (int i = 0; i < 4; ++i)
        {
            diffL = inDiffL[i].process(diffL);
            diffR = inDiffR[i].process(diffR);
        }

        // 2. Read from delay loop
        int readL = (loopWriteL - loopSizeL + loopSizeL + 10) % loopSizeL;
        int readR = (loopWriteR - loopSizeR + loopSizeR + 10) % loopSizeR;

        float nodeL = loopBufferL.getSample(0, readL);
        float nodeR = loopBufferR.getSample(0, readR);

        // Cross-feedback tank matrix
        float nextL = diffL + nodeR * decayFactor;
        float nextR = diffR + nodeL * decayFactor;

        // Nested Allpass diffusion inside loop
        nextL = loopDiffL[0].process(nextL);
        nextL = loopDiffL[1].process(nextL);

        nextR = loopDiffR[0].process(nextR);
        nextR = loopDiffR[1].process(nextR);

        // Damping lowpass
        dampStateL = dampStateL * dampCoeff + nextL * (1.0f - dampCoeff);
        dampStateR = dampStateR * dampCoeff + nextR * (1.0f - dampCoeff);

        // Write back to loop buffer with soft clipping
        loopBufferL.setSample(0, loopWriteL, std::tanh(dampStateL));
        loopBufferR.setSample(0, loopWriteR, std::tanh(dampStateR));

        loopWriteL = (loopWriteL + 1) % loopSizeL;
        loopWriteR = (loopWriteR + 1) % loopSizeR;

        // Output lush diffused reverb tail
        buffer.setSample(0, s, dampStateL * 0.5f);
        buffer.setSample(1, s, dampStateR * 0.5f);
    }

    // Shimmer Pitch Shifting ONLY for AmbientShimmer algorithm
    if (currentAlgo == ReverbAlgorithm::AmbientShimmer)
    {
        shimmerPitchShifter.process(buffer, 12.0f);
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
                data[s] = juce::jlimit(-1.0f, 1.0f, data[s]);
            }
        }
    }

    if (!isPreEQ)
    {
        eq.process(buffer);
    }
}

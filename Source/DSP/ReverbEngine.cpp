#include "ReverbEngine.h"

ReverbEngine::ReverbEngine()
{
}

void ReverbEngine::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Scale delay lengths proportional to sample rate
    float srScale = static_cast<float>(sampleRate / 44100.0);

    // 1. Prepare input allpass diffusers
    int diffDelaysL[4] = { 142, 107, 379, 277 };
    int diffDelaysR[4] = { 149, 113, 389, 281 };

    for (int i = 0; i < 4; ++i)
    {
        inDiffL[i].prepare(static_cast<int>(diffDelaysL[i] * srScale), 0.65f);
        inDiffR[i].prepare(static_cast<int>(diffDelaysR[i] * srScale), 0.65f);
    }

    // 2. Prepare modulated figure-of-8 delay lines
    loopLenL1 = static_cast<int>(1944 * srScale);
    loopLenL2 = static_cast<int>(1375 * srScale);
    loopLenR1 = static_cast<int>(1830 * srScale);
    loopLenR2 = static_cast<int>(1574 * srScale);

    loopBufferL1.setSize(1, loopLenL1 + 200);
    loopBufferL2.setSize(1, loopLenL2 + 200);
    loopBufferR1.setSize(1, loopLenR1 + 200);
    loopBufferR2.setSize(1, loopLenR2 + 200);

    loopBufferL1.clear();
    loopBufferL2.clear();
    loopBufferR1.clear();
    loopBufferR2.clear();

    writeL1 = writeL2 = writeR1 = writeR2 = 0;
    lfoPhase1 = lfoPhase2 = 0.0f;
    dampStateL = dampStateR = 0.0f;

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

    loopBufferL1.clear();
    loopBufferL2.clear();
    loopBufferR1.clear();
    loopBufferR2.clear();

    writeL1 = writeL2 = writeR1 = writeR2 = 0;
    lfoPhase1 = lfoPhase2 = 0.0f;
    dampStateL = dampStateR = 0.0f;

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

float ReverbEngine::readInterpolatedModulated(const juce::AudioBuffer<float>& buf, int writePos, int baseLen, float modOffset) const
{
    int bSize = buf.getNumSamples();
    if (bSize <= 0) return 0.0f;

    float readPos = static_cast<float>(writePos) - static_cast<float>(baseLen) + modOffset;
    while (readPos < 0.0f) readPos += static_cast<float>(bSize);

    int i1 = static_cast<int>(readPos) % bSize;
    int i2 = (i1 + 1) % bSize;
    float frac = readPos - std::floor(readPos);

    float s1 = buf.getSample(0, i1);
    float s2 = buf.getSample(0, i2);
    return s1 + frac * (s2 - s1);
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
                eraLfoPhase += phaseInc;
                if (eraLfoPhase > juce::MathConstants<float>::twoPi) eraLfoPhase -= juce::MathConstants<float>::twoPi;

                float mod = 1.0f + std::sin(eraLfoPhase) * lfoDepth * 0.05f;
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
    float tLoop = 0.12f; // Average loop delay (~120ms)
    float decayFactor = std::pow(10.0f, (-3.0f * tLoop) / std::max(0.1f, decaySeconds));
    decayFactor = juce::jlimit(0.10f, 0.90f, decayFactor); // Strictly capped < 0.92 to prevent runaway

    float dampCoeff = juce::jlimit(0.05f, 0.70f, 1.0f - diffusionHigh);
    float lfoDepthSamples = 8.0f * lfoDepth;

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = buffer.getSample(0, s);
        float inR = buffer.getSample(1, s);

        // 1. Pass input through 4 cascaded Allpass diffusers (smears sharp transients)
        float diffL = inL;
        float diffR = inR;
        for (int i = 0; i < 4; ++i)
        {
            diffL = inDiffL[i].process(diffL);
            diffR = inDiffR[i].process(diffR);
        }

        // 2. Advance LFOs for delay modulation (~0.7 Hz and ~1.1 Hz)
        lfoPhase1 += 0.7f / static_cast<float>(currentSampleRate);
        if (lfoPhase1 >= 1.0f) lfoPhase1 -= 1.0f;

        lfoPhase2 += 1.1f / static_cast<float>(currentSampleRate);
        if (lfoPhase2 >= 1.0f) lfoPhase2 -= 1.0f;

        float mod1 = std::sin(lfoPhase1 * juce::MathConstants<float>::twoPi) * lfoDepthSamples;
        float mod2 = std::cos(lfoPhase2 * juce::MathConstants<float>::twoPi) * lfoDepthSamples;

        // 3. Read interpolated modulated samples from figure-of-8 delay lines
        float nodeL1 = readInterpolatedModulated(loopBufferL1, writeL1, loopLenL1, mod1);
        float nodeR1 = readInterpolatedModulated(loopBufferR1, writeR1, loopLenR1, mod2);

        // 4. Cross feedback matrix
        float nextL = diffL + nodeR1 * decayFactor;
        float nextR = diffR + nodeL1 * decayFactor;

        // 5. Damping lowpass filters (smoothes metallic high frequencies)
        dampStateL = dampStateL * dampCoeff + nextL * (1.0f - dampCoeff);
        dampStateR = dampStateR * dampCoeff + nextR * (1.0f - dampCoeff);

        // 6. Write back into loop buffers with soft clipping
        loopBufferL1.setSample(0, writeL1, std::tanh(dampStateL));
        loopBufferR1.setSample(0, writeR1, std::tanh(dampStateR));

        writeL1 = (writeL1 + 1) % loopBufferL1.getNumSamples();
        writeR1 = (writeR1 + 1) % loopBufferR1.getNumSamples();

        // Output lush, smooth, non-metallic reverb tail
        buffer.setSample(0, s, dampStateL * 0.4f);
        buffer.setSample(1, s, dampStateR * 0.4f);
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

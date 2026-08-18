#include "ReverbEngine.h"

ReverbEngine::ReverbEngine()
{
}

void ReverbEngine::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    float srScale = static_cast<float>(sampleRate / 44100.0);

    // Dattorro input diffuser delays (142, 107, 379, 277 samples at 44.1k)
    inAllpass1.prepare(static_cast<int>(142 * srScale), 0.75f, sampleRate);
    inAllpass2.prepare(static_cast<int>(107 * srScale), 0.75f, sampleRate);
    inAllpass3.prepare(static_cast<int>(379 * srScale), 0.625f, sampleRate);
    inAllpass4.prepare(static_cast<int>(277 * srScale), 0.625f, sampleRate);

    // Dattorro Left Tank Half delays (allpass 672 & 1800, delays 4453 & 3720)
    loopAllpassL1.prepare(static_cast<int>(672 * srScale), -0.70f, sampleRate);
    loopDelayL1.prepare(static_cast<int>(4453 * srScale));
    loopAllpassL2.prepare(static_cast<int>(1800 * srScale), 0.50f, sampleRate);
    loopDelayL2.prepare(static_cast<int>(3720 * srScale));

    // Dattorro Right Tank Half delays (allpass 908 & 2656, delays 4217 & 3163)
    loopAllpassR1.prepare(static_cast<int>(908 * srScale), -0.70f, sampleRate);
    loopDelayR1.prepare(static_cast<int>(4217 * srScale));
    loopAllpassR2.prepare(static_cast<int>(2656 * srScale), 0.50f, sampleRate);
    loopDelayR2.prepare(static_cast<int>(3163 * srScale));

    dampStateL = 0.0f;
    dampStateR = 0.0f;
    lfoPhase1 = 0.0f;
    lfoPhase2 = 0.0f;

    shimmerPitchShifter.prepare(sampleRate, samplesPerBlock);

    preDelayBuffer.setSize(2, static_cast<int>(sampleRate * 1.0)); // Up to 1s pre-delay
    preDelayBuffer.clear();
    preDelayWritePos = 0;

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1 };
    eq.prepare(spec);
}

void ReverbEngine::reset()
{
    inAllpass1.reset();
    inAllpass2.reset();
    inAllpass3.reset();
    inAllpass4.reset();

    loopAllpassL1.reset();
    loopDelayL1.reset();
    loopAllpassL2.reset();
    loopDelayL2.reset();

    loopAllpassR1.reset();
    loopDelayR1.reset();
    loopAllpassR2.reset();
    loopDelayR2.reset();

    dampStateL = 0.0f;
    dampStateR = 0.0f;
    lfoPhase1 = 0.0f;
    lfoPhase2 = 0.0f;

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

    // Decay gain mapping based on decaySeconds (RT60 decay time equation)
    float tLoop = 0.18f; // ~180ms loop length
    float decayFactor = std::pow(10.0f, (-3.0f * tLoop) / std::max(0.1f, decaySeconds));
    decayFactor = juce::jlimit(0.10f, 0.88f, decayFactor); // Capped < 0.90 to guarantee stability

    float dampCoeff = juce::jlimit(0.05f, 0.60f, 1.0f - diffusionHigh);

    // LFO phase increments (~0.7 Hz and ~1.1 Hz)
    float lfoInc1 = (0.7f * lfoRate) / static_cast<float>(currentSampleRate);
    float lfoInc2 = (1.1f * lfoRate) / static_cast<float>(currentSampleRate);
    float maxModSamples = 8.0f * lfoDepth;

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = buffer.getSample(0, s);
        float inR = buffer.getSample(1, s);
        float inputMono = (inL + inR) * 0.5f;

        // 1. Pass input through 4 series Dattorro input diffusers
        float diff = inAllpass1.process(inputMono);
        diff = inAllpass2.process(diff);
        diff = inAllpass3.process(diff);
        diff = inAllpass4.process(diff);

        // 2. Advance LFOs for smooth allpass delay modulation
        lfoPhase1 += lfoInc1;
        if (lfoPhase1 >= 1.0f) lfoPhase1 -= 1.0f;
        lfoPhase2 += lfoInc2;
        if (lfoPhase2 >= 1.0f) lfoPhase2 -= 1.0f;

        float mod1 = std::sin(lfoPhase1 * juce::MathConstants<float>::twoPi) * maxModSamples;
        float mod2 = std::cos(lfoPhase2 * juce::MathConstants<float>::twoPi) * maxModSamples;

        // 3. Process Dattorro Left Tank Half
        // Tank Left Input = diff + (Right Delay 2 output * decayFactor)
        float nodeR2 = loopDelayR2.readModulated(0.0f);
        float tankInL = diff + nodeR2 * decayFactor;

        float apL1 = loopAllpassL1.process(tankInL, mod1);
        loopDelayL1.write(apL1);

        float delL1 = loopDelayL1.readModulated(0.0f);

        // Lowpass damping
        dampStateL = dampStateL * dampCoeff + delL1 * (1.0f - dampCoeff);

        float apL2 = loopAllpassL2.process(dampStateL, 0.0f);
        loopDelayL2.write(apL2);

        // 4. Process Dattorro Right Tank Half
        // Tank Right Input = diff + (Left Delay 2 output * decayFactor)
        float nodeL2 = loopDelayL2.readModulated(0.0f);
        float tankInR = diff + nodeL2 * decayFactor;

        float apR1 = loopAllpassR1.process(tankInR, mod2);
        loopDelayR1.write(apR1);

        float delR1 = loopDelayR1.readModulated(0.0f);

        // Lowpass damping
        dampStateR = dampStateR * dampCoeff + delR1 * (1.0f - dampCoeff);

        float apR2 = loopAllpassR2.process(dampStateR, 0.0f);
        loopDelayR2.write(apR2);

        // 5. Output Taps (Multi-tap Dattorro stereo summation for dense, lush, non-pitched reverberation)
        float outL = loopDelayL1.readTap(266) + loopDelayL1.readTap(2974) - loopAllpassL2.process(0.0f) + loopDelayL2.readTap(1913) - loopDelayR1.readTap(1996) - loopAllpassR1.process(0.0f) - loopDelayR2.readTap(198);
        float outR = loopDelayR1.readTap(353) + loopDelayR1.readTap(3627) - loopAllpassR2.process(0.0f) + loopDelayR2.readTap(1228) - loopDelayL1.readTap(2673) - loopAllpassL1.process(0.0f) - loopDelayL2.readTap(335);

        buffer.setSample(0, s, std::tanh(outL * 0.35f));
        buffer.setSample(1, s, std::tanh(outR * 0.35f));
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

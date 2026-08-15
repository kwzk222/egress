#include "DelayEngine.h"

DelayEngine::DelayEngine()
{
    for (int i = 0; i < 16; ++i)
    {
        patternPan[i] = 0.0f; // Center
        patternGate[i] = true;
    }
}

void DelayEngine::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    maxDelaySamples = static_cast<int>(sampleRate * 10.0); // Up to 10 seconds buffer

    delayBuffer.setSize(2, maxDelaySamples);
    delayBuffer.clear();
    writePos = 0;

    pitchShifter.prepare(sampleRate, samplesPerBlock);

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };
    eq.prepare(spec);

    bbdFilterL.prepare(spec);
    bbdFilterR.prepare(spec);
    bbdFilterL.setType(juce::dsp::StateVariableFilter::Parameters<float>::Type::lowPass);
    bbdFilterR.setType(juce::dsp::StateVariableFilter::Parameters<float>::Type::lowPass);
    bbdFilterL.setCutoffFrequency(2500.0f);
    bbdFilterR.setCutoffFrequency(2500.0f);
}

void DelayEngine::reset()
{
    delayBuffer.clear();
    writePos = 0;
    pitchShifter.reset();
    eq.reset();
    bbdFilterL.reset();
    bbdFilterR.reset();
}

void DelayEngine::setParams(float delayTimeMs, float feedback, DelayPanMode panMode,
                           DelayCharacterModel model, bool reverse, float pitchSemitones,
                           float duckingAmount, float satDrive, bool preEQ)
{
    targetDelayTimeMs = delayTimeMs;
    feedbackLevel = juce::jlimit(0.0f, 1.2f, feedback); // Allow super-infinity/freeze
    currentPanMode = panMode;
    currentModel = model;
    reverseMode = reverse;
    feedbackPitchShift = pitchSemitones;
    duckingDb = duckingAmount;
    saturationDrive = satDrive;
    isPreEQ = preEQ;
}

void DelayEngine::setPatternStep(int stepIndex, float pan, bool gated)
{
    if (stepIndex >= 0 && stepIndex < 16)
    {
        patternPan[stepIndex] = juce::jlimit(-1.0f, 1.0f, pan);
        patternGate[stepIndex] = gated;
    }
}

void DelayEngine::updateTempo(double bpm)
{
    if (bpm > 20.0 && bpm < 300.0) currentBpm = bpm;
}

float DelayEngine::applySaturation(float input, float drive) const
{
    if (drive <= 0.001f) return input;
    float driven = input * (1.0f + drive * 3.0f);
    return std::tanh(driven) / (1.0f + drive * 0.5f);
}

void DelayEngine::applyCharacterModel(float& left, float& right)
{
    switch (currentModel)
    {
        case DelayCharacterModel::StudioDigital:
            // Pristine, clean
            break;

        case DelayCharacterModel::AnalogBBD:
        {
            // BBD darkening & clock noise / mild saturation
            left = bbdFilterL.processSample(0, left);
            right = bbdFilterR.processSample(0, right);
            left = applySaturation(left, 0.2f);
            right = applySaturation(right, 0.2f);
            break;
        }

        case DelayCharacterModel::TapeEcho:
        {
            // Wow & flutter (pitch modulation) + tape saturation
            wowFlutterPhase += 0.05f;
            if (wowFlutterPhase > juce::MathConstants<float>::twoPi) wowFlutterPhase -= juce::MathConstants<float>::twoPi;
            float flutter = std::sin(wowFlutterPhase) * 0.002f;
            left = applySaturation(left, 0.4f + flutter);
            right = applySaturation(right, 0.4f - flutter);
            break;
        }

        case DelayCharacterModel::OilCan:
        {
            // Metallic resonance/oil smooth
            left = applySaturation(left, 0.1f) * 0.95f;
            right = applySaturation(right, 0.1f) * 0.95f;
            break;
        }

        case DelayCharacterModel::Digital80s:
        {
            // Bitcrush / lo-fi aliasing
            float crush = 1024.0f; // 10-bit simulation
            left = std::round(left * crush) / crush;
            right = std::round(right * crush) / crush;
            break;
        }
    }
}

void DelayEngine::process(juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>& dryInput)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels < 2) return;

    if (isPreEQ)
    {
        eq.process(buffer);
    }

    // Ducking envelope follower calculation on dryInput
    float dryRms = dryInput.getRMSLevel(0, 0, numSamples);
    float duckGain = 1.0f - juce::jlimit(0.0f, 1.0f, (dryRms * duckingDb / 12.0f));

    // Smooth delay time transitions
    float delayTimeSamples = static_cast<float>(targetDelayTimeMs * 0.001 * currentSampleRate);
    delayTimeSamples = juce::jlimit(1.0f, static_cast<float>(maxDelaySamples - 100), delayTimeSamples);

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = buffer.getSample(0, s);
        float inR = buffer.getSample(1, s);

        // Calculate delay read positions
        float readPosL = static_cast<float>(writePos) - delayTimeSamples;
        if (readPosL < 0.0f) readPosL += maxDelaySamples;

        float readPosR = readPosL;
        if (currentPanMode == DelayPanMode::PingPong)
        {
            readPosR = static_cast<float>(writePos) - (delayTimeSamples * 0.5f);
            if (readPosR < 0.0f) readPosR += maxDelaySamples;
        }

        // Interpolated read from delay buffer
        int iL1 = static_cast<int>(readPosL);
        int iL2 = (iL1 + 1) % maxDelaySamples;
        float fracL = readPosL - static_cast<float>(iL1);
        float delayedL = delayBuffer.getSample(0, iL1) + fracL * (delayBuffer.getSample(0, iL2) - delayBuffer.getSample(0, iL1));

        int iR1 = static_cast<int>(readPosR);
        int iR2 = (iR1 + 1) % maxDelaySamples;
        float fracR = readPosR - static_cast<float>(iR1);
        float delayedR = delayBuffer.getSample(1, iR1) + fracR * (delayBuffer.getSample(1, iR2) - delayBuffer.getSample(1, iR1));

        if (reverseMode)
        {
            // Reverse buffer read
            float revReadPos = static_cast<float>(maxDelaySamples - 1 - writePos);
            int rIdx = static_cast<int>(revReadPos) % maxDelaySamples;
            delayedL = delayBuffer.getSample(0, rIdx);
            delayedR = delayBuffer.getSample(1, rIdx);
        }

        // Apply Character model
        applyCharacterModel(delayedL, delayedR);

        // Saturation in feedback
        delayedL = applySaturation(delayedL, saturationDrive);
        delayedR = applySaturation(delayedR, saturationDrive);

        // Ducking
        delayedL *= duckGain;
        delayedR *= duckGain;

        // Sequencer step gating/panning
        if (!patternGate[currentStep])
        {
            delayedL *= 0.1f;
            delayedR *= 0.1f;
        }

        // Output to buffer
        buffer.setSample(0, s, delayedL);
        buffer.setSample(1, s, delayedR);

        // Feedback writeback
        float fbL = delayedL * feedbackLevel;
        float fbR = delayedR * feedbackLevel;

        delayBuffer.setSample(0, writePos, inL + fbL);
        delayBuffer.setSample(1, writePos, inR + fbR);

        writePos = (writePos + 1) % maxDelaySamples;

        // Step sequencer advance
        stepSampleCounter++;
        int samplesPer16th = static_cast<int>((currentSampleRate * 60.0) / (currentBpm * 4.0));
        if (samplesPer16th > 0 && stepSampleCounter >= samplesPer16th)
        {
            stepSampleCounter = 0;
            currentStep = (currentStep + 1) % 16;
        }
    }

    // Feedback pitch shifting if active
    if (std::abs(feedbackPitchShift) > 0.01f)
    {
        pitchShifter.process(buffer, feedbackPitchShift);
    }

    if (!isPreEQ)
    {
        eq.process(buffer);
    }
}

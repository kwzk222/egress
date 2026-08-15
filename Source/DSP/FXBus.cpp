#include "FXBus.h"

FXBus::FXBus()
{
    for (int i = 0; i < maxSlots; ++i)
    {
        slots[i] = FXSlot();
    }
}

void FXBus::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    chorusPhase = 0.0f;
}

void FXBus::reset()
{
    chorusPhase = 0.0f;
}

void FXBus::setSlot(int index, const FXSlot& slot)
{
    if (index >= 0 && index < maxSlots)
    {
        slots[index] = slot;
    }
}

void FXBus::setGlobalDrive(float drive)
{
    globalDrive = juce::jlimit(0.0f, 1.0f, drive);
}

void FXBus::processTapeSaturation(juce::AudioBuffer<float>& buffer, float drive, float bias, float mix)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float driveGain = 1.0f + drive * 5.0f;
    float dcBias = bias * 0.2f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int s = 0; s < numSamples; ++s)
        {
            float dry = data[s];
            float wet = (dry + dcBias) * driveGain;
            wet = std::tanh(wet) - (dcBias * 0.5f);
            data[s] = dry * (1.0f - mix) + wet * mix;
        }
    }
}

void FXBus::processTubeWarmth(juce::AudioBuffer<float>& buffer, float drive, float tone, float mix)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float driveGain = 1.0f + drive * 4.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int s = 0; s < numSamples; ++s)
        {
            float dry = data[s];
            float x = dry * driveGain;
            float wet = (x > 0.0f) ? (1.0f - std::exp(-x)) : (-1.0f + std::exp(x));
            data[s] = dry * (1.0f - mix) + wet * mix;
        }
    }
}

void FXBus::processBitcrusher(juce::AudioBuffer<float>& buffer, float bitDepth, float sampleRateReduction, float mix)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float bits = juce::jlimit(2.0f, 16.0f, 16.0f - bitDepth * 14.0f);
    float step = std::pow(2.0f, bits);
    int stepHold = static_cast<int>(1.0f + sampleRateReduction * 20.0f);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        float lastSample = 0.0f;

        for (int s = 0; s < numSamples; ++s)
        {
            float dry = data[s];
            if (s % stepHold == 0)
            {
                lastSample = std::round(dry * step) / step;
            }
            data[s] = dry * (1.0f - mix) + lastSample * mix;
        }
    }
}

void FXBus::processChorusPhaser(juce::AudioBuffer<float>& buffer, float rate, float depth, float mix)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float lfoInc = (rate * 5.0f + 0.1f) / static_cast<float>(currentSampleRate);

    for (int s = 0; s < numSamples; ++s)
    {
        chorusPhase += lfoInc;
        if (chorusPhase >= 1.0f) chorusPhase -= 1.0f;

        float mod = std::sin(chorusPhase * juce::MathConstants<float>::twoPi) * depth * 0.5f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float dry = buffer.getSample(ch, s);
            float wet = dry * (1.0f + mod);
            buffer.setSample(ch, s, dry * (1.0f - mix) + wet * mix);
        }
    }
}

void FXBus::processSlotPosition(FXSlotPosition pos, juce::AudioBuffer<float>& buffer)
{
    for (int i = 0; i < maxSlots; ++i)
    {
        if (slots[i].position == pos && slots[i].type != FXType::None)
        {
            switch (slots[i].type)
            {
                case FXType::TapeSaturation:
                    processTapeSaturation(buffer, slots[i].param1, slots[i].param2, slots[i].mix);
                    break;
                case FXType::TubeWarmth:
                    processTubeWarmth(buffer, slots[i].param1, slots[i].param2, slots[i].mix);
                    break;
                case FXType::Bitcrusher:
                    processBitcrusher(buffer, slots[i].param1, slots[i].param2, slots[i].mix);
                    break;
                case FXType::ChorusPhaser:
                    processChorusPhaser(buffer, slots[i].param1, slots[i].param2, slots[i].mix);
                    break;
                case FXType::None:
                    break;
            }
        }
    }
}

void FXBus::processGlobalDrive(juce::AudioBuffer<float>& buffer)
{
    if (globalDrive <= 0.001f) return;

    processTapeSaturation(buffer, globalDrive, 0.2f, 1.0f);
}

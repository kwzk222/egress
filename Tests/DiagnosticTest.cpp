#include "PluginProcessor.h"
#include <iostream>
#include <vector>
#include <cmath>

int main()
{
    std::cout << "--- DIAGNOSTIC TEST ---" << std::endl;

    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    // 1. Test ParametricEQ alone
    {
        ParametricEQ eq;
        juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };
        eq.prepare(spec);

        juce::AudioBuffer<float> buffer(2, samplesPerBlock);
        buffer.clear();
        // Piano sample impulse
        buffer.setSample(0, 0, 0.8f);
        buffer.setSample(1, 0, 0.8f);

        std::cout << "Testing ParametricEQ over 20 blocks..." << std::endl;
        float maxMag = 0.0f;
        for (int b = 0; b < 20; ++b)
        {
            eq.process(buffer);
            float m = buffer.getMagnitude(0, 0, samplesPerBlock);
            if (m > maxMag) maxMag = m;
            buffer.clear();
        }
        std::cout << "ParametricEQ Max Peak: " << maxMag << std::endl;
    }

    // 2. Test DelayEngine alone
    {
        DelayEngine delay;
        delay.prepare(sampleRate, samplesPerBlock);
        delay.setParams(350.0f, 0.4f, DelayPanMode::Stereo, DelayCharacterModel::StudioDigital, false, 0.0f, 0.0f, 0.0f, false);

        juce::AudioBuffer<float> buffer(2, samplesPerBlock);
        juce::AudioBuffer<float> dry(2, samplesPerBlock);

        std::cout << "Testing DelayEngine over 50 blocks..." << std::endl;
        float maxMag = 0.0f;
        for (int b = 0; b < 50; ++b)
        {
            buffer.clear();
            dry.clear();
            if (b == 0)
            {
                buffer.setSample(0, 0, 0.8f);
                buffer.setSample(1, 0, 0.8f);
                dry.setSample(0, 0, 0.8f);
                dry.setSample(1, 0, 0.8f);
            }
            delay.process(buffer, dry);
            float m = buffer.getMagnitude(0, 0, samplesPerBlock);
            if (m > maxMag) maxMag = m;
            if (b == 0 || b == 30 || b == 31)
            {
                std::cout << "  Delay Block " << b << " Mag: " << m << " S0: " << buffer.getSample(0, 0) << " S1: " << buffer.getSample(0, 1) << std::endl;
            }
        }
        std::cout << "DelayEngine Max Peak: " << maxMag << std::endl;
    }

    // 3. Test ReverbEngine alone
    {
        ReverbEngine reverb;
        reverb.prepare(sampleRate, samplesPerBlock);
        reverb.setParams(ReverbAlgorithm::HallPlate, ReverbEra::Era2000s, 3.0f, 0.7f, 20.0f, 0.7f, 0.7f, 1.0f, 0.3f, false);

        juce::AudioBuffer<float> buffer(2, samplesPerBlock);

        std::cout << "Testing ReverbEngine over 50 blocks..." << std::endl;
        float maxMag = 0.0f;
        for (int b = 0; b < 50; ++b)
        {
            buffer.clear();
            if (b == 0)
            {
                buffer.setSample(0, 0, 0.8f);
                buffer.setSample(1, 0, 0.8f);
            }
            reverb.process(buffer);
            float m = buffer.getMagnitude(0, 0, samplesPerBlock);
            if (m > maxMag) maxMag = m;
            if (b < 5 || b == 10)
            {
                std::cout << "  Reverb Block " << b << " Mag: " << m << " S0: " << buffer.getSample(0, 0) << " S1: " << buffer.getSample(0, 1) << std::endl;
            }
        }
        std::cout << "ReverbEngine Max Peak: " << maxMag << std::endl;
    }

    // 4. Test GranularPitchShifter alone
    {
        GranularPitchShifter pitch;
        pitch.prepare(sampleRate, samplesPerBlock);

        juce::AudioBuffer<float> buffer(2, samplesPerBlock);

        std::cout << "Testing GranularPitchShifter (+12 semitones) over 20 blocks..." << std::endl;
        float maxMag = 0.0f;
        for (int b = 0; b < 20; ++b)
        {
            buffer.clear();
            if (b == 0)
            {
                buffer.setSample(0, 0, 0.8f);
                buffer.setSample(1, 0, 0.8f);
            }
            pitch.process(buffer, 12.0f);
            float m = buffer.getMagnitude(0, 0, samplesPerBlock);
            if (m > maxMag) maxMag = m;
            if (b < 5)
            {
                std::cout << "  Pitch Block " << b << " Mag: " << m << " S0: " << buffer.getSample(0, 0) << " S1: " << buffer.getSample(0, 1) << std::endl;
            }
        }
        std::cout << "GranularPitchShifter Max Peak: " << maxMag << std::endl;
    }

    // 5. Test Full EchoValhallaAudioProcessor chain
    {
        EchoValhallaAudioProcessor processor;
        processor.prepareToPlay(sampleRate, samplesPerBlock);

        juce::AudioBuffer<float> buffer(2, samplesPerBlock);
        juce::MidiBuffer midi;

        std::cout << "Testing Full Processor Chain over 50 blocks..." << std::endl;
        for (int b = 0; b < 50; ++b)
        {
            buffer.clear();
            if (b == 0)
            {
                for (int s = 0; s < samplesPerBlock; ++s)
                {
                    float val = std::sin(2.0f * 3.14159f * 440.0f * s / 44100.0f) * 0.5f;
                    buffer.setSample(0, s, val);
                    buffer.setSample(1, s, val);
                }
            }
            processor.processBlock(buffer, midi);
            float m = buffer.getMagnitude(0, 0, samplesPerBlock);
            if (b == 0 || b == 1 || b == 30 || b == 49)
            {
                std::cout << "  Full Chain Block " << b << " Mag: " << m
                          << " S0: " << buffer.getSample(0, 0)
                          << " S1: " << buffer.getSample(0, 1)
                          << " S2: " << buffer.getSample(0, 2) << std::endl;
            }
        }
    }

    return 0;
}

#include "DSP/DelayEngine.h"
#include "DSP/ReverbEngine.h"
#include "DSP/FXBus.h"
#include "DSP/ParametricEQ.h"
#include <iostream>
#include <cmath>

int main()
{
    std::cout << "Testing Audio Processing Signal Flow..." << std::endl;

    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    // Test 1: ParametricEQ alone
    ParametricEQ eq;
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };
    eq.prepare(spec);

    juce::AudioBuffer<float> buffer(2, samplesPerBlock);
    buffer.clear();
    // Sine wave input
    for (int s = 0; s < samplesPerBlock; ++s)
    {
        float val = std::sin(2.0f * 3.14159f * 440.0f * s / 44100.0f) * 0.5f;
        buffer.setSample(0, s, val);
        buffer.setSample(1, s, val);
    }

    eq.process(buffer);
    std::cout << "EQ Output Sample 0: " << buffer.getSample(0, 0)
              << ", Sample 100: " << buffer.getSample(0, 100) << std::endl;

    // Test 2: Delay Engine
    DelayEngine delay;
    delay.prepare(sampleRate, samplesPerBlock);
    delay.setParams(350.0f, 0.4f, DelayPanMode::Stereo, DelayCharacterModel::StudioDigital, false, 0.0f, 0.0f, 0.0f, false);

    juce::AudioBuffer<float> delayBuf(2, samplesPerBlock);
    for (int s = 0; s < samplesPerBlock; ++s)
    {
        float val = std::sin(2.0f * 3.14159f * 440.0f * s / 44100.0f) * 0.5f;
        delayBuf.setSample(0, s, val);
        delayBuf.setSample(1, s, val);
    }
    juce::AudioBuffer<float> dryBuf = delayBuf;

    delay.process(delayBuf, dryBuf);
    std::cout << "Delay Output Sample 0: " << delayBuf.getSample(0, 0)
              << ", Sample 100: " << delayBuf.getSample(0, 100) << std::endl;

    // Test 3: Reverb Engine
    ReverbEngine reverb;
    reverb.prepare(sampleRate, samplesPerBlock);
    reverb.setParams(ReverbAlgorithm::HallPlate, ReverbEra::Era2000s, 3.0f, 0.7f, 20.0f, 0.7f, 0.7f, 1.0f, 0.3f, false);

    juce::AudioBuffer<float> revBuf(2, samplesPerBlock);
    for (int s = 0; s < samplesPerBlock; ++s)
    {
        float val = std::sin(2.0f * 3.14159f * 440.0f * s / 44100.0f) * 0.5f;
        revBuf.setSample(0, s, val);
        revBuf.setSample(1, s, val);
    }

    reverb.process(revBuf);
    std::cout << "Reverb Output Sample 0: " << revBuf.getSample(0, 0)
              << ", Sample 100: " << revBuf.getSample(0, 100) << std::endl;

    // Test 4: Run multiple blocks sequentially to check for instability/growth
    std::cout << "Running 50 blocks through full chain..." << std::endl;
    for (int block = 0; block < 50; ++block)
    {
        juce::AudioBuffer<float> blockBuf(2, samplesPerBlock);
        for (int s = 0; s < samplesPerBlock; ++s)
        {
            float val = (block == 0) ? std::sin(2.0f * 3.14159f * 440.0f * s / 44100.0f) * 0.5f : 0.0f;
            blockBuf.setSample(0, s, val);
            blockBuf.setSample(1, s, val);
        }
        juce::AudioBuffer<float> dryCopy = blockBuf;

        delay.process(blockBuf, dryCopy);
        reverb.process(blockBuf);

        if (block == 0 || block == 1 || block == 10 || block == 49)
        {
            float maxVal = blockBuf.getMagnitude(0, samplesPerBlock);
            std::cout << "Block " << block << " Peak Amplitude: " << maxVal << std::endl;
        }
    }

    std::cout << "Signal Flow Test Completed." << std::endl;
    return 0;
}

#include "DSP/DelayEngine.h"
#include "DSP/ReverbEngine.h"
#include "DSP/FXBus.h"
#include "DSP/ParametricEQ.h"
#include <iostream>
#include <cassert>

int main()
{
    std::cout << "Running DSP Unit Tests..." << std::endl;

    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    // Test Parametric EQ
    ParametricEQ eq;
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };
    eq.prepare(spec);

    juce::AudioBuffer<float> buffer(2, samplesPerBlock);
    buffer.clear();
    for (int s = 0; s < samplesPerBlock; ++s)
    {
        buffer.setSample(0, s, 1.0f);
        buffer.setSample(1, s, 1.0f);
    }

    eq.process(buffer);
    std::cout << "[PASS] ParametricEQ processing complete." << std::endl;

    // Test Delay Engine
    DelayEngine delay;
    delay.prepare(sampleRate, samplesPerBlock);
    delay.setParams(250.0f, 0.5f, DelayPanMode::Stereo, DelayCharacterModel::TapeEcho, false, 2.0f, 0.0f, 0.2f, false);

    juce::AudioBuffer<float> dryBuffer(2, samplesPerBlock);
    dryBuffer.clear();
    delay.process(buffer, dryBuffer);
    std::cout << "[PASS] DelayEngine processing complete." << std::endl;

    // Test Reverb Engine
    ReverbEngine reverb;
    reverb.prepare(sampleRate, samplesPerBlock);
    reverb.setParams(ReverbAlgorithm::AmbientShimmer, ReverbEra::Era1980s, 5.0f, 0.8f, 20.0f, 0.8f, 0.8f, 1.0f, 0.5f, false);
    reverb.process(buffer);
    std::cout << "[PASS] ReverbEngine processing complete." << std::endl;

    // Test FX Bus
    FXBus fxBus;
    fxBus.prepare(sampleRate, samplesPerBlock);
    fxBus.setGlobalDrive(0.5f);
    fxBus.processGlobalDrive(buffer);
    std::cout << "[PASS] FXBus processing complete." << std::endl;

    std::cout << "All DSP Unit Tests Passed Successfully!" << std::endl;
    return 0;
}

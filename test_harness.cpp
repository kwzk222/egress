#include "Source/PluginProcessor.h"
#include <iostream>
#include <cmath>

int main()
{
    std::cout << "Starting Audio Processor Diagnostic Test..." << std::endl;

    EchoValhallaAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    // Test 1: Process 100 blocks of SILENCE
    std::cout << "Test 1: Processing 100 blocks of pure silence..." << std::endl;
    bool noiseFound = false;
    for (int block = 0; block < 100; ++block)
    {
        buffer.clear();
        processor.processBlock(buffer, midi);
        float mag = buffer.getMagnitude(0, 512);
        if (mag > 0.00001f)
        {
            std::cout << "NOISE GENERATED ON SILENCE at block " << block << "! Mag: " << mag
                      << " Sample 0: " << buffer.getSample(0, 0)
                      << " Sample 1: " << buffer.getSample(0, 1) << std::endl;
            noiseFound = true;
            break;
        }
    }
    if (!noiseFound) std::cout << "Test 1 PASSED: Silence produced 0 noise." << std::endl;

    // Test 2: Process a 1-sample impulse (like a piano strike or click), then silence for 200 blocks
    std::cout << "\nTest 2: Impulse response test (1 sample = 0.5f, then 200 blocks silence)..." << std::endl;
    for (int block = 0; block < 200; ++block)
    {
        buffer.clear();
        if (block == 0)
        {
            buffer.setSample(0, 0, 0.5f);
            buffer.setSample(1, 0, 0.5f);
        }
        processor.processBlock(buffer, midi);
        float mag = buffer.getMagnitude(0, 512);
        if (block == 0 || block == 1 || block == 10 || block == 50 || block == 100 || block == 199 || mag > 1.0f)
        {
            std::cout << "Block " << block << " Max Mag: " << mag
                      << " S0: " << buffer.getSample(0, 0)
                      << " S1: " << buffer.getSample(0, 1) << std::endl;
        }
    }

    return 0;
}

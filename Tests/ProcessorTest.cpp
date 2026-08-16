#include "PluginProcessor.h"
#include <iostream>
#include <cmath>

int main()
{
    std::cout << "--- DETAILED AUDIO TEST ---" << std::endl;

    EchoValhallaAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    // Let's test with a sine wave input (simulating piano note at 440 Hz)
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    std::cout << "Feeding 440Hz Sine Wave for 100 blocks..." << std::endl;
    for (int block = 0; block < 100; ++block)
    {
        for (int s = 0; s < 512; ++s)
        {
            float val = std::sin(2.0f * 3.14159265f * 440.0f * (block * 512 + s) / 44100.0f) * 0.3f;
            buffer.setSample(0, s, val);
            buffer.setSample(1, s, val);
        }

        processor.processBlock(buffer, midi);

        // Check first 10 samples of block 0, 1, 2, 10, 50, 99
        if (block == 0 || block == 1 || block == 2 || block == 10 || block == 50 || block == 99)
        {
            std::cout << "\nBlock " << block << " (Output samples 0..9):" << std::endl;
            for (int s = 0; s < 10; ++s)
            {
                std::cout << "  s[" << s << "] = " << buffer.getSample(0, s) << std::endl;
            }
        }
    }

    std::cout << "\nNow feeding SILENCE after tone for 100 blocks..." << std::endl;
    for (int block = 100; block < 200; ++block)
    {
        buffer.clear();
        processor.processBlock(buffer, midi);

        float mag = buffer.getMagnitude(0, 0, 512);
        if (block == 100 || block == 101 || block == 110 || block == 150 || block == 199)
        {
            std::cout << "\nSilence Block " << block << " Peak Mag: " << mag << " Samples 0..4:" << std::endl;
            for (int s = 0; s < 5; ++s)
            {
                std::cout << "  s[" << s << "] = " << buffer.getSample(0, s) << std::endl;
            }
        }
    }

    return 0;
}

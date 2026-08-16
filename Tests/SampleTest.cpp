#include "PluginProcessor.h"
#include <iostream>
#include <vector>
#include <cmath>

int main()
{
    std::cout << "=== SAMPLE-BY-SAMPLE INSPECTION ===" << std::endl;

    EchoValhallaAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    // Feed a single piano note (sine wave at 261.63 Hz, C4, decaying)
    std::cout << "Feeding C4 note for 10 blocks..." << std::endl;

    for (int block = 0; block < 10; ++block)
    {
        for (int s = 0; s < 512; ++s)
        {
            int totalS = block * 512 + s;
            float env = std::exp(-static_cast<float>(totalS) / 10000.0f);
            float val = std::sin(2.0f * 3.14159265f * 261.63f * totalS / 44100.0f) * env * 0.5f;
            buffer.setSample(0, s, val);
            buffer.setSample(1, s, val);
        }

        processor.processBlock(buffer, midi);

        std::cout << "Block " << block << " (First 16 samples L):" << std::endl;
        for (int s = 0; s < 16; ++s)
        {
            std::cout << " [" << s << "]: " << buffer.getSample(0, s);
        }
        std::cout << std::endl;
    }

    return 0;
}

#include "../Source/PluginProcessor.h"
#include <iostream>
#include <cmath>

int main()
{
    std::cout << "Testing EchoValhallaAudioProcessor processBlock..." << std::endl;

    EchoValhallaAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    // Send a 1-sample impulse, then silence
    for (int block = 0; block < 100; ++block)
    {
        buffer.clear();
        if (block == 0)
        {
            buffer.setSample(0, 0, 0.8f);
            buffer.setSample(1, 0, 0.8f);
        }

        processor.processBlock(buffer, midi);

        float magL = buffer.getMagnitude(0, 0, 512);
        float magR = buffer.getMagnitude(1, 0, 512);

        if (block < 10 || block == 20 || block == 50 || block == 99)
        {
            std::cout << "Block " << block << " Max Mag L: " << magL << " R: " << magR << std::endl;
            std::cout << "  Sample 0 L: " << buffer.getSample(0, 0) << " Sample 1 L: " << buffer.getSample(0, 1) << std::endl;
        }
    }

    return 0;
}

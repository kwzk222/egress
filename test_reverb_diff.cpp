#include "Source/DSP/ReverbEngine.h"
#include <iostream>
#include <cmath>

int main()
{
    ReverbEngine reverb;
    reverb.prepare(44100.0, 512);
    reverb.setParams(ReverbAlgorithm::HallPlate, ReverbEra::Era2000s, 3.0f, 0.7f, 0.0f, 0.7f, 0.7f, 1.0f, 0.0f, false);

    juce::AudioBuffer<float> buffer(2, 512);

    // Feed a piano note C4 (261.63 Hz) for 5 blocks, then silence
    std::cout << "Testing Reverb response to C4 note..." << std::endl;
    for (int b = 0; b < 40; ++b)
    {
        buffer.clear();
        if (b < 5)
        {
            for (int s = 0; s < 512; ++s)
            {
                float val = std::sin(2.0f * 3.14159265f * 261.63f * (b * 512 + s) / 44100.0f) * 0.4f;
                buffer.setSample(0, s, val);
                buffer.setSample(1, s, val);
            }
        }

        reverb.process(buffer);

        if (b == 0 || b == 4 || b == 5 || b == 10 || b == 20 || b == 39)
        {
            float peak = buffer.getMagnitude(0, 0, 512);
            std::cout << "Block " << b << " Peak: " << peak
                      << " s[0]: " << buffer.getSample(0, 0)
                      << " s[1]: " << buffer.getSample(0, 1)
                      << " s[2]: " << buffer.getSample(0, 2) << std::endl;
        }
    }

    return 0;
}

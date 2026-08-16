#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include <cmath>

int main()
{
    juce::Reverb reverb;
    reverb.setSampleRate(44100.0);

    juce::Reverb::Parameters params;
    params.roomSize = 0.7f;
    params.damping = 0.3f;
    params.wetLevel = 1.0f; // Original wet level
    params.dryLevel = 0.0f;
    params.width = 1.0f;
    params.freezeMode = 0.0f;
    reverb.setParameters(params);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    // 0.5 amplitude sine wave (like piano note)
    for (int s = 0; s < 512; ++s)
    {
        float val = std::sin(2.0f * 3.14159265f * 440.0f * s / 44100.0f) * 0.5f;
        buffer.setSample(0, s, val);
        buffer.setSample(1, s, val);
    }

    reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), 512);

    float maxL = buffer.getMagnitude(0, 0, 512);
    std::cout << "Reverb Output Peak with wetLevel=1.0: " << maxL << " (Input was 0.5)" << std::endl;

    // Now test with wetLevel = 0.12
    reverb.reset();
    params.wetLevel = 0.12f;
    reverb.setParameters(params);

    buffer.clear();
    for (int s = 0; s < 512; ++s)
    {
        float val = std::sin(2.0f * 3.14159265f * 440.0f * s / 44100.0f) * 0.5f;
        buffer.setSample(0, s, val);
        buffer.setSample(1, s, val);
    }

    reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), 512);
    float maxL_fixed = buffer.getMagnitude(0, 0, 512);
    std::cout << "Reverb Output Peak with wetLevel=0.12: " << maxL_fixed << " (Input was 0.5)" << std::endl;

    return 0;
}

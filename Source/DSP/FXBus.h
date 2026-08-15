#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

enum class FXSlotPosition
{
    PreDelay,
    PostDelay,
    PreReverb,
    PostReverb,
    GlobalWet
};

enum class FXType
{
    TapeSaturation,
    TubeWarmth,
    Bitcrusher,
    ChorusPhaser,
    None
};

struct FXSlot
{
    FXType type { FXType::None };
    FXSlotPosition position { FXSlotPosition::GlobalWet };
    float param1 { 0.5f }; // Drive / Bits / Rate
    float param2 { 0.5f }; // Bias / SampleRate / Depth
    float mix { 1.0f };    // Wet Mix
};

class FXBus
{
public:
    FXBus();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    void setSlot(int index, const FXSlot& slot);
    void setGlobalDrive(float drive);

    void processSlotPosition(FXSlotPosition pos, juce::AudioBuffer<float>& buffer);
    void processGlobalDrive(juce::AudioBuffer<float>& buffer);

private:
    static constexpr int maxSlots = 5;
    std::array<FXSlot, maxSlots> slots;
    float globalDrive { 0.0f };
    double currentSampleRate { 44100.0 };

    // Modulation states for Chorus/Phaser
    float chorusPhase { 0.0f };

    void processTapeSaturation(juce::AudioBuffer<float>& buffer, float drive, float bias, float mix);
    void processTubeWarmth(juce::AudioBuffer<float>& buffer, float drive, float tone, float mix);
    void processBitcrusher(juce::AudioBuffer<float>& buffer, float bitDepth, float sampleRateReduction, float mix);
    void processChorusPhaser(juce::AudioBuffer<float>& buffer, float rate, float depth, float mix);
};

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

enum class ModSourceType
{
    LFO,
    EnvelopeFollower,
    StepSequencer,
    Macro,
    PitchTracker
};

struct ModRoute
{
    ModSourceType source { ModSourceType::LFO };
    int sourceIndex { 0 }; // 0..3
    juce::String targetParamID;
    float depth { 0.0f };
    float smoothing { 0.05f };
};

class ModulationEngine
{
public:
    ModulationEngine() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        sr = sampleRate;
        for (int i = 0; i < 4; ++i)
        {
            lfoPhases[i] = 0.0f;
            envFollowers[i] = 0.0f;
            seqSteps[i] = 0;
            seqCounters[i] = 0;
        }
    }

    void addRoute(const ModRoute& route)
    {
        routes.push_back(route);
    }

    void clearRoutes()
    {
        routes.clear();
    }

    void processBlock(const juce::AudioBuffer<float>& inputBuffer, juce::AudioProcessorValueTreeState& apvts)
    {
        if (sr <= 0.0) return;

        int numSamples = inputBuffer.getNumSamples();

        // Update Envelope Followers
        float rms = inputBuffer.getRMSLevel(0, 0, numSamples);
        for (int i = 0; i < 4; ++i)
        {
            envFollowers[i] = envFollowers[i] * 0.9f + rms * 0.1f;
        }

        // Advance LFOs
        for (int i = 0; i < 4; ++i)
        {
            lfoPhases[i] += (1.0f / static_cast<float>(sr)) * static_cast<float>(numSamples);
            if (lfoPhases[i] >= 1.0f) lfoPhases[i] -= 1.0f;
        }

        // Apply Modulation to target APVTS parameters with smoothing
        for (const auto& route : routes)
        {
            float modValue = 0.0f;
            if (route.source == ModSourceType::LFO)
            {
                modValue = std::sin(lfoPhases[route.sourceIndex % 4] * juce::MathConstants<float>::twoPi);
            }
            else if (route.source == ModSourceType::EnvelopeFollower)
            {
                modValue = envFollowers[route.sourceIndex % 4];
            }

            if (auto* param = apvts.getParameter(route.targetParamID))
            {
                float currentVal = param->getValue();
                float newVal = juce::jlimit(0.0f, 1.0f, currentVal + modValue * route.depth);
                param->setValueNotifyingHost(newVal);
            }
        }
    }

private:
    double sr { 44100.0 };
    std::array<float, 4> lfoPhases { 0.0f };
    std::array<float, 4> envFollowers { 0.0f };
    std::array<int, 4> seqSteps { 0 };
    std::array<int, 4> seqCounters { 0 };

    std::vector<ModRoute> routes;
};

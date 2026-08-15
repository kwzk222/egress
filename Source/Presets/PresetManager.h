#pragma once

#include <JuceHeader.h>

class PresetManager
{
public:
    PresetManager(juce::AudioProcessorValueTreeState& vts)
        : apvts(vts)
    {
    }

    void saveGlobalPreset(const juce::File& file)
    {
        auto state = apvts.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        if (xml != nullptr)
        {
            xml->writeTo(file);
        }
    }

    void loadGlobalPreset(const juce::File& file)
    {
        std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
        if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
        }
    }

    // Save/Load Component Presets
    void saveComponentPreset(const juce::File& file, const juce::StringArray& paramIDs)
    {
        juce::ValueTree componentState("ComponentPreset");
        for (const auto& id : paramIDs)
        {
            if (auto* param = apvts.getParameter(id))
            {
                componentState.setProperty(id, param->getValue(), nullptr);
            }
        }

        std::unique_ptr<juce::XmlElement> xml(componentState.createXml());
        if (xml != nullptr)
        {
            xml->writeTo(file);
        }
    }

    void loadComponentPreset(const juce::File& file)
    {
        std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
        if (xml != nullptr && xml->hasTagName("ComponentPreset"))
        {
            auto vt = juce::ValueTree::fromXml(*xml);
            for (int i = 0; i < vt.getNumProperties(); ++i)
            {
                auto propName = vt.getPropertyName(i).toString();
                if (auto* param = apvts.getParameter(propName))
                {
                    param->setValueNotifyingHost(static_cast<float>(vt.getProperty(propName)));
                }
            }
        }
    }

    // Store/Retrieve States for A/B Morphing
    void storeStateA()
    {
        stateA = apvts.copyState();
    }

    void storeStateB()
    {
        stateB = apvts.copyState();
    }

    void morphStateAB(float morphFactor)
    {
        if (!stateA.isValid() || !stateB.isValid()) return;

        morphFactor = juce::jlimit(0.0f, 1.0f, morphFactor);

        for (auto* param : apvts.processor.getParameters())
        {
            if (auto* floatParam = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
            {
                juce::String paramID = floatParam->paramID;

                if (stateA.hasProperty(paramID) && stateB.hasProperty(paramID))
                {
                    float valA = stateA.getProperty(paramID);
                    float valB = stateB.getProperty(paramID);

                    // Continuous linear/exponential interpolation for smooth parameters
                    float valMorphed = (morphFactor < 0.5f) ?
                        valA + (valB - valA) * morphFactor :
                        valA + (valB - valA) * morphFactor;

                    // Hard switch for discrete choices
                    if (paramID.contains("Model") || paramID.contains("Algo") || paramID.contains("Mode") || paramID.contains("Era"))
                    {
                        valMorphed = (morphFactor < 0.5f) ? valA : valB;
                    }

                    floatParam->setValueNotifyingHost(valMorphed);
                }
            }
        }
    }

    // Smart Randomizer with Parameter Lock support
    void randomizeParameters(const juce::StringArray& lockedParamIDs = {})
    {
        juce::Random rng;

        for (auto* param : apvts.processor.getParameters())
        {
            if (auto* floatParam = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
            {
                juce::String paramID = floatParam->paramID;
                if (!lockedParamIDs.contains(paramID) && !paramID.contains("Master") && !paramID.contains("Bypass"))
                {
                    float randomValue = rng.nextFloat();
                    floatParam->setValueNotifyingHost(randomValue);
                }
            }
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::ValueTree stateA;
    juce::ValueTree stateB;
};

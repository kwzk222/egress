#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "UI/DarkLookAndFeel.h"
#include "UI/EQVisualizer.h"
#include "PluginProcessor.h"

class EchoValhallaAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    EchoValhallaAudioProcessorEditor(EchoValhallaAudioProcessor& p);
    ~EchoValhallaAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    EchoValhallaAudioProcessor& audioProcessor;
    DarkLookAndFeel darkLookAndFeel;

    juce::Label titleLabel;

    // Master / Top Bar Controls
    juce::Slider masterDryWetSlider, masterGainSlider, morphSlider;
    juce::Label masterDryWetLabel, masterGainLabel, morphLabel;
    juce::TextButton randomizeBtn, freezeBtn, stutterBtn;
    juce::ComboBox routingBox;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterDryWetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> stutterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> routingAttachment;

    // Delay Controls
    juce::ComboBox delayModelBox;
    juce::Slider delayTimeSlider, delayFeedbackSlider;
    juce::Label delayTimeLabel, delayFeedbackLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> delayModelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayFeedbackAttachment;

    // Reverb Controls
    juce::ComboBox reverbAlgoBox;
    juce::Slider reverbDecaySlider, reverbSizeSlider;
    juce::Label reverbDecayLabel, reverbSizeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> reverbAlgoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbSizeAttachment;

    // Visualizers
    EQVisualizer delayEQVisualizer;
    EQVisualizer reverbEQVisualizer;

    void setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& labelText, const juce::String& paramID);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EchoValhallaAudioProcessorEditor)
};

#include "UI/DarkLookAndFeel.h"
#include "UI/EQVisualizer.h"
#include "PluginProcessor.h"

class EchoValhallaAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    EchoValhallaAudioProcessorEditor(EchoValhallaAudioProcessor& p)
        : AudioProcessorEditor(&p), audioProcessor(p),
          delayEQVisualizer(p.getDelayEngine().getEQ(), p.getDelaySpectrumData()),
          reverbEQVisualizer(p.getReverbEngine().getEQ(), p.getReverbSpectrumData())
    {
        juce::LookAndFeel::setDefaultLookAndFeel(&darkLookAndFeel);

        // Header / Master controls
        addAndMakeVisible(titleLabel);
        titleLabel.setText("ECHO VALHALLA SUPERPLUGIN", juce::dontSendNotification);
        titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
        titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00d2ff));

        // Combo Boxes
        addAndMakeVisible(delayModelBox);
        delayModelBox.addItemList(juce::StringArray { "Studio Digital", "Analog BBD", "Tape Echo", "Oil Can", "Digital 80s" }, 1);
        delayModelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.getAPVTS(), "delayModel", delayModelBox);

        addAndMakeVisible(reverbAlgoBox);
        reverbAlgoBox.addItemList(juce::StringArray { "Room/Chamber", "Hall/Plate", "Ambient/Shimmer", "Non-Linear/Gated", "Blackout/Blackhole" }, 1);
        reverbAlgoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.getAPVTS(), "reverbAlgo", reverbAlgoBox);

        addAndMakeVisible(routingBox);
        routingBox.addItemList(juce::StringArray { "Serial D->R", "Serial R->D", "Parallel", "Crossover" }, 1);
        routingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.getAPVTS(), "masterRouting", routingBox);

        // Master Sliders
        setupKnob(masterDryWetSlider, masterDryWetLabel, "DRY/WET", "masterDryWet");
        setupKnob(masterGainSlider, masterGainLabel, "GAIN", "masterOutputGain");
        setupKnob(delayTimeSlider, delayTimeLabel, "TIME", "delayTime");
        setupKnob(delayFeedbackSlider, delayFeedbackLabel, "FEEDBACK", "delayFeedback");
        setupKnob(reverbDecaySlider, reverbDecayLabel, "DECAY", "reverbDecay");
        setupKnob(reverbSizeSlider, reverbSizeLabel, "SIZE", "reverbSize");

        // Morph Slider
        addAndMakeVisible(morphSlider);
        morphSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        morphSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        morphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), "morphFactor", morphSlider);

        addAndMakeVisible(morphLabel);
        morphLabel.setText("A / B MORPH", juce::dontSendNotification);
        morphLabel.setFont(juce::Font(12.0f, juce::Font::bold));

        // Randomize & Action Buttons
        addAndMakeVisible(randomizeBtn);
        randomizeBtn.setButtonText("RANDOMIZE");
        randomizeBtn.onClick = [this] {
            audioProcessor.getPresetManager().randomizeParameters();
        };

        addAndMakeVisible(freezeBtn);
        freezeBtn.setButtonText("FREEZE");
        freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.getAPVTS(), "masterFreeze", freezeBtn);

        addAndMakeVisible(stutterBtn);
        stutterBtn.setButtonText("STUTTER");
        stutterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.getAPVTS(), "masterStutter", stutterBtn);

        // Visualizers
        addAndMakeVisible(delayEQVisualizer);
        addAndMakeVisible(reverbEQVisualizer);

        setResizable(true, true);
        setResizeLimits(1100, 700, 2800, 1800);
        setSize(1400, 900);
    }

    ~EchoValhallaAudioProcessorEditor() override
    {
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff121418));

        // Panel Borders & Backgrounds
        g.setColour(juce::Colour(0xff1e232a));

        // Delay Panel (Left)
        g.fillRoundedRectangle(20, 70, 420, 790, 8.0f);

        // Center Panel (FX & Routing)
        g.fillRoundedRectangle(460, 70, 480, 790, 8.0f);

        // Reverb Panel (Right)
        g.fillRoundedRectangle(960, 70, 420, 790, 8.0f);

        // Panel Headers
        g.setColour(juce::Colour(0xffe0e6ed));
        g.setFont(juce::Font(16.0f, juce::Font::bold));
        g.drawText("DELAY ENGINE", 35, 80, 200, 30, juce::Justification::left);
        g.drawText("ROUTING & FX BUS", 475, 80, 200, 30, juce::Justification::left);
        g.drawText("REVERB ENGINE", 975, 80, 200, 30, juce::Justification::left);
    }

    void resized() override
    {
        titleLabel.setBounds(20, 15, 350, 35);

        // Top Bar Controls
        morphLabel.setBounds(460, 15, 80, 20);
        morphSlider.setBounds(540, 15, 200, 25);
        randomizeBtn.setBounds(760, 15, 100, 30);
        freezeBtn.setBounds(870, 15, 80, 30);
        stutterBtn.setBounds(960, 15, 80, 30);

        masterDryWetSlider.setBounds(1180, 10, 50, 50);
        masterGainSlider.setBounds(1260, 10, 50, 50);

        // Left Panel - Delay
        delayModelBox.setBounds(40, 120, 150, 25);
        delayTimeSlider.setBounds(40, 160, 80, 80);
        delayFeedbackSlider.setBounds(140, 160, 80, 80);
        delayEQVisualizer.setBounds(35, 600, 390, 240);

        // Center Panel - Routing
        routingBox.setBounds(480, 120, 150, 25);

        // Right Panel - Reverb
        reverbAlgoBox.setBounds(980, 120, 150, 25);
        reverbDecaySlider.setBounds(980, 160, 80, 80);
        reverbSizeSlider.setBounds(1080, 160, 80, 80);
        reverbEQVisualizer.setBounds(975, 600, 390, 240);
    }

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

    void setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& labelText, const juce::String& paramID)
    {
        addAndMakeVisible(slider);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

        addAndMakeVisible(label);
        label.setText(labelText, juce::dontSendNotification);
        label.setFont(juce::Font(11.0f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centred);
        label.attachToComponent(&slider, false);

        if (paramID == "masterDryWet")
            masterDryWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), paramID, slider);
        else if (paramID == "masterOutputGain")
            masterGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), paramID, slider);
        else if (paramID == "delayTime")
            delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), paramID, slider);
        else if (paramID == "delayFeedback")
            delayFeedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), paramID, slider);
        else if (paramID == "reverbDecay")
            reverbDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), paramID, slider);
        else if (paramID == "reverbSize")
            reverbSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), paramID, slider);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EchoValhallaAudioProcessorEditor)
};

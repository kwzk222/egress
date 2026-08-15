#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "DSP/DelayEngine.h"
#include "DSP/ReverbEngine.h"
#include "DSP/FXBus.h"
#include "Presets/PresetManager.h"

class EchoValhallaAudioProcessor : public juce::AudioProcessor
{
public:
    EchoValhallaAudioProcessor();
    ~EchoValhallaAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    PresetManager& getPresetManager() { return presetManager; }
    DelayEngine& getDelayEngine() { return delayEngine; }
    ReverbEngine& getReverbEngine() { return reverbEngine; }
    FXBus& getFXBus() { return fxBus; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Visualizer FFT buffers / spectrum calculation helpers
    std::array<float, 512>& getDelaySpectrumData() { return delaySpectrumData; }
    std::array<float, 512>& getReverbSpectrumData() { return reverbSpectrumData; }

private:
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presetManager;

    DelayEngine delayEngine;
    ReverbEngine reverbEngine;
    FXBus fxBus;

    // Buffer processing scratch memory
    juce::AudioBuffer<float> delayBuffer;
    juce::AudioBuffer<float> reverbBuffer;
    juce::AudioBuffer<float> fxBuffer;
    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> crossoverLowBuffer;
    juce::AudioBuffer<float> crossoverHighBuffer;

    // Stutter buffer
    juce::AudioBuffer<float> stutterBuffer;
    int stutterWritePos { 0 };
    int stutterReadPos { 0 };
    int stutterLengthSamples { 0 };

    // Simple Crossover Filters
    juce::dsp::StateVariableTPTFilter<float> crossoverLP;
    juce::dsp::StateVariableTPTFilter<float> crossoverHP;

    // Spectrum FIFO buffers for UI
    static constexpr int fftSize = 1024;
    juce::dsp::FFT forwardFFT { 10 }; // 2^10 = 1024
    std::array<float, fftSize * 2> delayFFTInput { 0 };
    std::array<float, fftSize * 2> reverbFFTInput { 0 };
    std::array<float, 512> delaySpectrumData { 0 };
    std::array<float, 512> reverbSpectrumData { 0 };
    int delayFFTFifoIndex { 0 };
    int reverbFFTFifoIndex { 0 };

    void pushSampleToDelayFFT(float sample);
    void pushSampleToReverbFFT(float sample);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EchoValhallaAudioProcessor)
};

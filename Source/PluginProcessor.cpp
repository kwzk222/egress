#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout EchoValhallaAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Master Controls
    params.push_back(std::make_unique<juce::AudioParameterFloat>("masterDryWet", "Master Dry/Wet", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("masterOutputGain", "Master Gain dB", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("masterRouting", "Routing Mode", juce::StringArray { "Serial D->R", "Serial R->D", "Parallel", "Crossover" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterBool>("masterAutoGain", "Auto Gain", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("masterFreeze", "Freeze", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("masterStutter", "Stutter", false));

    // Delay Engine Controls
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayTime", "Delay Time Ms", 10.0f, 2000.0f, 350.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayFeedback", "Delay Feedback", 0.0f, 0.95f, 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("delayPanMode", "Delay Pan Mode", juce::StringArray { "Stereo", "PingPong", "LCR", "MidSide" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("delayModel", "Delay Model", juce::StringArray { "Studio Digital", "Analog BBD", "Tape Echo", "Oil Can", "Digital 80s" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterBool>("delayReverse", "Reverse Delay", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayPitch", "Feedback Pitch Shift", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayDucking", "Delay Ducking dB", 0.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delaySatDrive", "Delay Saturation", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("delayPreEQ", "Delay Pre/Post EQ", false));

    // Reverb Engine Controls
    params.push_back(std::make_unique<juce::AudioParameterChoice>("reverbAlgo", "Reverb Algo", juce::StringArray { "Room/Chamber", "Hall/Plate", "Ambient/Shimmer", "Non-Linear/Gated", "Blackout/Blackhole" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("reverbEra", "Reverb Era", juce::StringArray { "1970s", "1980s", "2000s" }, 2));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbDecay", "Decay Time Sec", 0.1f, 60.0f, 3.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbSize", "Room Size", 0.1f, 1.0f, 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbPreDelay", "Pre-Delay Ms", 0.0f, 500.0f, 20.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbDiffHigh", "Diff High", 0.0f, 1.0f, 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbDiffLow", "Diff Low", 0.0f, 1.0f, 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbModRate", "Mod Rate", 0.1f, 10.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbModDepth", "Mod Depth", 0.0f, 1.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("reverbPreEQ", "Reverb Pre/Post EQ", false));

    // Global FX Drive
    params.push_back(std::make_unique<juce::AudioParameterFloat>("globalDrive", "Global Saturation", 0.0f, 1.0f, 0.0f));

    // Morph Factor A/B
    params.push_back(std::make_unique<juce::AudioParameterFloat>("morphFactor", "A/B Morph", 0.0f, 1.0f, 0.0f));

    return { params.begin(), params.end() };
}

EchoValhallaAudioProcessor::EchoValhallaAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsSynth
                      #if ! JucePlugin_IsMono
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout()),
       presetManager(apvts)
#endif
{
}

EchoValhallaAudioProcessor::~EchoValhallaAudioProcessor()
{
}

bool EchoValhallaAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
}

void EchoValhallaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    delayEngine.prepare(sampleRate, samplesPerBlock);
    reverbEngine.prepare(sampleRate, samplesPerBlock);
    fxBus.prepare(sampleRate, samplesPerBlock);

    delayBuffer.setSize(2, samplesPerBlock);
    delayBuffer.clear();

    reverbBuffer.setSize(2, samplesPerBlock);
    reverbBuffer.clear();

    fxBuffer.setSize(2, samplesPerBlock);
    fxBuffer.clear();

    dryBuffer.setSize(2, samplesPerBlock);
    dryBuffer.clear();

    crossoverLowBuffer.setSize(2, samplesPerBlock);
    crossoverLowBuffer.clear();

    crossoverHighBuffer.setSize(2, samplesPerBlock);
    crossoverHighBuffer.clear();

    stutterBuffer.setSize(2, static_cast<int>(sampleRate * 2.0)); // 2 sec stutter loop
    stutterBuffer.clear();
    stutterWritePos = 0;
    stutterReadPos = 0;
    stutterLengthSamples = static_cast<int>(sampleRate * 0.25); // 1/16 stutter default

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };
    crossoverLP.prepare(spec);
    crossoverHP.prepare(spec);

    crossoverLP.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    crossoverHP.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    crossoverLP.setCutoffFrequency(800.0f);
    crossoverHP.setCutoffFrequency(800.0f);
}

void EchoValhallaAudioProcessor::releaseResources()
{
}

void EchoValhallaAudioProcessor::pushSampleToDelayFFT(float sample)
{
    if (delayFFTFifoIndex >= fftSize)
    {
        std::fill(delaySpectrumData.begin(), delaySpectrumData.end(), 0.0f);
        forwardFFT.performFrequencyOnlyForwardTransform(delayFFTInput.data());
        for (size_t i = 0; i < 512; ++i)
            delaySpectrumData[i] = delayFFTInput[i];
        delayFFTFifoIndex = 0;
    }
    delayFFTInput[delayFFTFifoIndex++] = sample;
}

void EchoValhallaAudioProcessor::pushSampleToReverbFFT(float sample)
{
    if (reverbFFTFifoIndex >= fftSize)
    {
        std::fill(reverbSpectrumData.begin(), reverbSpectrumData.end(), 0.0f);
        forwardFFT.performFrequencyOnlyForwardTransform(reverbFFTInput.data());
        for (size_t i = 0; i < 512; ++i)
            reverbSpectrumData[i] = reverbFFTInput[i];
        reverbFFTFifoIndex = 0;
    }
    reverbFFTInput[reverbFFTFifoIndex++] = sample;
}

void EchoValhallaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels < 2) return;

    // Retrieve host DAW tempo if available
    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (position->getBpm().hasValue())
            {
                delayEngine.updateTempo(*position->getBpm());
            }
        }
    }

    // Get current parameter values
    float dryWet = apvts.getRawParameterValue("masterDryWet")->load();
    float outputGainDb = apvts.getRawParameterValue("masterOutputGain")->load();
    int routingMode = static_cast<int>(apvts.getRawParameterValue("masterRouting")->load());
    bool autoGain = apvts.getRawParameterValue("masterAutoGain")->load() > 0.5f;
    bool freeze = apvts.getRawParameterValue("masterFreeze")->load() > 0.5f;
    bool stutter = apvts.getRawParameterValue("masterStutter")->load() > 0.5f;

    // Update Delay Engine Parameters
    float delayTime = apvts.getRawParameterValue("delayTime")->load();
    float delayFeedback = freeze ? 0.95f : apvts.getRawParameterValue("delayFeedback")->load();
    auto panMode = static_cast<DelayPanMode>(static_cast<int>(apvts.getRawParameterValue("delayPanMode")->load()));
    auto delayModel = static_cast<DelayCharacterModel>(static_cast<int>(apvts.getRawParameterValue("delayModel")->load()));
    bool reverse = apvts.getRawParameterValue("delayReverse")->load() > 0.5f;
    float delayPitch = apvts.getRawParameterValue("delayPitch")->load();
    float delayDucking = apvts.getRawParameterValue("delayDucking")->load();
    float delaySat = apvts.getRawParameterValue("delaySatDrive")->load();
    bool delayPreEQ = apvts.getRawParameterValue("delayPreEQ")->load() > 0.5f;

    delayEngine.setParams(delayTime, delayFeedback, panMode, delayModel, reverse, delayPitch, delayDucking, delaySat, delayPreEQ);

    // Update Reverb Engine Parameters
    auto reverbAlgo = static_cast<ReverbAlgorithm>(static_cast<int>(apvts.getRawParameterValue("reverbAlgo")->load()));
    auto reverbEra = static_cast<ReverbEra>(static_cast<int>(apvts.getRawParameterValue("reverbEra")->load()));
    float reverbDecay = freeze ? 60.0f : apvts.getRawParameterValue("reverbDecay")->load();
    float reverbSize = apvts.getRawParameterValue("reverbSize")->load();
    float reverbPreDelay = apvts.getRawParameterValue("reverbPreDelay")->load();
    float reverbDiffH = apvts.getRawParameterValue("reverbDiffHigh")->load();
    float reverbDiffL = apvts.getRawParameterValue("reverbDiffLow")->load();
    float reverbModR = apvts.getRawParameterValue("reverbModRate")->load();
    float reverbModD = apvts.getRawParameterValue("reverbModDepth")->load();
    bool reverbPreEQ = apvts.getRawParameterValue("reverbPreEQ")->load() > 0.5f;

    reverbEngine.setParams(reverbAlgo, reverbEra, reverbDecay, reverbSize, reverbPreDelay, reverbDiffH, reverbDiffL, reverbModR, reverbModD, reverbPreEQ);

    // FX Bus Global Drive
    float globalDrive = apvts.getRawParameterValue("globalDrive")->load();
    fxBus.setGlobalDrive(globalDrive);

    // Save Dry Copy
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // Copy Dry to Delay Buffer
    for (int ch = 0; ch < numChannels; ++ch)
        delayBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // Routing Logic
    if (routingMode == 0) // Serial: Delay -> Reverb
    {
        fxBus.processSlotPosition(FXSlotPosition::PreDelay, delayBuffer);
        delayEngine.process(delayBuffer, dryBuffer);
        fxBus.processSlotPosition(FXSlotPosition::PostDelay, delayBuffer);

        for (int ch = 0; ch < numChannels; ++ch)
            reverbBuffer.copyFrom(ch, 0, delayBuffer, ch, 0, numSamples);

        fxBus.processSlotPosition(FXSlotPosition::PreReverb, reverbBuffer);
        reverbEngine.process(reverbBuffer);
        fxBus.processSlotPosition(FXSlotPosition::PostReverb, reverbBuffer);
    }
    else if (routingMode == 1) // Serial: Reverb -> Delay
    {
        for (int ch = 0; ch < numChannels; ++ch)
            reverbBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

        fxBus.processSlotPosition(FXSlotPosition::PreReverb, reverbBuffer);
        reverbEngine.process(reverbBuffer);
        fxBus.processSlotPosition(FXSlotPosition::PostReverb, reverbBuffer);

        for (int ch = 0; ch < numChannels; ++ch)
            delayBuffer.copyFrom(ch, 0, reverbBuffer, ch, 0, numSamples);

        fxBus.processSlotPosition(FXSlotPosition::PreDelay, delayBuffer);
        delayEngine.process(delayBuffer, dryBuffer);
        fxBus.processSlotPosition(FXSlotPosition::PostDelay, delayBuffer);

        for (int ch = 0; ch < numChannels; ++ch)
            reverbBuffer.copyFrom(ch, 0, delayBuffer, ch, 0, numSamples);
    }
    else if (routingMode == 2) // Parallel
    {
        delayEngine.process(delayBuffer, dryBuffer);

        for (int ch = 0; ch < numChannels; ++ch)
            reverbBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
        reverbEngine.process(reverbBuffer);

        for (int ch = 0; ch < numChannels; ++ch)
            reverbBuffer.addFrom(ch, 0, delayBuffer, ch, 0, numSamples, 0.5f);
    }
    else // Crossover
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            crossoverLowBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
            crossoverHighBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
        }

        // Lows -> Reverb, Highs -> Delay
        reverbEngine.process(crossoverLowBuffer);
        delayEngine.process(crossoverHighBuffer, dryBuffer);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            reverbBuffer.copyFrom(ch, 0, crossoverLowBuffer, ch, 0, numSamples);
            reverbBuffer.addFrom(ch, 0, crossoverHighBuffer, ch, 0, numSamples);
        }
    }

    // Global FX Bus & Drive
    fxBus.processSlotPosition(FXSlotPosition::GlobalWet, reverbBuffer);
    fxBus.processGlobalDrive(reverbBuffer);

    // Stutter Glitch Effect
    if (stutter)
    {
        int stBufSize = stutterBuffer.getNumSamples();
        if (stBufSize > 0)
        {
            for (int s = 0; s < numSamples; ++s)
            {
                if (stutterLengthSamples > 0)
                {
                    int rPos = stutterReadPos % stutterLengthSamples;
                    for (int ch = 0; ch < numChannels; ++ch)
                    {
                        reverbBuffer.setSample(ch, s, stutterBuffer.getSample(ch, rPos));
                    }
                    stutterReadPos++;
                }
            }
        }
    }
    else
    {
        // Capture into stutter buffer continuously when stutter is inactive
        int stBufSize = stutterBuffer.getNumSamples();
        if (stBufSize > 0)
        {
            for (int s = 0; s < numSamples; ++s)
            {
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    stutterBuffer.setSample(ch, stutterWritePos, reverbBuffer.getSample(ch, s));
                }
                stutterWritePos = (stutterWritePos + 1) % stBufSize;
            }
        }
        stutterReadPos = 0;
    }

    // Feed FFT Visualizers
    for (int s = 0; s < numSamples; ++s)
    {
        pushSampleToDelayFFT(delayBuffer.getSample(0, s));
        pushSampleToReverbFFT(reverbBuffer.getSample(0, s));
    }

    // Auto Gain compensation calculation (only when explicitly enabled)
    float autoGainComp = 1.0f;
    if (autoGain)
    {
        float wetRms = reverbBuffer.getRMSLevel(0, 0, numSamples);
        if (wetRms > 0.5f) autoGainComp = 0.5f / wetRms;
    }

    // Master Output Mix: Dry + Wet
    float outputGain = juce::Decibels::decibelsToGain(outputGainDb) * autoGainComp;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* outData = buffer.getWritePointer(ch);
        const float* dryData = dryBuffer.getReadPointer(ch);
        const float* wetData = reverbBuffer.getReadPointer(ch);

        for (int s = 0; s < numSamples; ++s)
        {
            float mixed = ((dryData[s] * (1.0f - dryWet)) + (wetData[s] * dryWet)) * outputGain;
            outData[s] = juce::jlimit(-1.0f, 1.0f, mixed);
        }
    }
}

void EchoValhallaAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void EchoValhallaAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessorEditor* EchoValhallaAudioProcessor::createEditor()
{
    return new EchoValhallaAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EchoValhallaAudioProcessor();
}

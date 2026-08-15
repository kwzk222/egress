#pragma once

#include <JuceHeader.h>
#include "../DSP/ParametricEQ.h"

class EQVisualizer : public juce::Component, public juce::Timer
{
public:
    EQVisualizer(ParametricEQ& eqToVisualize, std::array<float, 512>& spectrumToVisualize)
        : eq(eqToVisualize), spectrumData(spectrumToVisualize)
    {
        startTimerHz(30); // 30 FPS update rate
    }

    ~EQVisualizer() override
    {
        stopTimer();
    }

    void timerCallback() override
    {
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Dark background with grid
        g.setColour(juce::Colour(0xff181c22));
        g.fillRect(bounds);

        g.setColour(juce::Colour(0xff282e38));
        for (float freq : { 100.0f, 500.0f, 1000.0f, 5000.0f, 10000.0f })
        {
            float x = bounds.getWidth() * (std::log10(freq / 20.0f) / std::log10(20000.0f / 20.0f));
            g.drawVerticalLine(static_cast<int>(x), 0.0f, bounds.getHeight());
        }

        // Spectrum FFT Overlay
        juce::Path spectrumPath;
        spectrumPath.startNewSubPath(0, bounds.getHeight());

        for (size_t i = 1; i < 512; ++i)
        {
            float freq = static_cast<float>(i) * 20000.0f / 512.0f;
            float x = bounds.getWidth() * (std::log10(freq / 20.0f) / std::log10(20000.0f / 20.0f));
            float mag = spectrumData[i];
            float y = bounds.getHeight() * (1.0f - juce::jlimit(0.0f, 1.0f, mag / 10.0f));
            spectrumPath.lineTo(x, y);
        }
        spectrumPath.lineTo(bounds.getWidth(), bounds.getHeight());
        g.setColour(juce::Colour(0x3000d2ff));
        g.fillPath(spectrumPath);

        // EQ Response Curve
        juce::Path eqPath;
        bool firstPoint = true;

        for (int px = 0; px < bounds.getWidth(); px += 2)
        {
            float normX = static_cast<float>(px) / bounds.getWidth();
            float freq = 20.0f * std::pow(20000.0f / 20.0f, normX);
            float mag = eq.getMagnitudeForFrequency(freq, 44100.0);
            float db = juce::Decibels::gainToDecibels(mag, -24.0f);

            // Scale dB (-24dB to +24dB) to component height
            float normY = 0.5f - (db / 48.0f);
            float py = bounds.getHeight() * juce::jlimit(0.0f, 1.0f, normY);

            if (firstPoint)
            {
                eqPath.startNewSubPath(static_cast<float>(px), py);
                firstPoint = false;
            }
            else
            {
                eqPath.lineTo(static_cast<float>(px), py);
            }
        }

        g.setColour(juce::Colour(0xff00d2ff));
        g.strokePath(eqPath, juce::PathStrokeType(2.5f));
    }

private:
    ParametricEQ& eq;
    std::array<float, 512>& spectrumData;
};

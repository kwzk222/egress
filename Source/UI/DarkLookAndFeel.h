#pragma once

#include <JuceHeader.h>

class DarkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DarkLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff121418));
        setColour(juce::Slider::thumbColourId, juce::Colour(0xff00d2ff));
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00d2ff));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff222830));
        setColour(juce::Label::textColourId, juce::Colour(0xffe0e6ed));
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e232a));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00d2ff));
        setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe0e6ed));
        setColour(juce::TextButton::textColourOnId, juce::Colour(0xff121418));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1e232a));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xffe0e6ed));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff343c48));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        auto radius = (float) juce::jmin(width / 2, height / 2) - 4.0f;
        auto centreX = (float) x + (float) width * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Background arc
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(slider.findColour(juce::Slider::rotarySliderOutlineColourId));
        g.strokePath(backgroundArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Value arc
        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
        g.strokePath(valueArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Knob dial
        g.setColour(juce::Colour(0xff1a1d24));
        g.fillEllipse(rx + 4, ry + 4, rw - 8, rw - 8);

        // Indicator line
        juce::Path p;
        auto pointerLength = radius * 0.6f;
        auto pointerThickness = 2.5f;
        p.addRectangle(-pointerThickness * 0.5f, -radius + 4, pointerThickness, pointerLength);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

        g.setColour(slider.findColour(juce::Slider::thumbColourId));
        g.fillPath(p);
    }
};

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct RotaryLookAndFeel : juce::LookAndFeel_V4
{
  void drawRotarySlider (juce::Graphics&, // from juce::LookAndFeel_V4 class line 206
                        int x, int y, int width, int height,
                        float sliderPosProportional,
                        float rotaryStartAngle,
                        float rotaryEndAngle,
                        juce::Slider&) override;
};

struct RotarySliderWithLabels : juce::Slider
{
  RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) : 
  juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox),
  param(&rap),
  suffix(unitSuffix)
  {
    setLookAndFeel(&lnf);
  }

  // setLookAndFeel, so unset it with destructor
  ~RotarySliderWithLabels()
  {
    setLookAndFeel(nullptr);
  }

  struct LabelPos
  {
    float pos;
    juce::String label;
  };

  juce::Array<LabelPos> labels;

  void paint(juce::Graphics& g) override;
  juce::Rectangle<int> getSliderBounds() const;
  int getTextHeight() const { return 14; }
  juce::String getDisplayString() const;
private:
  RotaryLookAndFeel lnf; // Calling this "LookAndFeel" throws ambiguous symbol error as could be juce::LookAndFeel

  juce::RangedAudioParameter* param;
  juce::String suffix;
};

struct ResponseCurveComponent: juce::Component,
juce::AudioProcessorParameter::Listener,
juce::Timer
{

  ResponseCurveComponent(EQPluginAudioProcessor&);
  ~ResponseCurveComponent();

  // callbacks from Listener and Timer definitions
  void parameterValueChanged (int parameterIndex, float newValue) override;

  void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override { }

  void timerCallback() override;

  void paint(juce::Graphics& g) override;
  void resized() override;
private:
    EQPluginAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged { false };
    
    MonoChain monoChain;

    void updateChain();

    juce::Image background;

};

//==============================================================================
/**
*/
class EQPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    EQPluginAudioProcessorEditor (EQPluginAudioProcessor&);
    ~EQPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.

    EQPluginAudioProcessor& audioProcessor;

    // attachment for widget needs to go before widget declaration so the attachment is destroyed before the widget

    RotarySliderWithLabels 
    peakFreqSlider,
    peakGainSlider,
    peakQualitySlider,
    highPassSlider,
    lowPassSlider,
    lowCutSlopeSlider,
    highCutSlopeSlider;

    ResponseCurveComponent responseCurveComponent;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment
    peakFreqSliderAttachment,
    peakGainSliderAttachment,
    peakQualitySliderAttachment,
    highPassSliderAttachment,
    lowPassSliderAttachment,
    lowCutSlopeSliderAttachment,
    highCutSlopeSliderAttachment;

    std::vector<juce::Component*> getComps();

    // My layout
    // juce::Slider lowcutSlider;
    // std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowcutSliderAttachment;
    //juce::Label lowcutLabel;

    // juce::Slider highcutSlider;
    // std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highcutSliderAttachment;
    //juce::Label highcutLabel;

    // juce::Slider peakFreqSlider;
    // std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakFreqSliderAttachment;
    //juce::Label peakFreqLabel;

    // juce::Slider peakGainSlider;
    // std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakGainSliderAttachment;
    //juce::Label peakGainLabel;

    // juce::Slider peakQualitySlider;
    // std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakQualitySliderAttachment;
    //juce::Label peakQualityLabel;

    juce::ComboBox lowcutCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lowcutComboAttachment;
    //juce::Label lowcutComboLabel;

    juce::ComboBox highcutCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> highcutComboAttachment;
    //juce::Label highcutComboLabel;

    RotaryLookAndFeel lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQPluginAudioProcessorEditor)
};

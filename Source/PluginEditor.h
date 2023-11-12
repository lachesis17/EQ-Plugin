/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySilder : juce::Slider
{
  CustomRotarySilder() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
  {

  }
};

//==============================================================================
/**
*/
class EQPluginAudioProcessorEditor  : public juce::AudioProcessorEditor,
juce::AudioProcessorParameter::Listener, 
juce::Timer
{
public:
    EQPluginAudioProcessorEditor (EQPluginAudioProcessor&);
    ~EQPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // from Listener definition
    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override { };
    
    // from Timer definiton
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.

    EQPluginAudioProcessor& audioProcessor;

    juce::Atomic<bool> parametersChanged { false };

    // attachment for widget needs to go before widget declaration so the attachment is destroyed before the widget

    CustomRotarySilder 
    peakFreqSlider,
    peakGainSlider,
    peakQualitySlider,
    highPassSlider,
    lowPassSlider,
    lowCutSlopeSlider,
    highCutSlopeSlider;

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
    juce::Label lowcutLabel;

    // juce::Slider highcutSlider;
    // std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highcutSliderAttachment;
    juce::Label highcutLabel;

    // juce::Slider peakFreqSlider;
    // std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakFreqSliderAttachment;
    juce::Label peakFreqLabel;

    // juce::Slider peakGainSlider;
    // std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakGainSliderAttachment;
    juce::Label peakGainLabel;

    // juce::Slider peakQualitySlider;
    // std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakQualitySliderAttachment;
    juce::Label peakQualityLabel;

    juce::ComboBox lowcutCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lowcutComboAttachment;
    juce::Label lowcutComboLabel;

    juce::ComboBox highcutCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> highcutComboAttachment;
    juce::Label highcutComboLabel;

    MonoChain monoChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQPluginAudioProcessorEditor)
};

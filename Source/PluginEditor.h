/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

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

    // attachment for widget needs to go before widget declaration so the attachment is destroyed before the widget

    juce::Slider lowcutSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowcutSliderAttachment;
    juce::Label lowcutLabel;

    juce::Slider highcutSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highcutSliderAttachment;
    juce::Label highcutLabel;

    juce::Slider peakFreqSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakFreqSliderAttachment;
    juce::Label peakFreqLabel;

    juce::Slider peakGainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakGainSliderAttachment;
    juce::Label peakGainLabel;

    juce::Slider peakQualitySlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakQualitySliderAttachment;
    juce::Label peakQualityLabel;

    juce::ComboBox lowcutCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lowcutComboAttachment;
    juce::Label lowcutComboLabel;

    juce::ComboBox highcutCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> highcutComboAttachment;
    juce::Label highcutComboLabel;

    EQPluginAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQPluginAudioProcessorEditor)
};

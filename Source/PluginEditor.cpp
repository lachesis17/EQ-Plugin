/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EQPluginAudioProcessorEditor::EQPluginAudioProcessorEditor (EQPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    
    lowcutSlider.setSliderStyle (juce::Slider::SliderStyle::LinearHorizontal);
    lowcutSlider.setTextBoxStyle (juce::Slider::TextBoxRight, true, 100, 50);
    lowcutSlider.setTextValueSuffix (" Hz");
    addAndMakeVisible (lowcutSlider);
    addAndMakeVisible (lowcutLabel);
    lowcutLabel.setFont(juce::Font{"Segoe UI", 18.f, 0});
    lowcutLabel.setText ("High Pass", juce::dontSendNotification);
    lowcutLabel.attachToComponent (&lowcutSlider, true);
    lowcutSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "LowCut Freq", lowcutSlider);     // allocate memory with make_unique

    highcutSlider.setSliderStyle (juce::Slider::SliderStyle::LinearHorizontal);
    highcutSlider.setTextBoxStyle (juce::Slider::TextBoxRight, true, 100, 50);
    highcutSlider.setTextValueSuffix (" Hz");
    addAndMakeVisible (highcutSlider);
    addAndMakeVisible (highcutLabel);
    highcutLabel.setFont(juce::Font{"Segoe UI", 18.f, 0});
    highcutLabel.setText ("Low Pass", juce::dontSendNotification);
    highcutLabel.attachToComponent (&highcutSlider, true);
    highcutSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "HighCut Freq", highcutSlider);

    peakFreqSlider.setSliderStyle (juce::Slider::SliderStyle::LinearHorizontal);
    peakFreqSlider.setTextBoxStyle (juce::Slider::TextBoxRight, true, 100, 50);
    peakFreqSlider.setTextValueSuffix (" Hz");
    addAndMakeVisible (peakFreqSlider);
    addAndMakeVisible (peakFreqLabel);
    peakFreqLabel.setText ("Peak Freq", juce::dontSendNotification);
    peakFreqLabel.attachToComponent (&peakFreqSlider, true);
    peakFreqSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Peak Freq", peakFreqSlider);

    peakGainSlider.setSliderStyle (juce::Slider::SliderStyle::LinearHorizontal);
    peakGainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, true, 100, 50);
    peakGainSlider.setTextValueSuffix (" dB");
    addAndMakeVisible (peakGainSlider);
    addAndMakeVisible (peakGainLabel);
    peakGainLabel.setText ("Peak Gain", juce::dontSendNotification);
    peakGainLabel.attachToComponent (&peakGainSlider, true);
    peakGainSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Peak Gain", peakGainSlider);

    peakQualitySlider.setSliderStyle (juce::Slider::SliderStyle::LinearHorizontal);
    peakQualitySlider.setTextBoxStyle (juce::Slider::TextBoxRight, true, 100, 50);
    addAndMakeVisible (peakQualitySlider);
    addAndMakeVisible (peakQualityLabel);
    peakQualityLabel.setText ("Peak Quality", juce::dontSendNotification);
    peakQualityLabel.attachToComponent (&peakQualitySlider, true);
    peakQualitySliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Peak Quality", peakQualitySlider);

    juce::StringArray stringArray;
    for(int i = 0; i < 4; i++) {
        juce::String str;
        // Order for pass filters { order = i + 1 * 2 }
        str << (12 + i * 12);
        str << " db/Oct";
        stringArray.add(str);
    }
    
    addAndMakeVisible (lowcutCombo);
    addAndMakeVisible (lowcutComboLabel);
    lowcutCombo.addItemList(stringArray, 1);
    lowcutComboLabel.setText ("Low Slope", juce::dontSendNotification);
    lowcutComboLabel.attachToComponent (&lowcutCombo, true);
    lowcutComboAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "LowCut Slope", lowcutCombo);

    addAndMakeVisible (highcutCombo);
    addAndMakeVisible (highcutComboLabel);
    highcutCombo.addItemList(stringArray, 1);
    highcutComboLabel.setText ("High Slope", juce::dontSendNotification);
    highcutComboLabel.attachToComponent (&highcutCombo, true);
    highcutComboAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "HighCut Slope", highcutCombo);

    setSize (700, 550);
}

EQPluginAudioProcessorEditor::~EQPluginAudioProcessorEditor()
{
}

//==============================================================================
void EQPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    // g.fillAll (juce::Colours::lightgreen);
    // g.fillAll(Colour(23, 0, 62));

    g.setGradientFill(ColourGradient (Colour(23, 0, 62), 0.125*(float) getWidth(), 0.125*(float) getHeight(),
                                       Colour(0, 0, 0), 0.875*(float) getWidth(), 0.875*(float) getHeight(), true));
    g.fillAll();

    // g.setColour (juce::Colours::white);
    // g.setColour (Colour(44, 0, 107)); 
    // g.setFont (100.0f);
    // g.drawFittedText ("EQ in VST3!", getLocalBounds(), juce::Justification::centred, 1);
}

void EQPluginAudioProcessorEditor::resized()
{
    lowcutSlider.setBounds(getWidth() / 2 - 200, getHeight() / 2 - 250, 500, 75);
    highcutSlider.setBounds(getWidth() / 2 - 200, getHeight() / 2 - 175, 500, 75);
    peakFreqSlider.setBounds(getWidth() / 2 - 200, getHeight() / 2 - 100, 500, 75);
    peakGainSlider.setBounds(getWidth() / 2 - 200, getHeight() / 2 - 25, 500, 75);
    peakQualitySlider.setBounds(getWidth() / 2 - 200, getHeight() / 2 + 50, 500, 75);
    lowcutCombo.setBounds(getWidth() / 2 - 200, getHeight() / 2 + 150, 400, 25);
    highcutCombo.setBounds(getWidth() / 2 - 200, getHeight() / 2 + 200, 400, 25);

    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

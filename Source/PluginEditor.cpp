/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void RotaryLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPosProportional,
float rotaryStartAngle, float rotaryEndAngle, juce::Slider & slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    g.setColour(Colour(63u, 11u, 74u));
    g.fillEllipse(bounds);

    g.setColour(Colour(250u, 250u, 250u));
    g.drawEllipse(bounds, 1.f);    

    if(auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

        p.addRoundedRectangle(r, 2.f);
        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);
        
        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(center);

        g.setColour(Colours::black);
        g.fillRect(r);

        g.setColour(Colours::white);
        g.drawFittedText(text, r.toNearestInt(), Justification::centred, 1);
    }


}
//==============================================================================
void RotarySliderWithLabels::paint(juce::Graphics &g)
{
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    // g.setColour(Colours::red);
    // g.drawRect(getLocalBounds());
    // g.setColour(Colours::yellow);
    // g.drawRect(sliderBounds);

    getLookAndFeel().drawRotarySlider(g, sliderBounds.getX(), sliderBounds.getY(), sliderBounds.getWidth(), sliderBounds.getHeight(), 
    jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0), startAng, endAng, *this);

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colour(0u, 172u, 1u));
    //g.setColour(Colour(172u, 79u, 15u));
    g.setFont(getTextHeight());
    

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(1.f <= pos);

        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang); //angling away from text label to corner of bbox

        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());

        g.drawFittedText(str, r.toNearestInt(), Justification::centred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 2;

    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    //return juce::String(getValue());
    if(auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();

    juce::String str;
    bool addK = false;

    if(auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = getValue();
        if(val > 999.f)
        {
            val /= 1000.f;
            addK = true;
        }

        str = juce::String(val, (addK ? 2 : 0));
    }
    else
    {
        jassertfalse;
    }

    if(suffix.isNotEmpty())
    {
        str << " ";
        if(addK)
            str << "k";
        
        str << suffix;
    }

    return str;
}
//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(EQPluginAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param: params)
    {
        param->addListener(this);
    }

    updateChain();

    startTimerHz(120);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param: params)
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if( parametersChanged.compareAndSetBool(false, true))
    {
        // DBG( "params changed"); // debug
        // update monochain
        updateChain();
        // signal repaint
        repaint();
    }
}

void ResponseCurveComponent::updateChain() 
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());

    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    g.fillAll (juce::Colours::black);

    auto responseArea = getLocalBounds();
    auto w = responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;

    mags.resize(w);
    
    // setting the magnitude of frequency per pixel of the gui's bounds
    for (int i = 0; i < w; ++i)
    {
        // gain not db
        double mag = 1.f;
        // get magnitude of pixel mapped from 'pixel' space to 'freq' space, map normalized pixel num to freq in hearing range
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

        if (! monoChain.isBypassed<ChainPositions::Peak>() )
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (! lowcut.isBypassed<0>() )
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (! lowcut.isBypassed<1>() )
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (! lowcut.isBypassed<2>() )
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (! lowcut.isBypassed<3>() )
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (! highcut.isBypassed<0>() )
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (! highcut.isBypassed<1>() )
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (! highcut.isBypassed<2>() )
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (! highcut.isBypassed<3>() )
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        mags[i] = Decibels::gainToDecibels(mag);
    }

    // convert vector of mags into a path

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for (size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]) );
    };

    g.setColour(Colours::purple);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

//==============================================================================
EQPluginAudioProcessorEditor::EQPluginAudioProcessorEditor (EQPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
    peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"), 
    peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""), 
    highPassSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"), 
    lowPassSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"), 
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"), 
    highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),

    responseCurveComponent(audioProcessor),
    peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider), 
    peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider), 
    peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider), 
    highPassSliderAttachment(audioProcessor.apvts, "LowCut Freq", highPassSlider), 
    lowPassSliderAttachment(audioProcessor.apvts, "HighCut Freq", lowPassSlider), 
    lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider), 
    highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)
{


    // highPassSlider.setTextValueSuffix (" Hz");
    // lowPassSlider.setTextValueSuffix (" Hz");

    // addAndMakeVisible(lowcutLabel);
    // lowcutLabel.setFont(juce::Font{"Segoe UI", 18.f, 0});
    // lowcutLabel.setText ("High Pass", juce::dontSendNotification);
    // lowcutLabel.attachToComponent (&highPassSlider, false);

    // addAndMakeVisible(highcutLabel);
    // highcutLabel.setFont(juce::Font{"Segoe UI", 18.f, 0});
    // highcutLabel.setText ("Low Pass", juce::dontSendNotification);
    // highcutLabel.attachToComponent (&lowPassSlider, false);

    // addAndMakeVisible (peakFreqLabel);
    // peakFreqLabel.setText ("Peak Freq", juce::dontSendNotification);
    // peakFreqLabel.attachToComponent (&peakFreqSlider, false);

    // addAndMakeVisible (peakGainLabel);
    // peakGainLabel.setText ("Peak Gain", juce::dontSendNotification);
    // peakGainLabel.attachToComponent (&peakGainSlider, false);

    // addAndMakeVisible (peakQualityLabel);
    // peakQualityLabel.setText ("Peak Quality", juce::dontSendNotification);
    // peakQualityLabel.attachToComponent (&peakQualitySlider, false);

    // addAndMakeVisible (lowcutComboLabel);
    // lowcutComboLabel.setText ("Low Slope", juce::dontSendNotification);
    // lowcutComboLabel.attachToComponent (&lowCutSlopeSlider, false);

    // addAndMakeVisible (highcutComboLabel);
    // highcutComboLabel.setText ("High Slope", juce::dontSendNotification);
    // highcutComboLabel.attachToComponent (&highCutSlopeSlider, false);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    // My layout
    // lowcutSlider.setSliderStyle (juce::Slider::SliderStyle::LinearHorizontal);
    // lowcutSlider.setTextBoxStyle (juce::Slider::TextBoxRight, true, 100, 50);
    // lowcutSlider.setTextValueSuffix (" Hz");
    // addAndMakeVisible (lowcutSlider);
    // addAndMakeVisible (lowcutLabel);
    // lowcutLabel.setFont(juce::Font{"Segoe UI", 18.f, 0});
    // lowcutLabel.setText ("High Pass", juce::dontSendNotification);
    // lowcutLabel.attachToComponent (&lowcutSlider, true);
    // lowcutSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "LowCut Freq", lowcutSlider);     // allocate memory with make_unique

    // highcutSlider.setSliderStyle (juce::Slider::SliderStyle::LinearHorizontal);
    // highcutSlider.setTextBoxStyle (juce::Slider::TextBoxRight, true, 100, 50);
    // highcutSlider.setTextValueSuffix (" Hz");
    // addAndMakeVisible (highcutSlider);
    // addAndMakeVisible (highcutLabel);
    // highcutLabel.setFont(juce::Font{"Segoe UI", 18.f, 0});
    // highcutLabel.setText ("Low Pass", juce::dontSendNotification);
    // highcutLabel.attachToComponent (&highcutSlider, true);
    // highcutSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "HighCut Freq", highcutSlider);

    // peakFreqSlider.setSliderStyle (juce::Slider::SliderStyle::LinearHorizontal);
    // peakFreqSlider.setTextBoxStyle (juce::Slider::TextBoxRight, true, 100, 50);
    // peakFreqSlider.setTextValueSuffix (" Hz");
    // addAndMakeVisible (peakFreqSlider);
    // addAndMakeVisible (peakFreqLabel);
    // peakFreqLabel.setText ("Peak Freq", juce::dontSendNotification);
    // peakFreqLabel.attachToComponent (&peakFreqSlider, true);
    // peakFreqSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Peak Freq", peakFreqSlider);

    // peakGainSlider.setSliderStyle (juce::Slider::SliderStyle::LinearHorizontal);
    // peakGainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, true, 100, 50);
    // peakGainSlider.setTextValueSuffix (" dB");
    // addAndMakeVisible (peakGainSlider);
    // addAndMakeVisible (peakGainLabel);
    // peakGainLabel.setText ("Peak Gain", juce::dontSendNotification);
    // peakGainLabel.attachToComponent (&peakGainSlider, true);
    // peakGainSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Peak Gain", peakGainSlider);

    // peakQualitySlider.setSliderStyle (juce::Slider::SliderStyle::LinearHorizontal);
    // peakQualitySlider.setTextBoxStyle (juce::Slider::TextBoxRight, true, 100, 50);
    // addAndMakeVisible (peakQualitySlider);
    // addAndMakeVisible (peakQualityLabel);
    // peakQualityLabel.setText ("Peak Quality", juce::dontSendNotification);
    // peakQualityLabel.attachToComponent (&peakQualitySlider, true);
    // peakQualitySliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Peak Quality", peakQualitySlider);

    peakFreqSlider.labels.add({0.f, "20Hz"});
    peakFreqSlider.labels.add({1.f, "20kHz"});

    peakGainSlider.labels.add({0.f, "-24dB"});
    peakGainSlider.labels.add({1.f, "+24dB"});

    peakQualitySlider.labels.add({0.f, "0.1"});
    peakQualitySlider.labels.add({1.f, "10.0"});

    highPassSlider.labels.add({0.f, "20Hz"});
    highPassSlider.labels.add({1.f, "20kHz"});

    lowPassSlider.labels.add({0.f, "20Hz"});
    lowPassSlider.labels.add({1.f, "20kHz"});

    lowCutSlopeSlider.labels.add({0.f, "12"});
    lowCutSlopeSlider.labels.add({1.f, "48"});
    
    highCutSlopeSlider.labels.add({0.f, "12"});
    highCutSlopeSlider.labels.add({1.f, "48"});

    for( auto* comp: getComps())
    {
        addAndMakeVisible(comp);
    }

    juce::StringArray stringArray;
    for(int i = 0; i < 4; i++) {
        juce::String str;
        // Order for pass filters { order = i + 1 * 2 }
        str << (12 + i * 12);
        str << " db/Oct";
        stringArray.add(str);
    }
    
    // addAndMakeVisible (lowcutCombo);
    // addAndMakeVisible (lowcutComboLabel);
    // lowcutCombo.addItemList(stringArray, 1);
    // lowcutComboLabel.setText ("Low Slope", juce::dontSendNotification);
    // lowcutComboLabel.attachToComponent (&lowcutCombo, true);
    // lowcutComboAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "LowCut Slope", lowcutCombo);

    // addAndMakeVisible (highcutCombo);
    // addAndMakeVisible (highcutComboLabel);
    // highcutCombo.addItemList(stringArray, 1);
    // highcutComboLabel.setText ("High Slope", juce::dontSendNotification);
    // highcutComboLabel.attachToComponent (&highcutCombo, true);
    // highcutComboAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "HighCut Slope", highcutCombo);

    setSize (750, 500);
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
    // g.setColour (juce::Colours::white);
    // g.setColour (Colour(44, 0, 107)); 
    // g.setFont (100.0f);
    // g.drawFittedText ("EQ in VST3!", getLocalBounds(), juce::Justification::centred, 1);

    using namespace juce;
    g.setGradientFill(ColourGradient (Colour(23, 0, 62), 0.125*(float) getWidth(), 0.125*(float) getHeight(),
                                       Colour(0, 0, 0), 0.875*(float) getWidth(), 0.875*(float) getHeight(), true));
    g.fillAll();
}

void EQPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    float hRatio = 30.f / 100.f; //JUCE_LIVE_CONSTANT(33) / 100.f; // live values
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);

    responseCurveComponent.setBounds(responseArea);

    bounds.removeFromTop(15);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

    highPassSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);

    lowPassSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);

    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);

    // lowcutSlider.setBounds(getWidth() / 2 - 200, getHeight() / 2 - 250, 500, 75);
    // highcutSlider.setBounds(getWidth() / 2 - 200, getHeight() / 2 - 175, 500, 75);
    // peakFreqSlider.setBounds(getWidth() / 2 - 200, getHeight() / 2 - 100, 500, 75);
    // peakGainSlider.setBounds(getWidth() / 2 - 200, getHeight() / 2 - 25, 500, 75);
    // peakQualitySlider.setBounds(getWidth() / 2 - 200, getHeight() / 2 + 50, 500, 75);

    lowcutCombo.setBounds(getWidth() / 2 - 200, getHeight() / 2 + 150, 400, 25);
    highcutCombo.setBounds(getWidth() / 2 - 200, getHeight() / 2 + 200, 400, 25);

}

std::vector<juce::Component*> EQPluginAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &highPassSlider,
        &lowPassSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent
    };
}
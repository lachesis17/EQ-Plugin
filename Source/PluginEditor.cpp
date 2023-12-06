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
    g.setGradientFill(ColourGradient (Colour(Colours::darkorange), 0.175*(float) width, 0.175*(float) height,
                                       Colour(63, 11, 74), 0.75*(float) width, 0.75*(float) height, true));
    g.fillEllipse(bounds);

    g.setColour(Colour(250u, 250u, 250u));
    g.drawEllipse(bounds, 1.f);    

    if(auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2.5);
        r.setRight(center.getX() + 2.5);
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
        
        r.setSize(strWidth + 22, rswl->getTextHeight() + 10);
        r.setCentre(center);

        g.setColour(Colours::black);
        g.drawRoundedRectangle(r, 12,1);
        g.fillRoundedRectangle(r, 12);
        //g.fillRect(r);

        g.setColour(Colours::white);
        //g.setFont(Font("Tahoma", 17, 0)); // a few different ways to change font, but instead change for all labels in the lookandfeel class in header
        // juce::Font roboto_font = juce::Typeface::createSystemTypefaceFor(BinaryData::RobotoRegular_ttf, BinaryData::RobotoRegular_ttfSize);
        // g.setFont(Font(roboto_font.getTypefaceName(), 15, 1));
        // const juce::Typeface::Ptr typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::RobotoRegular_ttf,BinaryData::RobotoRegular_ttfSize);
        // g.setFont(juce::Font(typeface).withHeight(17.0f));
        const juce::Typeface::Ptr typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::Orbitron_ttf, BinaryData::Orbitron_ttfSize);
        g.setFont(juce::Font(typeface).withHeight(15.5f)); // slider labels
        g.drawFittedText(text, r.toNearestInt(), Justification::centred, 1);
    }
}

void RotaryLookAndFeel::drawToggleButton(juce::Graphics &g,
                        juce::ToggleButton &toggleButton, 
                        bool shouldDrawButtonAsHighlighted, 
                        bool shouldDrawButtonAsDown)
{
    using namespace juce; // Projucer->Settings->Add "using namespace juce" to JuceHeader.h

    Path powerButton;

    auto bounds = toggleButton.getLocalBounds();
    //bounds.removeFromBottom(10);
    bounds.removeFromRight(90);
    bounds.removeFromLeft(90);
    // g.setColour(Colours::red);
    // g.drawRect(bounds);
    auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6; // jmin returns smaller of 2 vals, to make square a square in non-square bbox
    auto r = bounds.withSizeKeepingCentre(size, size).toFloat(); // returns a rectangle! very cool

    float ang = 30.f;

    size -= 6;

    powerButton.addCentredArc(r.getCentreX(), r.getCentreY(), size * 0.5, size * 0.5, 0.f, degreesToRadians(ang), degreesToRadians(360.f - ang), true);

    powerButton.startNewSubPath(r.getCentreX(), r.getY());
    powerButton.lineTo(r.getCentre());

    PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved); // need to use this class to customise the path strokes for the analyzer

    auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u); // need to start doing this one line if else

    g.setColour(color);
    g.strokePath(powerButton, pst);
    g.drawEllipse(r, 2);
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

    //g.setColour(Colour(172u, 79u, 15u));
    //g.setFont(getTextHeight());
    g.setColour(Colour(0u, 172u, 1u));
    g.setFont(12); // green labels
    

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
ResponseCurveComponent::ResponseCurveComponent(EQPluginAudioProcessor& p) : 
audioProcessor(p),
//leftChannelFifo(&audioProcessor.leftChannelFifo)
leftPathProducer(audioProcessor.leftChannelFifo),
rightPathProducer(audioProcessor.rightChannelFifo)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param: params)
    {
        param->addListener(this);
    }

    updateChain();

    startTimerHz(165); // refresh rate
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

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer; // create temp buffer to hold buffer if exists
    
    //while there are buffers to pull from SCF, prepapre to send to FFT generator
    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
    {
        if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
        {
            //shifting all the blocks in the buffer one to the left as they are updated in gui
            auto size = tempIncomingBuffer.getNumSamples();

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0), 
            monoBuffer.getReadPointer(0, size), 
            monoBuffer.getNumSamples() - size);

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
            tempIncomingBuffer.getReadPointer(0, 0),
            size);

            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }

    //while FTT buffers have been prepared, generate a path
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    // 48000 (sample rate) / 2048 (bins) = 23hz (bin width)
    const auto binWidth = sampleRate / (double)fftSize; // audioProcessor.getSampleRate()

    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
    {
        std::vector<float> fftData;
        if (leftChannelFFTDataGenerator.getFFTData(fftData))   
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
        }
    }

    // while there are paths, pull all available, display the most recent

    while (pathProducer.getNumPathsAvailable())
    {
        pathProducer.getPath(leftChannelFFTPath);
    }
}

void ResponseCurveComponent::timerCallback()
{
    auto fftBounds = getAnalysisArea().toFloat();
    auto sampleRate = audioProcessor.getSampleRate();

    leftPathProducer.process(fftBounds, sampleRate);
    rightPathProducer.process(fftBounds, sampleRate);

    if( parametersChanged.compareAndSetBool(false, true))
    {
        // DBG( "params changed"); // debug
        // update monochain
        updateChain();
        // signal repaint
        //repaint(); // now we need to repaint all the time instead of when params change
    }

    repaint();
}

void ResponseCurveComponent::updateChain() 
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);

    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    monoChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);

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

    g.drawImage(background, getLocalBounds().toFloat());

    auto responseArea = getAnalysisArea();

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

        if (!monoChain.isBypassed<ChainPositions::Peak>() )
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!monoChain.isBypassed<ChainPositions::LowCut>())
        {
            if (! lowcut.isBypassed<0>() )
                mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! lowcut.isBypassed<1>() )
                mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! lowcut.isBypassed<2>() )
                mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! lowcut.isBypassed<3>() )
                mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!monoChain.isBypassed<ChainPositions::HighCut>())
        {
            if (! highcut.isBypassed<0>() )
                mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! highcut.isBypassed<1>() )
                mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! highcut.isBypassed<2>() )
                mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! highcut.isBypassed<3>() )
                mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

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

    auto leftChannelFFTPath = leftPathProducer.getPath();
    leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY() - 2.5));

    // PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);
    
    g.setColour(Colours::blueviolet);
    g.strokePath(leftChannelFFTPath, PathStrokeType(2.f));
    // g.strokePath(leftChannelFFTPath, pst);

    auto rightChannelFFTPath = rightPathProducer.getPath();
    rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY() - 2.5));

    g.setColour(Colours::darkorange);
    g.strokePath(rightChannelFFTPath, PathStrokeType(2.f));
    // g.strokePath(rightChannelFFTPath, pst);
    // g.fillPath(rightChannelFFTPath); // fills the spectrum line, but the Y is messed up from the response area

    g.setColour(Colours::purple);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.5f));
}

void ResponseCurveComponent::resized()
{
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);

    Graphics g(background);

    const juce::Typeface::Ptr typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::Monomaniac_ttf, BinaryData::Monomaniac_ttfSize);
    g.setFont(juce::Font(typeface).withHeight(11.0f));

    Array<float> freqs
    {
        20, /*30, 40,*/ 50, 100,
        200, /*300, 400,*/ 500, 1000,
        2000, /*3000, 4000,*/ 5000, 10000,
        20000,
    };

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    Array<float> xs;
    for (auto f: freqs)
    {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xs.add(left + width * normX);
    }

    g.setColour(Colours::grey);
    for (auto x : xs)
    {
        g.drawVerticalLine(x, top, bottom);
    }

    Array<float> gain
    {
        -24, -12, 0, 12, 24,
    };

    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);
    }

    g.setColour(Colours::lightgrey);
    const int fontHeight = 14; // grid labels
    g.setFont(fontHeight);

    for (int i = 0; i < freqs.size(); ++i)
    {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if (f > 999.f)
        {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if (addK)
            str << "k";
        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);

        g.drawFittedText(str, r, Justification::centred, 1);
    }

    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

        String str;
        if (gDb > 0)
            str << "+";
        str << gDb;

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);

        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey);
        g.drawFittedText(str, r, Justification::centred, 1);

        str.clear();
        str << (gDb - 24.f);

        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, Justification::centred, 1);
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop(16);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);

    return bounds;
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
    highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),

    lowcutBypassButtonAttachment(audioProcessor.apvts, "LowCut Bypassed", lowcutBypassButton),
    peakBypassButtonAttachment(audioProcessor.apvts, "Peak Bypassed", peakBypassButton),
    highcutBypassButtonAttachment(audioProcessor.apvts, "HighCut Bypassed", highcutBypassButton),
    analyzerEnabledButtonAttachment(audioProcessor.apvts, "Analyzer Enabled", analyzerEnabledButton)

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

    lowcutBypassButton.setLookAndFeel(&lnf);
    peakBypassButton.setLookAndFeel(&lnf);
    highcutBypassButton.setLookAndFeel(&lnf);

    setSize (650, 650);
    setResizable(true,true);
}

EQPluginAudioProcessorEditor::~EQPluginAudioProcessorEditor()
{
    lowcutBypassButton.setLookAndFeel(nullptr);
    peakBypassButton.setLookAndFeel(nullptr);
    highcutBypassButton.setLookAndFeel(nullptr);
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
    float hRatio = 32.f / 100.f; //JUCE_LIVE_CONSTANT(33) / 100.f; // live values
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);

    responseCurveComponent.setBounds(responseArea);

    bounds.removeFromTop(15);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

    lowcutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    highPassSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);

    highcutBypassButton.setBounds(highCutArea.removeFromTop(25));
    lowPassSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);

    peakBypassButton.setBounds(bounds.removeFromTop(25));
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
        &responseCurveComponent,

        &lowcutBypassButton, 
        &peakBypassButton,
        &highcutBypassButton,
        &analyzerEnabledButton
    };
}
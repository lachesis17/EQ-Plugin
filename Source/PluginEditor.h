/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

template<typename BlockType>
struct FFTDataGenerator
{
    /**
     produces the FFT data from an audio buffer.
     */
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();
        
        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());
        
        // first apply a windowing function to our data
        window->multiplyWithWindowingTable (fftData.data(), fftSize);       // [1]
        
        // then render our FFT data..
        forwardFFT->performFrequencyOnlyForwardTransform (fftData.data());  // [2]
        
        int numBins = (int)fftSize / 2;
        
        //normalize the fft values.
        for( int i = 0; i < numBins; ++i )
        {
            auto v = fftData[i];
//            fftData[i] /= (float) numBins;
            if( !std::isinf(v) && !std::isnan(v) )
            {
                v /= float(numBins);
            }
            else
            {
                v = 0.f;
            }
            fftData[i] = v;
        }
        
        //convert them to decibels
        for( int i = 0; i < numBins; ++i )
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }
        
        fftDataFifo.push(fftData);
    }
    
    void changeOrder(FFTOrder newOrder)
    {
        //when you change order, recreate the window, forwardFFT, fifo, fftData
        //also reset the fifoIndex
        //things that need recreating should be created on the heap via std::make_unique<>
        
        order = newOrder;
        auto fftSize = getFFTSize();
        
        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);
        
        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }
    //==============================================================================
    int getFFTSize() const { return 1 << order; }
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    //==============================================================================
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    
    Fifo<BlockType> fftDataFifo;
};


template<typename PathType>
struct AnalyzerPathGenerator //lol who needs classes with structs
{
    /*
     converts 'renderData[]' into a juce::Path
     */
    void generatePath(const std::vector<float>& renderData,
                      juce::Rectangle<float> fftBounds,
                      int fftSize,
                      float binWidth,
                      float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                              negativeInfinity, 0.f,
                              float(bottom+10),   top);
        };

        auto y = map(renderData[0]);

//        jassert( !std::isnan(y) && !std::isinf(y) );
        if( std::isnan(y) || std::isinf(y) )
            y = bottom;
        
        p.startNewSubPath(0, y);

        const int pathResolution = 2; //you can draw line-to's every 'pathResolution' pixels.

        for( int binNum = 1; binNum < numBins; binNum += pathResolution )
        {
            y = map(renderData[binNum]);

//            jassert( !std::isnan(y) && !std::isinf(y) );

            if( !std::isnan(y) && !std::isinf(y) )
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                float binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};


struct RotaryLookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider (juce::Graphics&, // from juce::LookAndFeel_V4 class line 206
                        int x, int y, int width, int height,
                        float sliderPosProportional,
                        float rotaryStartAngle,
                        float rotaryEndAngle,
                        juce::Slider&) override;

  // juce::Font m_tf = juce::Typeface::createSystemTypefaceFor(BinaryData::RobotoRegular_ttf, BinaryData::RobotoRegular_ttfSize);

//   juce::Font getLabelFont(juce::Label& label) override { 
//     return juce::Font(typeface).withHeight(label.getHeight() * 1.f);
//   } // add the font to all Slider labels by overriding the getLabelFont method

    //LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypeface(juce::Font(typeface));

    void drawToggleButton (juce::Graphics &g, // line 119
                        juce::ToggleButton &toggleButton, 
                        bool shouldDrawButtonAsHighlighted, 
                        bool shouldDrawButtonAsDown) override;

private:
//   const juce::Typeface::Ptr typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::RobotoMono_ttf, BinaryData::RobotoMono_ttfSize);
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

struct PathProducer
{
    PathProducer(SingleChannelSampleFifo<EQPluginAudioProcessor::BlockType>& scsf) : leftChannelFifo(&scsf)
    {
        leftChannelFFTDataGenerator.changeOrder(FFTOrder::order8192);
        monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
    }

    void process(juce::Rectangle<float> fftBounds, double sampleRate);
    juce::Path getPath() { return leftChannelFFTPath; }

private:
    SingleChannelSampleFifo<EQPluginAudioProcessor::BlockType>* leftChannelFifo;
    
    juce::AudioBuffer<float> monoBuffer;
    
    FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;
    
    AnalyzerPathGenerator<juce::Path> pathProducer;
    
    juce::Path leftChannelFFTPath;
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

  void toggleAnalysisEnablement(bool enabled)
  {
    shouldShowFFTAnalysis = enabled;
  }
private:
    EQPluginAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged { false };
    
    MonoChain monoChain;

    void updateChain();

    juce::Image background;

    juce::Rectangle<int> getRenderArea();
    juce::Rectangle<int> getAnalysisArea();

    PathProducer leftPathProducer, rightPathProducer;

    bool shouldShowFFTAnalysis = true;
};

//==============================================================================
struct PowerButton : juce::ToggleButton {}; // init below at line 334, easiest subclass implementation of my life
struct AnalyzerButton : juce::ToggleButton
{
    void resized() override
    {
        auto bounds = getLocalBounds();
        auto insetRect = bounds.reduced(4); // reduced!! // use this!

        randomPath.clear();

        juce::Random r;

        randomPath.startNewSubPath(insetRect.getX(), insetRect.getY() + insetRect.getHeight() * r.nextFloat());

        for (auto x = insetRect.getX() + 1; x < insetRect.getRight(); x += 2)
        {
            randomPath.lineTo(x, insetRect.getY() + insetRect.getHeight() * r.nextFloat());
        }
    }

    Path randomPath; // cool random squiggley line
};
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
    highPassSlopeSlider,
    lowPassSlopeSlider;

    ResponseCurveComponent responseCurveComponent;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment
    peakFreqSliderAttachment,
    peakGainSliderAttachment,
    peakQualitySliderAttachment,
    highPassSliderAttachment,
    lowPassSliderAttachment,
    highPassSlopeSliderAttachment,
    lowPassSlopeSliderAttachment;

    PowerButton highpassBypassButton, peakBypassButton, lowpassBypassButton;
    AnalyzerButton analyzerEnabledButton;

    using ButtonAttachment = APVTS::ButtonAttachment;
    ButtonAttachment highpassBypassButtonAttachment, peakBypassButtonAttachment, lowpassBypassButtonAttachment, analyzerEnabledButtonAttachment;

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

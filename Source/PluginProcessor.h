/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope {
  Slope_12,
  Slope_24,
  Slope_36,
  Slope_48
};

// Extract params from audio processor value tree state, save it in nice data type (struct)
struct ChainSettings {
  float peakFreq {0}, peakGainInDecibels{0}, peakQuality{1.f};
  float lowCutFreq {0}, highCutFreq {0};
  Slope lowCutSlope {Slope::Slope_12}, highCutSlope {Slope::Slope_12};
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class EQPluginAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    EQPluginAudioProcessor();
    ~EQPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Create object instance of APVTS, with layout function as required *args
    // static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    // juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameterLayout()};
    juce::AudioProcessorValueTreeState apvts; //{*this, nullptr, "Parameters", createParameters()};

private:

    // Create filter type aliases to use for setting two mono chains to process in stereo
    // Peak Filter
    using Filter = juce::dsp::IIR::Filter<float>;

    // Four filters for the multiples of 12 (each filter type in IIR::Filter has 12db range) from combo box
    // In DSP, define a chain and pass it ProcessingContext values which goes through each member of chain auto
    // So put 4 filters into a chain to pass a single context and return all audio
    // CutFilter // Slope of cut filters = order
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

    // Use Peak and Cut filters to apply parametric filter
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

    // For both channels
    MonoChain leftChain, rightChain;

    // Positions of links in chain to pass in prepareToPlay() chain.get()
    enum ChainPositions {
      lowCut,
      Peak,
      HighCut
    };

    // Update the filter and coefficients instead of copy/paste switches
    void updatePeakFilter(const ChainSettings& chainSettings);
    using Coefficients = Filter::CoefficientsPtr;
    static void updateCoefficients(Coefficients& old, const Coefficients& replacements);

    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& leftLowCut, const CoefficientType& cutCoefficients, const Slope& lowCutSlope) { // const ChainSettings& chainSettings) {
      // auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, getSampleRate(), 2 * (chainSettings.lowCutSlope + 1));

      // auto& leftLowCut = leftChain.get<ChainPositions::lowCut>();

      leftLowCut.template setBypassed<0>(true);
      leftLowCut.template setBypassed<1>(true);
      leftLowCut.template setBypassed<2>(true);
      leftLowCut.template setBypassed<3>(true);

      // switch (chainSettings.lowCutSlope) {
      switch (lowCutSlope) {
          case Slope_12: {
              // still pointing to derefence, add the coefficient returned using the slope to the array
              *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
              leftLowCut.template setBypassed<0>(false);
              break;
          }
          case Slope_24:{
              *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
              leftLowCut.template setBypassed<0>(false);
              *leftLowCut.template get<1>().coefficients = *cutCoefficients[1];
              leftLowCut.template setBypassed<1>(false);
              break;
          }
          case Slope_36: {
              *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
              leftLowCut.template setBypassed<0>(false);
              *leftLowCut.template get<1>().coefficients = *cutCoefficients[1];
              leftLowCut.template setBypassed<1>(false);
              *leftLowCut.template get<2>().coefficients = *cutCoefficients[2];
              leftLowCut.template setBypassed<2>(false);
              break;
          }
          case Slope_48: {
              *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
              leftLowCut.template setBypassed<0>(false);
              *leftLowCut.template get<1>().coefficients = *cutCoefficients[1];
              leftLowCut.template setBypassed<1>(false);
              *leftLowCut.template get<2>().coefficients = *cutCoefficients[2];
              leftLowCut.template setBypassed<2>(false);
              *leftLowCut.template get<3>().coefficients = *cutCoefficients[3];
              leftLowCut.template setBypassed<3>(false);
              break;
          }
        }
      }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQPluginAudioProcessor)
};

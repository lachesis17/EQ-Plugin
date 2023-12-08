/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace std;

//==============================================================================
EQPluginAudioProcessor::EQPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts (*this, nullptr, "Parameters", createParameters())
#endif
{
}

EQPluginAudioProcessor::~EQPluginAudioProcessor()
{
}

//==============================================================================
const juce::String EQPluginAudioProcessor::getName() const
{
    // CMake to export a compile_commands.json
    // return JucePlugin_Name;
    return "";
}

bool EQPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EQPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EQPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EQPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EQPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EQPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EQPluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EQPluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void EQPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void EQPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    // Pass this ProcessSpec object to the chains which passes it down each link in the chain
    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;

    spec.numChannels = 1;

    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    // Now chain is prepared and we have the parameters from settings, make coefficients with static helper function in IIR::Coefficients
    // For the gain need to convert the decibel (db) value to gain using helper function juce::Decibels::decibelsToGain(db)

    updateFilters(); // just update the filters

    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);

    // osc.initialise([](float x) { return std::sin(x); }); // lambda? // sine wave noise
    // spec.numChannels = getTotalNumOutputChannels();
    // osc.prepare(spec);
    // osc.setFrequency(50); // making noise with sine waves!

    // auto chainSettings = getChainSettings(apvts);

    // // coefficent object = reference counted wrapper around array, allocated on the heap (oh noes)
    // updatePeakFilter(chainSettings);
    // //auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

    // // Access a link in the chain, which needs an index - use the enum defined in header file:
    // // Dereference it from the heap (stack is better for thread usage in audio as everything is in a thread so things can
    // // execute in time before the big bad buffer catches you, so heap=bad and havinghearing=nice)
    // // Cool losing virginity with pointers to derefence from heap
    // //*leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    // //*rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    // // Now peak filter is set up and will make changes to audio but sliders are not doing anything until values are returned from widget listeners in ProcessBlock()

    // auto lowcutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, sampleRate, 2 * (chainSettings.lowCutSlope + 1));

    // auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    // updateCutFilter(leftLowCut, lowcutCoefficients, chainSettings.lowCutSlope);

    // auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    // updateCutFilter(rightLowCut, lowcutCoefficients, chainSettings.lowCutSlope);

    // auto highcutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, sampleRate, 2 * (chainSettings.highCutSlope + 1));

    // auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    // updateCutFilter(leftHighCut, highcutCoefficients, chainSettings.highCutSlope);

    // auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    // updateCutFilter(rightHighCut, highcutCoefficients, chainSettings.highCutSlope);

}

void EQPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EQPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void EQPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // To make the GUI widgets affect the audio we need to update the parameters BEFORE processing audio through them
    // auto chainSettings = getChainSettings(apvts);

    // updatePeakFilter(chainSettings);

    // // auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

    // // *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    // // *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

    // auto lowcutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, getSampleRate(), 2 * (chainSettings.lowCutSlope + 1));

    // auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    // updateCutFilter(leftLowCut, lowcutCoefficients, chainSettings.lowCutSlope);

    // auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    // updateCutFilter(rightLowCut, lowcutCoefficients, chainSettings.lowCutSlope);

    // auto highcutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, getSampleRate(), 2 * (chainSettings.highCutSlope + 1));

    // auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    // updateCutFilter(leftHighCut, highcutCoefficients, chainSettings.highCutSlope);

    // auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    // updateCutFilter(rightHighCut, highcutCoefficients, chainSettings.highCutSlope);

    updateFilters();


    // Chain needs a ProcessingContext to be passed to run audio through links in chain
    // ProccessingContent requires an AudioBlock (chunk of audio, controlled by buffer)
    juce::dsp::AudioBlock<float> block(buffer);

    // buffer.clear(); // sine wave noise
    // juce::dsp::ProcessContextReplacing<float> stereoContext(block); // sine wave noise
    // osc.process(stereoContext); // sine wave noise

    // Buffer can have any num of channels, so pass declare the two we have using HelperFunction
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    // Blocks split by channel, create ProcessingContext from each block
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    // Process the context
    leftChain.process(leftContext);
    rightChain.process(rightContext);

    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);

    // Commenting out the default below
    // After this, need to go to JUCE dir, open up the AudioPlugIn host with Projucer, make a build then build it with CMake to run it.

    // // This is the place where you'd normally do the guts of your plugin's
    // // audio processing...
    // // Make sure to reset the state if your inner loop is processing
    // // the samples and the outer loop is handling the channels.
    // // Alternatively, you can process the samples with the channels
    // // interleaved by keeping the same state.
    // for (int channel = 0; channel < totalNumInputChannels; ++channel)
    // {
    //     auto* channelData = buffer.getWritePointer (channel);

    //     // ..do something to the data...
    // }
}

//==============================================================================
bool EQPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EQPluginAudioProcessor::createEditor()
{
    return new EQPluginAudioProcessorEditor (*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void EQPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    // saving and loading parameters to the memory block for quick processing of tree value state
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void EQPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) 
    {
        apvts.replaceState(tree);
        updateFilters();
    }

}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts) {
    ChainSettings settings;

    // Can get parameter with listener this way but it returns a normalised value (which we define below)
    // Functions creating coefficients need real values to process, not the normalised ones, so can't use this (but handy)
    // apvts.getParameter("LowCut Freq")->getValue();

    // Returns value based on the ranges set when we defined the params below
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load()); // Gets error because slope value was defined as int... which we changed to enum with Slope::Slope_12
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load()); // static_cast<type>(); looks awesome
    // settings.lowCutSlope = static_cast<Slope>(static_cast<int>(round(apvts.getRawParameterValue("LowCut Slope")->load()))); // For some reason the float to enum fails on other system...
    // settings.highCutSlope = static_cast<Slope>(static_cast<int>(round(apvts.getRawParameterValue("HighCut Slope")->load())));

    // parameters are stored as floats even when bool..... val > 0.5 == true
    settings.lowCutBypassed = apvts.getRawParameterValue("LowCut Bypassed")->load() > 0.5f;
    settings.peakBypassed = apvts.getRawParameterValue("Peak Bypassed")->load() > 0.5f;
    settings.highCutBypassed = apvts.getRawParameterValue("HighCut Bypassed")->load() > 0.5f;

    return settings;
}

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

void EQPluginAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings) 
{
    auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());

    leftChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    rightChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);

    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

}

void updateCoefficients(Coefficients &old, const Coefficients &replacements) 
{
    *old = *replacements;
}

void EQPluginAudioProcessor::updateLowCutFilters(const ChainSettings &chainSettings)
{
    auto lowcutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

    leftChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    rightChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);

    updateCutFilter(leftLowCut, lowcutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, lowcutCoefficients, chainSettings.lowCutSlope);
}


void EQPluginAudioProcessor::updateHighCutFilters(const ChainSettings &chainSettings) 
{
    auto highcutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();

    leftChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    rightChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);

    updateCutFilter(leftHighCut, highcutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highcutCoefficients, chainSettings.highCutSlope);
}

void EQPluginAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);

    updateLowCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
    updateHighCutFilters(chainSettings);

}

// juce::AudioProcessorValueTreeState::ParameterLayout EQPluginAudioProcessor::createParameterLayout() 
// {
//     // Make a new layout and then add the widgets to it
//     juce::AudioProcessorValueTreeState::ParameterLayout layout;

//     // Making a fancy string array to populate options on combo box instead of explicity naming them
//     juce::StringArray stringArray;
//     for(int i = 0; i < 4; i++) {
//         juce::String str;
//         // Order for pass filters { order = i + 1 * 2 }
//         str << (12 + i * 12);
//         str << " db/Oct";
//         stringArray.add(str);
//     }

//     // Sliders with args: id, name, range(min, max, interval increment, skew), default value // High Cut == Low Pass
//     layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq", "LowCut Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20.f));
//     layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq", "HighCut Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));
//     layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq", "Peak Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 750.f));
//     layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain", "Peak Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f), 0.0f));
//     layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality", "Peak Quality", juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f), 1.f));

//     layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
//     layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));

//     return layout;
// }

juce::AudioProcessorValueTreeState::ParameterLayout EQPluginAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Making a fancy string array to populate options on combo box instead of explicity naming them
    juce::StringArray stringArray;
    for(int i = 0; i < 4; i++) {
        juce::String str;
        // Order for pass filters { order = i + 1 * 2 }
        str << (12 + i * 12);
        str << " db/Oct";
        stringArray.add(str);
    }
    
    params.push_back (std::make_unique<juce::AudioParameterFloat>("LowCut Freq", "LowCut Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("HighCut Freq", "HighCut Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Peak Freq", "Peak Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 75.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Peak Gain", "Peak Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Peak Quality", "Peak Quality", juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f), 1.f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>("LowCut Bypassed", "LowCut Bypassed", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("Peak Bypassed", "Peak Bypassed", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("HighCut Bypassed", "HighCut Bypassed", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("Analyzer Enabled", "Analyzer Enabled", true));
    
    return { params.begin(), params.end() };
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EQPluginAudioProcessor();
}

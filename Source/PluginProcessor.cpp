#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace modulerack
{

ModuleRackProcessor::ModuleRackProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void ModuleRackProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    signalGraph.prepare(sampleRate, samplesPerBlock);
}

bool ModuleRackProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    return mainOut == juce::AudioChannelSet::mono() || mainOut == juce::AudioChannelSet::stereo();
}

void ModuleRackProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    midiMapper.processIncomingMidi(midiMessages, signalGraph.getModules());
    signalGraph.process(buffer, midiMessages);
}

juce::AudioProcessorEditor* ModuleRackProcessor::createEditor()
{
    return new ModuleRackEditor(*this);
}

void ModuleRackProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = PresetManager::toXml(signalGraph.getModules(), signalGraph.getClock(), midiMapper);
    copyXmlToBinary(*xml, destData);
}

void ModuleRackProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr)
        PresetManager::fromXml(*xml, signalGraph.getModules(), signalGraph.getClock(), midiMapper);
}

} // namespace modulerack

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new modulerack::ModuleRackProcessor();
}

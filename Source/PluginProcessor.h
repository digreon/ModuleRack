#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "Core/SignalGraph.h"
#include "Core/MidiMapper.h"
#include "Core/PresetManager.h"

namespace modulerack
{

class ModuleRackProcessor : public juce::AudioProcessor
{
public:
    ModuleRackProcessor();
    ~ModuleRackProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    using juce::AudioProcessor::processBlock; // keep the double-precision overload visible
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ModuleRack"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    SignalGraph& getSignalGraph() { return signalGraph; }
    MidiMapper& getMidiMapper() { return midiMapper; }

    bool savePreset(const juce::File& file)
    {
        return PresetManager::saveToFile(file, signalGraph.getModules(), signalGraph.getClock(), midiMapper);
    }

    bool loadPreset(const juce::File& file)
    {
        return PresetManager::loadFromFile(file, signalGraph.getModules(), signalGraph.getClock(), midiMapper);
    }

private:
    SignalGraph signalGraph;
    MidiMapper midiMapper;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModuleRackProcessor)
};

} // namespace modulerack

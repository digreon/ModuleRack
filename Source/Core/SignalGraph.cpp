#include "SignalGraph.h"
#include "../Modules/KickModule.h"
#include "../Modules/BassModule.h"
#include "../Modules/LeadModule.h"
#include "../Modules/PadChordModule.h"
#include "../Modules/ArpSequencerModule.h"
#include "../Modules/PercussionModule.h"
#include "../Modules/FxFilterDelayModule.h"
#include "../Modules/FxReverbCompModule.h"

namespace modulerack
{

SignalGraph::SignalGraph()
{
    modules[kickIndex]       = std::make_unique<KickModule>();
    modules[bassIndex]       = std::make_unique<BassModule>();
    modules[leadIndex]       = std::make_unique<LeadModule>();
    modules[padIndex]        = std::make_unique<PadChordModule>();
    modules[arpIndex]        = std::make_unique<ArpSequencerModule>();
    modules[percussionIndex] = std::make_unique<PercussionModule>();
    modules[fx1Index]        = std::make_unique<FxFilterDelayModule>();
    modules[fx2Index]        = std::make_unique<FxReverbCompModule>();
}

void SignalGraph::prepare(double sampleRate, int maximumBlockSize)
{
    clock.prepare(sampleRate, maximumBlockSize);

    for (auto& module : modules)
        module->prepare(sampleRate, maximumBlockSize);

    mixBuffer.setSize(2, maximumBlockSize);
    scratchBuffer.setSize(2, maximumBlockSize);
}

void SignalGraph::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    clock.advanceBlock(numSamples);

    mixBuffer.setSize(numChannels, numSamples, false, false, true);
    mixBuffer.clear();
    scratchBuffer.setSize(numChannels, numSamples, false, false, true);

    // Sound-generating modules sum into the mix bus. Each module's Level knob goes
    // up to 1.5, so six of them summing flat would sit far into the master
    // soft-clip; the trim keeps a default patch a few dB below full scale and
    // leaves the clipper as the safety net it is meant to be rather than the
    // thing shaping the sound.
    for (int i = 0; i <= percussionIndex; ++i)
    {
        scratchBuffer.clear();
        modules[(size_t) i]->process(scratchBuffer, midi, clock);
        for (int ch = 0; ch < numChannels; ++ch)
            mixBuffer.addFrom(ch, 0, scratchBuffer, ch, 0, numSamples, mixTrim);
    }

    // Fx1 (filter/delay) and Fx2 (reverb / kick sidechain-comp) process the bus in place, in series.
    modules[fx1Index]->process(mixBuffer, midi, clock);

    modules[fx2Index]->setSidechainEnvelope(modules[kickIndex]->getSidechainEnvelope());
    modules[fx2Index]->process(mixBuffer, midi, clock);

    // Master safety stage: six modules summing into one bus at full level can peak
    // well above 0 dBFS, and this runs into headphones/an iPad speaker. tanh() turns
    // what would be hard digital clipping into gentle saturation instead.
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* source = mixBuffer.getReadPointer(ch);
        auto* destination = buffer.getWritePointer(ch);

        for (int n = 0; n < numSamples; ++n)
            destination[n] = std::tanh(source[n]);
    }
}

} // namespace modulerack

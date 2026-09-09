/*
    Headless checks for the ModuleRack DSP core.

    These render the real SignalGraph offline -- no audio device, no window, no
    plugin host -- so the generative modules, the shared clock, the controller
    mapping and preset round-tripping can be verified on any machine and in CI.
    The plugin's Apple targets (AU/AUv3) still have to be built and tried in a
    host by hand; everything below the plugin client is covered here.
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <cmath>
#include <iostream>
#include <vector>

#include "Core/Clock.h"
#include "Core/Euclidean.h"
#include "Core/MidiMapper.h"
#include "Core/PresetManager.h"
#include "Core/Scale.h"
#include "Core/SignalGraph.h"

namespace
{

int failureCount = 0;
int checkCount = 0;

void check(bool condition, const juce::String& what, int line)
{
    ++checkCount;

    if (! condition)
    {
        ++failureCount;
        std::cout << "  FAIL (line " << line << "): " << what << std::endl;
    }
}

#define CHECK(cond) check((cond), #cond, __LINE__)

void startTest(const juce::String& name)
{
    std::cout << "- " << name << std::endl;
}

/** Renders `seconds` of audio through the graph in ragged block sizes, the way a
    real host does, and reports what came out. */
struct RenderResult
{
    float peak = 0.0f;
    double sumOfSquares = 0.0;
    int totalSamples = 0;
    bool allFinite = true;

    float rms() const { return totalSamples > 0 ? (float) std::sqrt(sumOfSquares / totalSamples) : 0.0f; }
};

RenderResult render(modulerack::SignalGraph& graph, double sampleRate, double seconds, int maximumBlockSize = 512)
{
    RenderResult result;

    graph.prepare(sampleRate, maximumBlockSize);

    juce::AudioBuffer<float> buffer(2, maximumBlockSize);
    juce::MidiBuffer midi;
    juce::Random random(1234);

    const int wanted = (int) (sampleRate * seconds);

    while (result.totalSamples < wanted)
    {
        // Ragged block sizes on purpose: a tick landing on the last sample of a
        // block used to be dropped, and only odd block sizes expose that.
        const int numSamples = juce::jmin(1 + random.nextInt(maximumBlockSize), wanted - result.totalSamples);

        buffer.setSize(2, numSamples, false, false, true);
        buffer.clear();
        graph.process(buffer, midi);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto* data = buffer.getReadPointer(ch);

            for (int n = 0; n < numSamples; ++n)
            {
                const float sample = data[n];

                if (! std::isfinite(sample))
                    result.allFinite = false;
                else
                {
                    result.peak = juce::jmax(result.peak, std::abs(sample));
                    result.sumOfSquares += (double) sample * (double) sample;
                }
            }
        }

        result.totalSamples += numSamples;
    }

    return result;
}

//==============================================================================
void testClockTiming()
{
    startTest("Clock fires every 16th, on the grid, with no drift and nothing dropped");

    constexpr double sampleRate = 44100.0;
    constexpr float bpm = 128.0f;
    const double samplesPerTick = 60.0 / bpm / 4.0 * sampleRate; // deliberately not a whole number

    modulerack::Clock clock;
    clock.prepare(sampleRate, 512);
    clock.setBpm(bpm);

    juce::Random random(99);
    std::vector<double> tickPositions;
    int expectedIndex = 0;
    long long samplesRendered = 0;
    const long long totalSamples = (long long) (sampleRate * 30.0);

    while (samplesRendered < totalSamples)
    {
        const int numSamples = (int) juce::jmin((long long) (1 + random.nextInt(512)),
                                                totalSamples - samplesRendered);
        clock.advanceBlock(numSamples);

        int lastOffset = -1;

        for (const auto& tick : clock.ticksThisBlock())
        {
            CHECK(tick.sampleOffset >= 0 && tick.sampleOffset < numSamples);
            CHECK(tick.sampleOffset >= lastOffset);
            CHECK(tick.sixteenthIndex == expectedIndex);

            lastOffset = tick.sampleOffset;
            expectedIndex = (expectedIndex + 1) % 16;
            tickPositions.push_back((double) (samplesRendered + tick.sampleOffset));
        }

        samplesRendered += numSamples;
    }

    // 30 seconds at 128 bpm is exactly 256 sixteenths: the first lands on sample 0
    // and the 257th would land one sample past the end of what was rendered.
    CHECK(tickPositions.size() == 256);

    double worstError = 0.0;

    for (size_t i = 0; i < tickPositions.size(); ++i)
        worstError = juce::jmax(worstError, std::abs(tickPositions[i] - (double) i * samplesPerTick));

    // Rounding to whole samples costs at most one sample; anything more means the
    // grid is drifting away from the tempo as blocks go by.
    CHECK(worstError < 1.0);
}

void testEuclideanPatterns()
{
    startTest("Euclidean patterns place exactly the requested number of hits");

    for (int hits = 0; hits <= 16; ++hits)
    {
        const auto pattern = modulerack::computeEuclideanPattern(hits);
        int count = 0;

        for (const bool step : pattern)
            count += step ? 1 : 0;

        CHECK(count == hits);
    }

    const auto fourOnTheFloor = modulerack::computeEuclideanPattern(4);
    CHECK(fourOnTheFloor[0] && fourOnTheFloor[4] && fourOnTheFloor[8] && fourOnTheFloor[12]);
    CHECK(! fourOnTheFloor[1] && ! fourOnTheFloor[7]);
}

void testScaleQuantiser()
{
    startTest("Scale quantiser handles octave wrap and negative degrees");

    using Scale = modulerack::Scale;
    constexpr int root = 60;

    CHECK(Scale::degreeToMidiNote(0, root, Scale::Type::naturalMinor) == root);
    CHECK(Scale::degreeToMidiNote(7, root, Scale::Type::naturalMinor) == root + 12);
    CHECK(Scale::degreeToMidiNote(-7, root, Scale::Type::naturalMinor) == root - 12);
    CHECK(Scale::degreeToMidiNote(-1, root, Scale::Type::naturalMinor) == root - 2);  // last degree, octave below
    CHECK(Scale::degreeToMidiNote(5, root, Scale::Type::minorPentatonic) == root + 12);

    // Every degree in a two-octave sweep must stay ordered and in range.
    int previous = -1;

    for (int degree = -14; degree <= 14; ++degree)
    {
        const int note = Scale::degreeToMidiNote(degree, root, Scale::Type::dorian);
        CHECK(note > previous);
        previous = note;
    }
}

void testGraphRendersCleanAudio()
{
    startTest("Full graph renders finite, audible, headroom-respecting audio");

    modulerack::SignalGraph graph;
    const auto result = render(graph, 48000.0, 12.0);

    CHECK(result.allFinite);
    CHECK(result.rms() > 0.005f);          // something is actually playing
    CHECK(result.peak > 0.05f);
    CHECK(result.peak <= 1.0f);            // master soft-clip holds the bus inside full scale

    // Gain staging: a default patch should land a few dB below full scale, not sit
    // on the soft-clip. Peaking at ~0.98 with an RMS over 0.5 is what a mix that
    // is being squashed by the clipper looks like.
    CHECK(result.peak < 0.9f);
    CHECK(result.rms() < 0.35f);
}

void testExtremeParameterSettingsStaySane()
{
    startTest("Every parameter at its minimum and at its maximum stays stable");

    for (const bool useMaximum : { false, true })
    {
        modulerack::SignalGraph graph;

        for (auto& module : graph.getModules())
            for (auto& param : module->getParams())
                param.set(useMaximum ? param.maxValue : param.minValue);

        const auto result = render(graph, 44100.0, 6.0);

        CHECK(result.allFinite);
        CHECK(result.peak <= 1.0f);
    }
}

void testSilenceWhenEverythingIsTurnedDown()
{
    startTest("All levels at zero produces silence");

    modulerack::SignalGraph graph;

    for (auto& module : graph.getModules())
        for (auto& param : module->getParams())
            if (param.id == "level" || param.id == "reverbMix" || param.id == "delayMix")
                param.set(0.0f);

    const auto result = render(graph, 48000.0, 4.0);

    CHECK(result.allFinite);
    CHECK(result.peak < 1.0e-4f);
}

void testKickDrivesTheSidechain()
{
    startTest("Kick exposes a sidechain envelope for Fx2 to duck from");

    modulerack::SignalGraph graph;
    graph.prepare(48000.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    float highestEnvelope = 0.0f;

    for (int block = 0; block < 200; ++block)
    {
        buffer.clear();
        graph.process(buffer, midi);
        highestEnvelope = juce::jmax(highestEnvelope,
                                     graph.getModules()[modulerack::SignalGraph::kickIndex]->getSidechainEnvelope());
    }

    CHECK(highestEnvelope > 0.1f);
    CHECK(highestEnvelope <= 1.0f);
}

void testMidiMapping()
{
    startTest("Pads select modules and knobs drive the selected module's params");

    modulerack::SignalGraph graph;
    modulerack::MidiMapper mapper;
    graph.prepare(48000.0, 512);

    const auto profile = modulerack::MidiMapper::mpkMiniProfile();

    // Pad 3 selects module 2 (Lead).
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(10, profile.padNotes[2], (juce::uint8) 100), 0);
    mapper.processIncomingMidi(midi, graph.getModules());
    CHECK(mapper.getSelectedModuleIndex() == 2);

    // Knob 1 at full drives that module's first parameter to its maximum.
    auto& leadParams = graph.getModules()[2]->getParams();
    midi.clear();
    midi.addEvent(juce::MidiMessage::controllerEvent(1, profile.knobCcs[0], 127), 0);
    mapper.processIncomingMidi(midi, graph.getModules());
    CHECK(juce::approximatelyEqual(leadParams[0].get(), leadParams[0].maxValue));

    // A knob that has been remapped follows its new CC and ignores the old one.
    mapper.setKnobCc(1, 20);
    midi.clear();
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 20, 0), 0);
    mapper.processIncomingMidi(midi, graph.getModules());
    CHECK(juce::approximatelyEqual(leadParams[1].get(), leadParams[1].minValue));

    // Pads on another module leave the first module's params where they were.
    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOn(10, profile.padNotes[5], (juce::uint8) 100), 0);
    mapper.processIncomingMidi(midi, graph.getModules());
    CHECK(mapper.getSelectedModuleIndex() == 5);
    CHECK(juce::approximatelyEqual(leadParams[0].get(), leadParams[0].maxValue));
}

void testPresetRoundTrip()
{
    startTest("A saved patch restores every parameter, the tempo and the mapping");

    modulerack::SignalGraph source;
    modulerack::MidiMapper sourceMapper;
    juce::Random random(7);

    source.getClock().setBpm(137.0f);
    sourceMapper.setPadNote(0, 60);
    sourceMapper.setKnobCc(0, 21);

    for (auto& module : source.getModules())
        for (auto& param : module->getParams())
            param.setNormalised(random.nextFloat());

    auto file = juce::File::createTempFile(".xml");
    CHECK(modulerack::PresetManager::saveToFile(file, source.getModules(), source.getClock(), sourceMapper));

    modulerack::SignalGraph loaded;
    modulerack::MidiMapper loadedMapper;
    CHECK(modulerack::PresetManager::loadFromFile(file, loaded.getModules(), loaded.getClock(), loadedMapper));

    CHECK(juce::approximatelyEqual(loaded.getClock().getBpm(), 137.0f));
    CHECK(loadedMapper.getPadNote(0) == 60);
    CHECK(loadedMapper.getKnobCc(0) == 21);

    for (int i = 0; i < modulerack::SignalGraph::numModules; ++i)
    {
        auto& sourceParams = source.getModules()[(size_t) i]->getParams();
        auto& loadedParams = loaded.getModules()[(size_t) i]->getParams();

        for (size_t p = 0; p < sourceParams.size(); ++p)
        {
            CHECK(sourceParams[p].id == loadedParams[p].id);
            CHECK(std::abs(sourceParams[p].get() - loadedParams[p].get()) < 1.0e-4f);
        }
    }

    file.deleteFile();
}

void testPresetNamingAndListing()
{
    startTest("Patches are named, listed in order and overwritten by name");

    auto directory = juce::File::createTempFile("");
    directory.deleteFile();
    directory.createDirectory();

    modulerack::SignalGraph graph;
    modulerack::MidiMapper mapper;

    // A name with characters a filesystem won't take still lands somewhere sane.
    const auto messy = modulerack::PresetManager::fileForPresetName(directory, "Deep / Techno: 01");
    CHECK(messy.getFileExtension() == ".xml");
    CHECK(! messy.getFileNameWithoutExtension().containsAnyOf("/:"));

    // An empty name is not allowed to produce a dotfile or an empty filename.
    const auto blank = modulerack::PresetManager::fileForPresetName(directory, "   ");
    CHECK(blank.getFileNameWithoutExtension().isNotEmpty());

    CHECK(modulerack::PresetManager::getPresetFiles(directory).isEmpty());

    for (const auto* name : { "Zulu", "alpha", "Mike" })
        CHECK(modulerack::PresetManager::saveToFile(modulerack::PresetManager::fileForPresetName(directory, name),
                                                    graph.getModules(), graph.getClock(), mapper));

    const auto listed = modulerack::PresetManager::getPresetFiles(directory);
    CHECK(listed.size() == 3);
    CHECK(listed[0].getFileNameWithoutExtension() == "alpha");   // ordered the way a person reads
    CHECK(listed[1].getFileNameWithoutExtension() == "Mike");
    CHECK(listed[2].getFileNameWithoutExtension() == "Zulu");

    // Saving the same name again replaces that patch rather than adding another.
    graph.getClock().setBpm(150.0f);
    CHECK(modulerack::PresetManager::saveToFile(modulerack::PresetManager::fileForPresetName(directory, "alpha"),
                                                graph.getModules(), graph.getClock(), mapper));
    CHECK(modulerack::PresetManager::getPresetFiles(directory).size() == 3);

    modulerack::SignalGraph reloaded;
    modulerack::MidiMapper reloadedMapper;
    CHECK(modulerack::PresetManager::loadFromFile(modulerack::PresetManager::fileForPresetName(directory, "alpha"),
                                                  reloaded.getModules(), reloaded.getClock(), reloadedMapper));
    CHECK(juce::approximatelyEqual(reloaded.getClock().getBpm(), 150.0f));

    directory.deleteRecursively();
}

void testTempoChangesAreHonoured()
{
    startTest("Changing tempo changes how often modules fire");

    auto countTicksAtBpm = [](float bpm)
    {
        modulerack::Clock clock;
        clock.prepare(48000.0, 512);
        clock.setBpm(bpm);

        int ticks = 0;

        for (int block = 0; block < 480; ++block) // 480 * 512 = 5.12 seconds
        {
            clock.advanceBlock(512);
            ticks += (int) clock.ticksThisBlock().size();
        }

        return ticks;
    };

    const int slow = countTicksAtBpm(60.0f);
    const int fast = countTicksAtBpm(180.0f);

    CHECK(slow > 0);
    CHECK(fast > 2 * slow); // three times the tempo, three times the 16ths
}

} // namespace

int main()
{
    std::cout << "ModuleRack DSP tests" << std::endl;

    testClockTiming();
    testEuclideanPatterns();
    testScaleQuantiser();
    testGraphRendersCleanAudio();
    testExtremeParameterSettingsStaySane();
    testSilenceWhenEverythingIsTurnedDown();
    testKickDrivesTheSidechain();
    testMidiMapping();
    testPresetRoundTrip();
    testPresetNamingAndListing();
    testTempoChangesAreHonoured();

    std::cout << std::endl
              << (failureCount == 0 ? "All checks passed" : "FAILURES")
              << ": " << (checkCount - failureCount) << "/" << checkCount << " checks"
              << std::endl;

    return failureCount == 0 ? 0 : 1;
}

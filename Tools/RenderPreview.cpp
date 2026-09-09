/*
    Renders the ModuleRack signal graph to a WAV file, offline and headless.

    Handy for hearing what a patch does without a plugin host (or a Mac), and for
    checking a DSP change by ear from any machine:

        ModuleRackRender out.wav [seconds] [preset.xml]
*/

#include <juce_audio_formats/juce_audio_formats.h>

#include <iostream>

#include "Core/PresetManager.h"
#include "Core/SignalGraph.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "usage: ModuleRackRender <out.wav> [seconds] [preset.xml]" << std::endl;
        return 1;
    }

    const juce::File outputFile = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    const double seconds = argc > 2 ? juce::jlimit(0.1, 600.0, juce::String(argv[2]).getDoubleValue()) : 30.0;

    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 512;

    modulerack::SignalGraph graph;
    modulerack::MidiMapper mapper;

    if (argc > 3)
    {
        const juce::File presetFile = juce::File::getCurrentWorkingDirectory().getChildFile(argv[3]);

        if (! modulerack::PresetManager::loadFromFile(presetFile, graph.getModules(), graph.getClock(), mapper))
        {
            std::cout << "could not read preset: " << presetFile.getFullPathName() << std::endl;
            return 1;
        }
    }

    graph.prepare(sampleRate, blockSize);

    outputFile.deleteFile();
    std::unique_ptr<juce::OutputStream> stream(outputFile.createOutputStream().release());

    if (stream == nullptr)
    {
        std::cout << "could not write to " << outputFile.getFullPathName() << std::endl;
        return 1;
    }

    juce::WavAudioFormat wavFormat;
    const auto writerOptions = juce::AudioFormatWriterOptions {}
                                   .withSampleRate(sampleRate)
                                   .withNumChannels(2)
                                   .withBitsPerSample(24);
    // Takes ownership of the stream on success, and leaves it alone on failure.
    auto writer = wavFormat.createWriterFor(stream, writerOptions);

    if (writer == nullptr)
    {
        std::cout << "could not create a WAV writer" << std::endl;
        return 1;
    }

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;
    const int totalSamples = (int) (sampleRate * seconds);

    for (int rendered = 0; rendered < totalSamples; rendered += blockSize)
    {
        const int numSamples = juce::jmin(blockSize, totalSamples - rendered);

        buffer.setSize(2, numSamples, false, false, true);
        buffer.clear();
        graph.process(buffer, midi);

        if (! writer->writeFromAudioSampleBuffer(buffer, 0, numSamples))
        {
            std::cout << "write failed" << std::endl;
            return 1;
        }
    }

    writer.reset(); // flushes the header

    std::cout << "wrote " << seconds << "s to " << outputFile.getFullPathName() << std::endl;
    return 0;
}

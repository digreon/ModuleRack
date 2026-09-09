#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "UI/MixOverviewComponent.h"
#include "UI/ModulePanel.h"

namespace modulerack
{

class ModuleRackEditor : public juce::AudioProcessorEditor
{
public:
    explicit ModuleRackEditor(ModuleRackProcessor& processorToUse);
    ~ModuleRackEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void applySelectedControllerProfile();

    ModuleRackProcessor& processor;

    juce::Label titleLabel;
    juce::Label bpmLabel;
    juce::Slider bpmSlider;
    juce::Label controllerLabel;
    juce::ComboBox controllerBox;
    juce::TextButton savePresetButton { "Save Preset" };
    juce::TextButton loadPresetButton { "Load Preset" };

    MixOverviewComponent padRow;
    ModulePanel modulePanel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModuleRackEditor)
};

} // namespace modulerack

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "UI/MixOverviewComponent.h"
#include "UI/ModulePanel.h"
#include "UI/PresetBrowser.h"

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
    void showPresetBrowser(bool shouldBeVisible);

    ModuleRackProcessor& rackProcessor;

    juce::Label titleLabel;
    juce::Label bpmLabel;
    juce::Slider bpmSlider;
    juce::Label controllerLabel;
    juce::ComboBox controllerBox;
    juce::TextButton presetsButton { "Presets" };

    MixOverviewComponent padRow;
    ModulePanel modulePanel;
    PresetBrowser presetBrowser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModuleRackEditor)
};

} // namespace modulerack

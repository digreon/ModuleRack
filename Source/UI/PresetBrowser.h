#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../PluginProcessor.h"

namespace modulerack
{

/**
 * In-plugin preset list: a name field, the patches on disk, and Save / Load /
 * Delete. Deliberately does NOT use juce::FileChooser -- on iOS that is a system
 * document picker, and modal system UI inside an AUv3 extension is exactly the
 * kind of thing that fails on a device while working fine on a desktop. Nothing
 * here leaves the plugin's own window.
 *
 * Delete asks for a second tap rather than opening a confirmation dialog, for
 * the same reason: no system alerts, and no patch lost to a stray fingertip.
 */
class PresetBrowser : public juce::Component,
                      private juce::ListBoxModel,
                      private juce::Timer
{
public:
    explicit PresetBrowser(ModuleRackProcessor& processorToUse);
    ~PresetBrowser() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    /** Re-reads the preset folder. Called when the browser is shown. */
    void refresh();

    /** Fired after a patch is loaded, so the editor can re-read tempo etc. */
    std::function<void()> onPresetLoaded;

private:
    // ListBoxModel
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool isSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

    void timerCallback() override;

    void savePreset();
    void loadSelectedPreset();
    void deleteSelectedPreset();
    void setStatus(const juce::String& message);
    juce::File getSelectedFile() const;
    void resetDeleteConfirmation();

    ModuleRackProcessor& processor;

    juce::Label titleLabel;
    juce::TextEditor nameEditor;
    juce::ListBox presetList { "Presets", this };
    juce::TextButton saveButton { "Save" };
    juce::TextButton loadButton { "Load" };
    juce::TextButton deleteButton { "Delete" };
    juce::Label statusLabel;

    juce::Array<juce::File> presetFiles;
    bool deleteArmed = false;
    int statusCountdown = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};

} // namespace modulerack

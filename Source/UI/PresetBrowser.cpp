#include "PresetBrowser.h"

namespace modulerack
{

namespace
{
    constexpr int rowHeight = 44;      // Apple's minimum touch target.
    constexpr int statusTicks = 6;     // Timer runs at 2 Hz, so ~3 seconds.
}

PresetBrowser::PresetBrowser(ModuleRackProcessor& processorToUse)
    : processor(processorToUse)
{
    titleLabel.setText("Presets", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(18.0f).withStyle("Bold")));
    addAndMakeVisible(titleLabel);

    nameEditor.setTextToShowWhenEmpty("Patch name", juce::Colours::grey);
    nameEditor.setReturnKeyStartsNewLine(false);
    nameEditor.onReturnKey = [this] { savePreset(); };
    addAndMakeVisible(nameEditor);

    presetList.setRowHeight(rowHeight);
    presetList.setMultipleSelectionEnabled(false);
    addAndMakeVisible(presetList);

    saveButton.onClick = [this] { savePreset(); };
    loadButton.onClick = [this] { loadSelectedPreset(); };
    deleteButton.onClick = [this] { deleteSelectedPreset(); };
    addAndMakeVisible(saveButton);
    addAndMakeVisible(loadButton);
    addAndMakeVisible(deleteButton);

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setFont(juce::Font(juce::FontOptions(13.0f)));
    addAndMakeVisible(statusLabel);

    refresh();
}

PresetBrowser::~PresetBrowser()
{
    stopTimer();
}

void PresetBrowser::resized()
{
    auto area = getLocalBounds().reduced(10);

    titleLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(6);

    auto nameRow = area.removeFromTop(rowHeight);
    saveButton.setBounds(nameRow.removeFromRight(90));
    nameRow.removeFromRight(8);
    nameEditor.setBounds(nameRow);

    area.removeFromTop(8);

    auto buttonRow = area.removeFromBottom(rowHeight);
    loadButton.setBounds(buttonRow.removeFromLeft(120));
    buttonRow.removeFromLeft(8);
    deleteButton.setBounds(buttonRow.removeFromLeft(140));
    buttonRow.removeFromLeft(12);
    statusLabel.setBounds(buttonRow);

    area.removeFromBottom(8);
    presetList.setBounds(area);
}

void PresetBrowser::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void PresetBrowser::refresh()
{
    const auto previouslySelected = getSelectedFile();

    presetFiles = PresetManager::getPresetFiles(PresetManager::getPresetsDirectory());
    presetList.updateContent();

    // Keep pointing at the same patch across a refresh where that still exists.
    int rowToSelect = -1;

    for (int i = 0; i < presetFiles.size(); ++i)
        if (presetFiles[i] == previouslySelected)
            rowToSelect = i;

    if (rowToSelect >= 0)
        presetList.selectRow(rowToSelect, true, true);
    else
        presetList.deselectAllRows();

    if (nameEditor.getText().trim().isEmpty())
    {
        // Saving should never require typing on a touch screen: offer the first
        // unused "Patch N" so Save is always one tap away.
        int number = 1;

        while (PresetManager::fileForPresetName(PresetManager::getPresetsDirectory(),
                                                "Patch " + juce::String(number)).existsAsFile())
            ++number;

        nameEditor.setText("Patch " + juce::String(number), juce::dontSendNotification);
    }

    resetDeleteConfirmation();
    presetList.repaint();
}

int PresetBrowser::getNumRows()
{
    return presetFiles.size();
}

void PresetBrowser::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool isSelected)
{
    if (! juce::isPositiveAndBelow(row, presetFiles.size()))
        return;

    if (isSelected)
        g.fillAll(juce::Colours::orange.withAlpha(0.35f));

    g.setColour(getLookAndFeel().findColour(juce::Label::textColourId));
    g.setFont(juce::Font(juce::FontOptions(16.0f)));
    g.drawText(presetFiles[row].getFileNameWithoutExtension(),
               juce::Rectangle<int>(10, 0, width - 20, height),
               juce::Justification::centredLeft, true);
}

void PresetBrowser::selectedRowsChanged(int)
{
    const auto file = getSelectedFile();

    if (file != juce::File())
        nameEditor.setText(file.getFileNameWithoutExtension(), juce::dontSendNotification);

    resetDeleteConfirmation();
}

void PresetBrowser::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    presetList.selectRow(row);
    loadSelectedPreset();
}

juce::File PresetBrowser::getSelectedFile() const
{
    const int row = presetList.getSelectedRow();
    return juce::isPositiveAndBelow(row, presetFiles.size()) ? presetFiles[row] : juce::File();
}

void PresetBrowser::savePreset()
{
    const auto file = PresetManager::fileForPresetName(PresetManager::getPresetsDirectory(),
                                                       nameEditor.getText());
    const bool replacing = file.existsAsFile();

    if (! processor.savePreset(file))
    {
        setStatus("Could not save " + file.getFileName());
        return;
    }

    refresh();

    for (int i = 0; i < presetFiles.size(); ++i)
        if (presetFiles[i] == file)
            presetList.selectRow(i, true, true);

    setStatus((replacing ? "Replaced " : "Saved ") + file.getFileNameWithoutExtension());
}

void PresetBrowser::loadSelectedPreset()
{
    const auto file = getSelectedFile();

    if (file == juce::File())
    {
        setStatus("Pick a patch first");
        return;
    }

    if (! processor.loadPreset(file))
    {
        setStatus("Could not read " + file.getFileName());
        return;
    }

    if (onPresetLoaded != nullptr)
        onPresetLoaded();

    setStatus("Loaded " + file.getFileNameWithoutExtension());
}

void PresetBrowser::deleteSelectedPreset()
{
    const auto file = getSelectedFile();

    if (file == juce::File())
    {
        setStatus("Pick a patch first");
        return;
    }

    // Two taps rather than a confirmation dialog: no system alert (which is the
    // unreliable part inside an AUv3 extension), and no patch lost to a stray
    // fingertip. The arming resets itself after a few seconds.
    if (! deleteArmed)
    {
        deleteArmed = true;
        deleteButton.setButtonText("Tap to confirm");
        setStatus("Deleting " + file.getFileNameWithoutExtension());
        return;
    }

    resetDeleteConfirmation();

    if (! file.deleteFile())
    {
        setStatus("Could not delete " + file.getFileName());
        return;
    }

    const auto deletedName = file.getFileNameWithoutExtension();
    nameEditor.clear();
    refresh();
    setStatus("Deleted " + deletedName);
}

void PresetBrowser::resetDeleteConfirmation()
{
    deleteArmed = false;
    deleteButton.setButtonText("Delete");
}

void PresetBrowser::setStatus(const juce::String& message)
{
    statusLabel.setText(message, juce::dontSendNotification);
    statusCountdown = statusTicks;
    startTimerHz(2);
}

void PresetBrowser::timerCallback()
{
    if (--statusCountdown > 0)
        return;

    statusLabel.setText({}, juce::dontSendNotification);
    resetDeleteConfirmation();
    stopTimer();
}

} // namespace modulerack

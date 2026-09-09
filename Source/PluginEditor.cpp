#include "PluginEditor.h"

namespace modulerack
{

ModuleRackEditor::ModuleRackEditor(ModuleRackProcessor& processorToUse)
    : AudioProcessorEditor(&processorToUse),
      processor(processorToUse),
      padRow(processor.getSignalGraph(), processor.getMidiMapper()),
      modulePanel(processor.getSignalGraph(), processor.getMidiMapper())
{
    titleLabel.setText("ModuleRack", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(22.0f).withStyle("Bold")));
    addAndMakeVisible(titleLabel);

    bpmLabel.setText("BPM", juce::dontSendNotification);
    addAndMakeVisible(bpmLabel);

    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    bpmSlider.setRange(60.0, 200.0, 1.0);
    bpmSlider.setValue(processor.getSignalGraph().getClock().getBpm(), juce::dontSendNotification);
    bpmSlider.onValueChange = [this] { processor.getSignalGraph().getClock().setBpm((float) bpmSlider.getValue()); };
    addAndMakeVisible(bpmSlider);

    controllerLabel.setText("Controller", juce::dontSendNotification);
    addAndMakeVisible(controllerLabel);

    controllerBox.addItem(MidiMapper::mpkMiniProfile().name, 1);
    controllerBox.addItem(MidiMapper::genericProfile().name, 2);
    controllerBox.setSelectedId(1, juce::dontSendNotification);
    controllerBox.onChange = [this] { applySelectedControllerProfile(); };
    addAndMakeVisible(controllerBox);

    savePresetButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>("Save ModuleRack preset", PresetManager::getPresetsDirectory(), "*.xml");
        fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                                  [this](const juce::FileChooser& fc)
                                  {
                                      auto file = fc.getResult();
                                      if (file != juce::File())
                                          processor.savePreset(file.withFileExtension(".xml"));
                                  });
    };
    addAndMakeVisible(savePresetButton);

    loadPresetButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>("Load ModuleRack preset", PresetManager::getPresetsDirectory(), "*.xml");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode,
                                  [this](const juce::FileChooser& fc)
                                  {
                                      auto file = fc.getResult();
                                      if (file != juce::File())
                                      {
                                          processor.loadPreset(file);
                                          bpmSlider.setValue(processor.getSignalGraph().getClock().getBpm(),
                                                             juce::dontSendNotification);
                                      }
                                  });
    };
    addAndMakeVisible(loadPresetButton);

    addAndMakeVisible(padRow);
    addAndMakeVisible(modulePanel);

    setResizable(true, true);

    // Sized for an iPad in landscape, but has to survive a host pane in Split View
    // or a small AUv3 window, so the minimum is well below the default.
    setResizeLimits(480, 340, 1600, 1200);
    setSize(900, 560);
}

ModuleRackEditor::~ModuleRackEditor() = default;

void ModuleRackEditor::applySelectedControllerProfile()
{
    processor.getMidiMapper().setProfile(controllerBox.getSelectedId() == 2 ? MidiMapper::genericProfile()
                                                                            : MidiMapper::mpkMiniProfile());
}

void ModuleRackEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void ModuleRackEditor::resized()
{
    auto area = getLocalBounds().reduced(12);

    auto header = area.removeFromTop(40);
    titleLabel.setBounds(header.removeFromLeft(300));
    loadPresetButton.setBounds(header.removeFromRight(110));
    header.removeFromRight(8);
    savePresetButton.setBounds(header.removeFromRight(110));

    area.removeFromTop(8);
    auto transport = area.removeFromTop(36);
    bpmLabel.setBounds(transport.removeFromLeft(40));
    bpmSlider.setBounds(transport.removeFromLeft(200));
    transport.removeFromLeft(16);
    controllerLabel.setBounds(transport.removeFromLeft(80));
    controllerBox.setBounds(transport.removeFromLeft(juce::jmax(160, transport.getWidth())));

    area.removeFromTop(10);

    // 56pt of pad row: comfortably past Apple's 44pt minimum touch target, since
    // these are the eight things that get hit most on an iPad.
    padRow.setBounds(area.removeFromTop(56));

    area.removeFromTop(10);
    modulePanel.setBounds(area);
}

} // namespace modulerack

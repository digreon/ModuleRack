#include "PluginEditor.h"

namespace modulerack
{

ModuleRackEditor::ModuleRackEditor(ModuleRackProcessor& processorToUse)
    : AudioProcessorEditor(&processorToUse),
      processor(processorToUse),
      padRow(processor.getSignalGraph(), processor.getMidiMapper()),
      modulePanel(processor.getSignalGraph(), processor.getMidiMapper()),
      presetBrowser(processor)
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

    // Short menu entries: the full profile name spells out the note/CC numbers,
    // which is README material, not something to squeeze into a host pane.
    controllerBox.addItem("MPK Mini", 1);
    controllerBox.addItem("Generic CC 1-8", 2);
    controllerBox.setSelectedId(1, juce::dontSendNotification);
    controllerBox.onChange = [this] { applySelectedControllerProfile(); };
    addAndMakeVisible(controllerBox);

    presetsButton.setClickingTogglesState(true);
    presetsButton.onClick = [this] { showPresetBrowser(presetsButton.getToggleState()); };
    addAndMakeVisible(presetsButton);

    presetBrowser.onPresetLoaded = [this]
    {
        bpmSlider.setValue(processor.getSignalGraph().getClock().getBpm(), juce::dontSendNotification);
    };
    addChildComponent(presetBrowser);

    addAndMakeVisible(padRow);
    addAndMakeVisible(modulePanel);

    setResizable(true, true);

    // Sized for an iPad in landscape, but has to survive a host pane in Split View
    // or a small AUv3 window, so the minimum is well below the default.
    setResizeLimits(480, 340, 1600, 1200);
    setSize(900, 560);
}

ModuleRackEditor::~ModuleRackEditor() = default;

void ModuleRackEditor::showPresetBrowser(bool shouldBeVisible)
{
    // The browser takes over the panel area rather than opening a window: an AUv3
    // view on an iPad has no room for a second window, and modal system UI inside
    // an extension is unreliable.
    if (shouldBeVisible)
        presetBrowser.refresh();

    presetBrowser.setVisible(shouldBeVisible);
    modulePanel.setVisible(! shouldBeVisible);
}

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
    // A host pane in Split View can be barely taller than the chrome. When that
    // happens every fixed height gives up a few points so the knobs -- the part
    // you actually play -- keep room to be knobs rather than dots.
    const bool compact = getHeight() < 460;

    auto area = getLocalBounds().reduced(compact ? 8 : 12);

    auto header = area.removeFromTop(compact ? 34 : 40);
    titleLabel.setBounds(header.removeFromLeft(compact ? 200 : 300));
    presetsButton.setBounds(header.removeFromRight(120));

    area.removeFromTop(compact ? 6 : 8);
    auto transport = area.removeFromTop(compact ? 32 : 36);
    bpmLabel.setBounds(transport.removeFromLeft(40));
    bpmSlider.setBounds(transport.removeFromLeft(juce::jmin(200, transport.getWidth() / 2)));
    transport.removeFromLeft(16);

    // The word "Controller" is the first thing to go when the row gets tight.
    const bool showControllerLabel = transport.getWidth() > 260;
    controllerLabel.setVisible(showControllerLabel);

    if (showControllerLabel)
        controllerLabel.setBounds(transport.removeFromLeft(80));

    controllerBox.setBounds(transport);

    area.removeFromTop(compact ? 8 : 10);

    // 56pt of pad row: comfortably past Apple's 44pt minimum touch target, since
    // these are the eight things that get hit most on an iPad. A cramped pane
    // gets exactly the 44pt minimum, never less.
    padRow.setBounds(area.removeFromTop(compact ? 44 : 56));

    area.removeFromTop(compact ? 8 : 10);
    modulePanel.setBounds(area);
    presetBrowser.setBounds(area);
}

} // namespace modulerack

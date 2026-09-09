#include "ModulePanel.h"

namespace modulerack
{

ModulePanel::ModulePanel(SignalGraph& graphToUse, MidiMapper& mapperToUse)
    : graph(graphToUse), mapper(mapperToUse)
{
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setFont(juce::Font(juce::FontOptions(20.0f).withStyle("Bold")));
    addAndMakeVisible(titleLabel);

    for (int i = 0; i < 8; ++i)
    {
        auto& slider = knobSliders[(size_t) i];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
        slider.setRange(0.0, 1.0, 0.001);
        slider.onValueChange = [this, i]
        {
            if (updatingFromCode)
                return;
            const int selected = mapper.getSelectedModuleIndex();
            graph.getModules()[(size_t) selected]->setParamNormalised(i, (float) knobSliders[(size_t) i].getValue());
        };
        addAndMakeVisible(slider);

        auto& label = knobLabels[(size_t) i];
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(12.0f)));
        addAndMakeVisible(label);
    }

    rebuildForSelectedModule();
    startTimerHz(30);
}

ModulePanel::~ModulePanel()
{
    stopTimer();
}

void ModulePanel::resized()
{
    auto area = getLocalBounds().reduced(10);
    titleLabel.setBounds(area.removeFromTop(28));
    area.removeFromTop(6);

    const int cellWidth = area.getWidth() / 4;
    const int cellHeight = area.getHeight() / 2;

    for (int i = 0; i < 8; ++i)
    {
        const int col = i % 4;
        const int row = i / 4;
        auto cell = juce::Rectangle<int>(area.getX() + col * cellWidth, area.getY() + row * cellHeight, cellWidth, cellHeight);
        knobLabels[(size_t) i].setBounds(cell.removeFromBottom(16));
        knobSliders[(size_t) i].setBounds(cell.reduced(6));
    }
}

void ModulePanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.0f));
}

void ModulePanel::timerCallback()
{
    if (mapper.getSelectedModuleIndex() != lastSelectedIndex)
        rebuildForSelectedModule();
    else
        refresh();
}

void ModulePanel::rebuildForSelectedModule()
{
    lastSelectedIndex = mapper.getSelectedModuleIndex();
    auto& module = *graph.getModules()[(size_t) lastSelectedIndex];
    titleLabel.setText(module.name, juce::dontSendNotification);

    auto& params = module.getParams();
    for (int i = 0; i < 8; ++i)
    {
        knobSliders[(size_t) i].setRange(0.0, 1.0, 0.001);
        knobLabels[(size_t) i].setText(params[(size_t) i].label, juce::dontSendNotification);
    }
    refresh();
}

void ModulePanel::refresh()
{
    auto& module = *graph.getModules()[(size_t) lastSelectedIndex];
    auto& params = module.getParams();

    updatingFromCode = true;
    for (int i = 0; i < 8; ++i)
        knobSliders[(size_t) i].setValue(params[(size_t) i].getNormalised(), juce::dontSendNotification);
    updatingFromCode = false;
}

} // namespace modulerack

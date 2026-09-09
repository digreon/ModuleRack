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
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 74, 20);
        slider.onValueChange = [this, i]
        {
            if (updatingFromCode)
                return;

            const int selected = mapper.getSelectedModuleIndex();
            graph.getModules()[(size_t) selected]->getParams()[(size_t) i].set((float) knobSliders[(size_t) i].getValue());
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
    // The pad row already highlights the selected module, so in a short panel the
    // title is redundant chrome standing between you and a usable knob.
    const bool showTitle = getHeight() >= 200;

    auto area = getLocalBounds().reduced(showTitle ? 10 : 6);

    titleLabel.setVisible(showTitle);

    if (showTitle)
    {
        titleLabel.setBounds(area.removeFromTop(28));
        area.removeFromTop(6);
    }

    const int cellWidth = area.getWidth() / 4;
    const int cellHeight = area.getHeight() / 2;

    // In a narrow host pane there isn't room for a knob AND a readout under it;
    // the knob is the part you play with, so the number is what goes.
    const bool showReadouts = cellHeight >= 86;

    for (int i = 0; i < 8; ++i)
    {
        auto& slider = knobSliders[(size_t) i];
        slider.setTextBoxStyle(showReadouts ? juce::Slider::TextBoxBelow : juce::Slider::NoTextBox,
                               true, 74, 20);

        const int col = i % 4;
        const int row = i / 4;
        auto cell = juce::Rectangle<int>(area.getX() + col * cellWidth, area.getY() + row * cellHeight, cellWidth, cellHeight);
        knobLabels[(size_t) i].setBounds(cell.removeFromBottom(18));
        slider.setBounds(cell.reduced(4));
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
        auto& param = params[(size_t) i];
        auto& slider = knobSliders[(size_t) i];

        // Knobs read in the parameter's own units -- a cutoff shows 500 (Hz) and a
        // density shows 6 (hits per bar). Showing a normalised 0..1 tells a player
        // nothing about what the module is doing.
        slider.setRange(param.minValue, param.maxValue);

        const float span = param.maxValue - param.minValue;
        slider.setNumDecimalPlacesToDisplay(span >= 100.0f ? 0 : (span >= 10.0f ? 1 : (span >= 1.0f ? 2 : 3)));

        knobLabels[(size_t) i].setText(param.label, juce::dontSendNotification);
    }

    refresh();
}

void ModulePanel::refresh()
{
    auto& module = *graph.getModules()[(size_t) lastSelectedIndex];
    auto& params = module.getParams();

    updatingFromCode = true;

    for (int i = 0; i < 8; ++i)
    {
        auto& slider = knobSliders[(size_t) i];

        // Don't write to a knob that a finger is on: a touch drag and this timer
        // both moving the same slider makes it stutter under the fingertip.
        if (slider.isMouseButtonDown())
            continue;

        slider.setValue(params[(size_t) i].get(), juce::dontSendNotification);
    }

    updatingFromCode = false;
}

} // namespace modulerack

/*
------------------------------------------------------------------

This file is part of a plugin for the Open Ephys GUI
Copyright (C) 2018 Translational NeuroEngineering Laboratory, MGH

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef MEAN_SPIKE_RATE_EDITOR_H_INCLUDED
#define MEAN_SPIKE_RATE_EDITOR_H_INCLUDED

#include <EditorHeaders.h>
#include "MeanSpikeRate.h"

class ElectrodeStateButton : public ElectrodeButton
{
public:
    ElectrodeStateButton(SpikeChannel* chan) : ElectrodeButton(0)
    {
        identifier = chan->getIdentifier();
    }
    ~ElectrodeStateButton() {};
    String getIdentifier() { return identifier; }
private:
    String identifier;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ElectrodeStateButton);
};

class ElectrodeViewport : public Viewport
{
public:
    ElectrodeViewport() {};
    ~ElectrodeViewport() {};

    void resized() override {};

    //Override mouseWheelMove to prevent scrolling conflict with editor viewport
    void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override {}
};

class MeanSpikeRateEditor : public GenericEditor, public Button::Listener
{
public:
    MeanSpikeRateEditor(MeanSpikeRate* parentNode);
    ~MeanSpikeRateEditor();

    void updateSettings() override;

    void selectedStreamHasChanged() override { updateSettings(); }

    int getNumActiveElectrodes();

    void buttonClicked(Button* button) override;

    bool getSpikeChannelEnabled(int index);
    void setSpikeChannelEnabled(int index, bool enabled);

private:
    // functions
    ElectrodeStateButton* makeNewChannelButton(SpikeChannel* chan);
    void layoutChannelButtons();

    // UI elements
    ScopedPointer<ElectrodeViewport> spikeChannelViewport;
    ScopedPointer<Component> spikeChannelCanvas;
    OwnedArray<ElectrodeStateButton> spikeChannelButtons;

    // constants
    static const int BUTTON_WIDTH = 35;
    static const int BUTTON_HEIGHT = 15;

    static const int WIDTH = 200;
    static const int VIEWPORT_WIDTH = 170;
    static const int VIEWPORT_HEIGHT = 50;
    static const int BUTTONS_PER_ROW = 4; //CONTENT_WIDTH / BUTTON_WIDTH;

    const String OUTPUT_TOOLTIP = "Continuous channel to overwrite with the spike rate (meaned over time and selected electrodes)";
    const String TIME_CONST_TOOLTIP = "Time for the influence of a single spike to decay to 36.8% (1/e) of its initial value (larger = smoother, smaller = faster reaction to changes)";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeanSpikeRateEditor);
};

#endif // MEAN_SPIKE_RATE_EDITOR_H_INCLUDED
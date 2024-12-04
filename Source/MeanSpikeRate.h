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

#ifndef MEAN_SPIKE_RATE_H_INCLUDED
#define MEAN_SPIKE_RATE_H_INCLUDED

#include <ProcessorHeaders.h>


/**

    Holds settings for each data stream

*/
class MeanSpikeRateSettings {
public:
    float timeConstMs;
    int outputChan;


};

/* Estimates the mean spike rate over time and channels. Uses an exponentially
 * weighted moving average to estimate a temporal mean (with adjustable time
 * constant), and averages the rate across selected spike channels (electrodes).
 * Outputs the resulting rate onto a selected continuous channel (overwriting its contents).
 *
 * @see GenericProcessor
 */
class MeanSpikeRate : public GenericProcessor
{
    friend class MeanSpikeRateEditor;

public:

    /** Constructor */
    MeanSpikeRate();

    /** Destructor */
    ~MeanSpikeRate();

    /** Registers parameters for this processor */
    void registerParameters() override;

    /** Creates the custom editor for this processor */
    AudioProcessorEditor* createEditor() override;

    /** Checks whether an incoming spike channel is selected */
    bool isActive(const SpikeChannel* chan) { return spikeChannelActive[chan->getIdentifier()]; };

    /** Overwrites continuous data with average spike rate */
    void process(AudioBuffer<float>& continuousBuffer) override;

    /** Called when a spike is received */
    void handleSpike(SpikePtr spike) override;

    /** Called when a parameter is changed */
    void parameterValueChanged(Parameter* param) override;

    /** Loads spike channel selection state. */
    void loadCustomParametersFromXml(XmlElement* parentElement) override;

    /** Saves spike channel selection state. */
    void saveCustomParametersToXml(XmlElement* parentElement) override;

private:

    std::map<String, bool> spikeChannelActive;

    // functions
    int getNumActiveElectrodes();
    void updateSettings() override;;

    // internals
    StreamSettings<MeanSpikeRateSettings> settings;
    std::map<uint16, int> currSample;          // per-buffer - allows processing samples while handling events
    std::map<uint16, double> spikeAmp;         // updated once per buffer
    std::map<uint16, float> currMean;
    std::map<uint16, float*> wpBuffer;
    std::map<uint16, double> decayPerSample;   // updated once per buffer

    const String OUTPUT_TOOLTIP = "Continuous channel to overwrite with the spike rate (meaned over time and selected electrodes)";
    const String TIME_CONST_TOOLTIP = "Time for the influence of a single spike to decay to 36.8% (1/e) of its initial value (larger = smoother, smaller = faster reaction to changes)";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeanSpikeRate);
};

#endif // MEAN_SPIKE_RATE_H_INCLUDED
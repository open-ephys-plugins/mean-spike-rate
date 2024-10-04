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

#include "MeanSpikeRate.h"
#include "MeanSpikeRateEditor.h"

MeanSpikeRate::MeanSpikeRate() : GenericProcessor("Mean Spike Rate")
{
    addSelectedChannelsParameter(Parameter::STREAM_SCOPE, "Output", "Output", OUTPUT_TOOLTIP, 1);
    addFloatParameter(Parameter::STREAM_SCOPE, "Time_Const", "Time Constant", TIME_CONST_TOOLTIP, "ms", 1000.0, 1, std::numeric_limits<float>::max(), 0.001);
}

MeanSpikeRate::~MeanSpikeRate() {
}

AudioProcessorEditor* MeanSpikeRate::createEditor()
{
    editor = std::make_unique<MeanSpikeRateEditor>(this);
    return editor.get();
}

void MeanSpikeRate::process(AudioBuffer<float>& continuousBuffer)
{
    for (auto stream : getDataStreams())
    {
        // Get parameters for current stream
        const uint16 streamId = stream->getStreamId();
        MeanSpikeRateSettings* msrSettings = settings[streamId];
        float timeConstMs = msrSettings->timeConstMs;
        int outputChan = msrSettings->outputChan;

        uint32 numSamples;
        if (getNumInputs() == 0 || (numSamples = getNumSamplesInBlock(streamId)) == 0)
        {
            continue;
        }

        int numActiveChannels = continuousChannels.size();
        // Check that active channel is valid. Output chan is the global index, so use all continuous channels
        if (!(outputChan > -1 && outputChan < numActiveChannels)) 
        {
            continue;
        }

        // update algorithm parameters
        // we assume each spike channel has the same sample rate as the selected channel.
        // if not, this would get a lot more complicated.
        int numActiveElectrodes = 0;
        for (auto spikeChannel : stream->getSpikeChannels())
        {
            numActiveElectrodes += isActive(spikeChannel);
        }
        if (numActiveElectrodes == 0)
        {
            continue;
        }
        double timeConstSec = timeConstMs / 1000.0;
        
        //double timeConstSamp = timeConstSec * getDataChannel(outputChan)->getSampleRate();
        double timeConstSamp = timeConstSec * getSampleRate(streamId);
        decayPerSample[streamId] = exp(-1 / timeConstSamp);

        // the initial amplitude of each spike such that if there is a steady rate of
        // spiking, the average over time of the exponentially weighted mean
        // (at the limit where the process has been continuing forever)
        // equals the actual spike rate in Hz. This is just 1 / (time const in sec).
        spikeAmp[streamId] = 1 / (timeConstSec * numActiveElectrodes);

        // initialize first sample
        currSample[streamId] = 0;

        // handle each spike, calculating the mean spike rate of samples in between.
        checkForEvents(true);

        // after all spikes are handled, finish writing samples
        wpBuffer[streamId] = continuousBuffer.getWritePointer(outputChan);
        for (int samp = currSample[streamId]; samp < numSamples; ++samp)
        {
            wpBuffer[streamId][samp] = currMean[streamId];
            currMean[streamId] *= decayPerSample[streamId];
        }
    }
    
}
void MeanSpikeRate::handleSpike(SpikePtr spike)
{
    Spike* spikeEvent = spike.get();

    //Check if spike channel is enabled
    const SpikeChannel* spikeChannel = spikeEvent->spikeChannel;
    if (!isActive(spikeChannel))
    {
        return;
    }

    int streamId = spikeChannel->getStreamId();
    int samplePosition = spikeChannel->currentSampleIndex;

    jassert(samplePosition >= currSample[streamId]); // spike sample must not have already been finished

    // write samples up to the spike position
    for (int samp = currSample[streamId]; samp < samplePosition; ++samp)
    {
        wpBuffer[streamId][samp] = currMean[streamId]; //problem 
        currMean[streamId] *= decayPerSample[streamId];
    }
    currSample[streamId] = samplePosition;

    // add spike contribution
    currMean[streamId] += spikeAmp[streamId];
}

void MeanSpikeRate::updateSettings()
{
    settings.update(getDataStreams());

    for (auto stream : getDataStreams())
    {
        // Update settings objects
        parameterValueChanged(stream->getParameter("Output"));
        parameterValueChanged(stream->getParameter("Time_Const"));
    }
}

void MeanSpikeRate::parameterValueChanged(Parameter* param)
{
    const uint16 streamId = param->getStreamId();

    if (param->getName().equalsIgnoreCase("Output"))
    {
        Array<var>* array = param->getValue().getArray();

        // Make sure there's a selected value
        if (array->size() > 0)
        {
            int localIndex = int(array->getFirst());
            int globalIndex = getDataStream(streamId)->getContinuousChannels()[localIndex]->getGlobalIndex();
            settings[streamId]->outputChan = globalIndex;
        }
        else
        {
            settings[streamId]->outputChan = -1;
        }
    }
    else if (param->getName().equalsIgnoreCase("Time_Const"))
    {
        settings[streamId]->timeConstMs = (float)param->getValue();
    }
}

int MeanSpikeRate::getNumActiveElectrodes()
{
    auto editor = static_cast<MeanSpikeRateEditor*>(getEditor());
    return editor->getNumActiveElectrodes();
}


void MeanSpikeRate::loadCustomParametersFromXml(XmlElement* parentElement)
{

    auto msrEditor = static_cast<MeanSpikeRateEditor*>(getEditor());

    forEachXmlChildElement(*parentElement, mainNode)
    {
        if (mainNode->hasTagName("MeanSpikeRate"))
        {
            forEachXmlChildElement(*mainNode, activeNode)
            {
                spikeChannelActive[activeNode->getStringAttribute("channel")] = activeNode->getBoolAttribute("isActive");
            }
        }
    }
}

void MeanSpikeRate::saveCustomParametersToXml(XmlElement* parentElement)
{
    auto editor = static_cast<MeanSpikeRateEditor*>(getEditor());
    XmlElement* mainNode = parentElement->createNewChildElement("MeanSpikeRate");
    for (auto spikeChannel : spikeChannels)
    {
        XmlElement* activeNode = mainNode->createNewChildElement("ACTIVE");
        activeNode->setAttribute("channel", spikeChannel->getIdentifier());
        activeNode->setAttribute("isActive", isActive(spikeChannel));
    }
}
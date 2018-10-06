/*
 ==============================================================================
 
 This file is part of SPARTA; a suite of spatial audio plug-ins.
 Copyright (c) 2018 - Leo McCormack.
 
 SPARTA is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 SPARTA is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with SPARTA.  If not, see <http://www.gnu.org/licenses/>.
 
 ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h" 

PluginProcessor::PluginProcessor()
{
    ringBufferInputs = new float*[MAX_NUM_CHANNELS];
    for (int i = 0; i < MAX_NUM_CHANNELS; i++)
        ringBufferInputs[i] = new float[FRAME_SIZE];
    
    ringBufferOutputs = new float*[MAX_NUM_CHANNELS];
    for (int i = 0; i < MAX_NUM_CHANNELS; i++)
        ringBufferOutputs[i] = new float[FRAME_SIZE];
    
	binauraliser_create(&hBin);
}

PluginProcessor::~PluginProcessor()
{
	binauraliser_destroy(&hBin);
    
    for (int i = 0; i < MAX_NUM_CHANNELS; ++i)
        delete[] ringBufferInputs[i];
    delete[] ringBufferInputs;
    
    for (int i = 0; i < MAX_NUM_CHANNELS; ++i)
        delete[] ringBufferOutputs[i];
    delete[] ringBufferOutputs;
}

void PluginProcessor::setParameter (int index, float newValue)
{
    if (index % 2 || index == 0)
        binauraliser_setSourceAzi_deg(hBin, index/2, (newValue - 0.5f)*360.0f);
    else
        binauraliser_setSourceElev_deg(hBin, (index-1)/2, (newValue - 0.5f)*180.0f);
}

void PluginProcessor::setCurrentProgram (int index)
{
}

float PluginProcessor::getParameter (int index)
{
    if (index % 2 || index == 0)
        return (binauraliser_getSourceAzi_deg(hBin, index/2)/360.0f) + 0.5f;
    else
        return (binauraliser_getSourceElev_deg(hBin, (index-1)/2)/180.0f) + 0.5f;
}

int PluginProcessor::getNumParameters()
{
	return MIN(binauraliser_getMaxNumSources(), NUM_OF_AUTOMATABLE_SOURCES);
}

const String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

const String PluginProcessor::getParameterName (int index)
{
    if (index % 2 || index == 0)
        return TRANS("Azim_") + String(index/2);
    else
        return TRANS("Elev_") + String((index-1)/2);
}

const String PluginProcessor::getParameterText(int index)
{
    if (index % 2 || index == 0)
        return String(binauraliser_getSourceAzi_deg(hBin, index/2));
    else
        return String(binauraliser_getSourceElev_deg(hBin, (index-1)/2));
}

const String PluginProcessor::getInputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

const String PluginProcessor::getOutputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 0;
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

const String PluginProcessor::getProgramName (int index)
{
    return String::empty;
}


bool PluginProcessor::isInputChannelStereoPair (int index) const
{
    return true;
}

bool PluginProcessor::isOutputChannelStereoPair (int index) const
{
    return true;
}


bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::silenceInProducesSilenceOut() const
{
    return false;
}

void PluginProcessor::changeProgramName (int index, const String& newName)
{
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	nHostBlockSize = samplesPerBlock;
	nSampleRate = (int)(sampleRate + 0.5);
    
    nNumInputs = getTotalNumInputChannels();
    nNumOutputs = getTotalNumOutputChannels();
 
	setPlayConfigDetails(nNumInputs, nNumOutputs, (double)nSampleRate, nHostBlockSize);
	numChannelsChanged();
    isPlaying = false;

	binauraliser_init(hBin, sampleRate);
    
    for (int i = 0; i < MAX_NUM_CHANNELS; ++i)
        memset(ringBufferInputs[i], 0, FRAME_SIZE*sizeof(float));
    for (int i = 0; i < MAX_NUM_CHANNELS; ++i)
        memset(ringBufferOutputs[i], 0, FRAME_SIZE * sizeof(float));
}

void PluginProcessor::releaseResources()
{
    isPlaying = false;
}

void PluginProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    int nCurrentBlockSize = buffer.getNumSamples();
    float** bufferData = buffer.getArrayOfWritePointers();
    float** outputs = new float*[nNumOutputs];
    for (int i = 0; i < nNumOutputs; i++)
        outputs[i] = new float[FRAME_SIZE];

    if(nCurrentBlockSize % FRAME_SIZE == 0){ /* divisible by frame size */
        for (int frame = 0; frame < nCurrentBlockSize/FRAME_SIZE; frame++) {
            for (int ch = 0; ch < nNumInputs; ch++)
                for (int i = 0; i < FRAME_SIZE; i++)
                    ringBufferInputs[ch][i] = bufferData[ch][frame*FRAME_SIZE + i];
            
            /* determine if there is actually audio in the damn buffer */
            playHead = getPlayHead();
            bool PlayHeadAvailable = playHead->getCurrentPosition(currentPosition);
            if (PlayHeadAvailable == true)
                isPlaying = currentPosition.isPlaying;
            else
                isPlaying = true;
            if(!isPlaying) /* for DAWs with no transport */
                isPlaying = buffer.getRMSLevel(0, 0, nCurrentBlockSize)>1e-5f ? true : false;
     
            /* perform processing */
            binauraliser_process(hBin, ringBufferInputs, ringBufferOutputs, nNumInputs, nNumOutputs, FRAME_SIZE, isPlaying);
            
            /* replace buffer with new audio */
            buffer.clear(frame*FRAME_SIZE, FRAME_SIZE);
            for (int ch = 0; ch < nNumOutputs; ch++)
                for (int i = 0; i < FRAME_SIZE; i++)
                    bufferData[ch][frame*FRAME_SIZE + i] = ringBufferOutputs[ch][i];
        }
    }
    else
        buffer.clear();
  
    for (int i = 0; i < nNumOutputs; ++i)
        delete[] outputs[i];
    delete[] outputs;
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; 
}

AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (this);
}

//==============================================================================
void PluginProcessor::getStateInformation (MemoryBlock& destData)
{
    XmlElement xml("BINAURALISERPLUGINSETTINGS");
    for(int i=0; i<binauraliser_getMaxNumSources(); i++){
        xml.setAttribute("SourceAziDeg" + String(i), binauraliser_getSourceAzi_deg(hBin,i));
        xml.setAttribute("SourceElevDeg" + String(i), binauraliser_getSourceElev_deg(hBin,i));
    }
    xml.setAttribute("nSources", binauraliser_getNumSources(hBin));
    
    if(!binauraliser_getUseDefaultHRIRsflag(hBin))
        xml.setAttribute("SofaFilePath", String(binauraliser_getSofaFilePath(hBin)));
    
    xml.setAttribute("JSONFilePath", lastDir.getFullPathName());
    
    copyXmlToBinary(xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    ScopedPointer<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr) {
        if (xmlState->hasTagName("BINAURALISERPLUGINSETTINGS")) {
            for(int i=0; i<binauraliser_getMaxNumSources(); i++){
                if(xmlState->hasAttribute("SourceAziDeg" + String(i)))
                    binauraliser_setSourceAzi_deg(hBin, i, (float)xmlState->getDoubleAttribute("SourceAziDeg" + String(i), 0.0f));
                if(xmlState->hasAttribute("SourceElevDeg" + String(i)))
                    binauraliser_setSourceElev_deg(hBin, i, (float)xmlState->getDoubleAttribute("SourceElevDeg" + String(i), 0.0f));
            }
            if(xmlState->hasAttribute("nSources"))
               binauraliser_setNumSources(hBin, xmlState->getIntAttribute("nSources", 1)); 
            
            if(xmlState->hasAttribute("SofaFilePath")){
                String directory = xmlState->getStringAttribute("SofaFilePath", "no_file");
                const char* new_cstring = (const char*)directory.toUTF8();
                binauraliser_setSofaFilePath(hBin, new_cstring);
            }
            
            if(xmlState->hasAttribute("JSONFilePath"))
                lastDir = xmlState->getStringAttribute("JSONFilePath", "");
            
            binauraliser_refreshSettings(hBin);
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}

void PluginProcessor::saveConfigurationToFile (File destination)
{
    sources.removeAllChildren(nullptr);
    for (int i=0; i<binauraliser_getNumSources(hBin);i++)
    {
        sources.appendChild (ConfigurationHelper::
                             createSource(binauraliser_getSourceAzi_deg(hBin, i),
                                          binauraliser_getSourceElev_deg(hBin, i),
                                          1.0f,
                                          i,
                                          false,
                                          1.0f), nullptr);
    }
    DynamicObject* jsonObj = new DynamicObject();
    jsonObj->setProperty("Name", var("SPARTA Binauraliser source directions."));
    char versionString[10];
    strcpy(versionString, "v");
    strcat(versionString, JucePlugin_VersionString);
    jsonObj->setProperty("Description", var("This configuration file was created with the SPARTA Binauraliser " + String(versionString) + " plug-in. " + Time::getCurrentTime().toString(true, true)));
    jsonObj->setProperty ("SourceArrangement", ConfigurationHelper::convertSourcesToVar (sources, "Source Directions"));
    Result result = ConfigurationHelper::writeConfigurationToFile (destination, var (jsonObj));
    assert(result.wasOk());
}

void PluginProcessor::loadConfiguration (const File& configFile)
{
    sources.removeAllChildren(nullptr);
    Result result = ConfigurationHelper::parseFileForSourceArrangement (configFile, sources, nullptr);
    if(!result.wasOk()){
        result = ConfigurationHelper::parseFileForLoudspeakerLayout (configFile, sources, nullptr);
    }
    if(result.wasOk()){
        int num_srcs = 0;
        int src_idx = 0;
        for (ValueTree::Iterator it = sources.begin() ; it != sources.end(); ++it)
            if ( !((*it).getProperty("Imaginary")))
                num_srcs++;
        binauraliser_setNumSources(hBin, num_srcs);
        for (ValueTree::Iterator it = sources.begin() ; it != sources.end(); ++it){
            if ( !((*it).getProperty("Imaginary"))){
                binauraliser_setSourceAzi_deg(hBin, src_idx, (*it).getProperty("Azimuth"));
                binauraliser_setSourceElev_deg(hBin, src_idx, (*it).getProperty("Elevation"));
                src_idx++;
            }
        }
    }
}



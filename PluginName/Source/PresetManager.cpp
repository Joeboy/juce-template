/*
  ==============================================================================

    PresetManager.cpp
    Created: 25 Nov 2021 9:02:19am
    Author:  Nicholas Berriochoa

  ==============================================================================
*/

#include "PresetManager.h"

//==============================================================================
PresetManager::PresetManager(juce::AudioProcessor* processor, juce::AudioProcessorValueTreeState* parameters)
:   processor(processor),
    parameters(parameters),
    mCurrentPresetIsSaved(false),
    mCurrentPresetName("Untitled")
{
    juce::String dir_sep = juce::File::getSeparatorString();
    mPresetDirectory = juce::File::getSpecialLocation(juce::File::userMusicDirectory).getFullPathName()
                       + dir_sep + JucePlugin_Manufacturer
                       + dir_sep + JucePlugin_Name
                       + dir_sep + "Presets";
    
    
    if (!juce::File(mPresetDirectory).exists()) {
        
        // create the preset directory
        if(juce::File(mPresetDirectory).createDirectory().fail("Failed to create preset directory")) {
            jassertfalse;
        }
    }
    
    processor->getStateInformation(presetA);
    processor->getStateInformation(presetB);
    
    storeLocalPreset();
}

PresetManager::~PresetManager()
{
    
}

//==============================================================================
void PresetManager::getXmlForPreset(juce::XmlElement* inElement)
{
    const juce::Array<juce::AudioProcessorParameter*>& parameters = processor->getParameters();
    
    for (int i = 0; i < parameters.size(); i++)
    {
        juce::AudioProcessorParameterWithID* parameter = (juce::AudioProcessorParameterWithID*)parameters.getUnchecked(i);
        
        inElement->setAttribute(parameter->paramID, parameter->getValue());
    }
}

void PresetManager::loadPresetForXml(juce::XmlElement* inElement)
{
    mCurrentPresetXml = inElement;
    
    const juce::Array<juce::AudioProcessorParameter*>& parameters = processor->getParameters();
    
    for (int i = 0; i < mCurrentPresetXml->getNumAttributes(); i++)
    {
        const juce::String paramID = mCurrentPresetXml->getAttributeName(i);
        const float value = mCurrentPresetXml->getDoubleAttribute(paramID);
        
        for (int j = 0; j < parameters.size(); j++) {
            juce::AudioProcessorParameterWithID* parameter = (juce::AudioProcessorParameterWithID*)parameters.getUnchecked(i);
            
            if (paramID == parameter->paramID)
            {
                parameter->setValueNotifyingHost(value);
            }
        }
    }
}

//==============================================================================
int PresetManager::getNumberOfPresets()
{
    return mLocalPresets.size();
}

juce::String PresetManager::getPresetName(int inPresetIndex)
{
    return mLocalPresets[inPresetIndex].getFileNameWithoutExtension();
}

juce::String PresetManager::getPresetPath()
{
    return mPresetDirectory;
}

//==============================================================================
void PresetManager::createNewPreset()
{
    const juce::Array<juce::AudioProcessorParameter*>& parameters = processor->getParameters();
    
    for (int i = 0; i < parameters.size(); i++)
    {
        juce::AudioProcessorParameterWithID* parameter = (juce::AudioProcessorParameterWithID*)parameters.getUnchecked(i);
        
        const float defaultValue = parameter->getDefaultValue();
        
        parameter->setValueNotifyingHost(defaultValue);
    }
    
    mCurrentPresetIsSaved = false;
    mCurrentPresetName = "Untitled";
}

void PresetManager::savePreset()
{
    juce::MemoryBlock destinationData;
    processor->getStateInformation(destinationData);
    
    mCurrentlyLoadedPreset.deleteFile();
    mCurrentlyLoadedPreset.appendData(destinationData.getData(),
                                      destinationData.getSize());
    
    mCurrentPresetIsSaved = true;
}

void PresetManager::saveAsPreset(juce::String inPresetName, juce::String notes)
{
    // path information
    juce::String dir_sep = juce::File::getSeparatorString();
    juce::File presetXML = juce::File(mPresetDirectory + dir_sep + inPresetName + PRESET_FILE_EXTENSION);
    
    // delete the preset file if it exists
    // save as overwrites the existing settings
    if (presetXML.exists()) {
        presetXML.deleteFile();
    }
    
    if (presetXML.create().fail("Failed to create xml file")) {
        jassertfalse;
    }
    
    // convert parameter state to xml and write to data to the file
    auto state = parameters->copyState();
    std::unique_ptr<juce::XmlElement> xml = state.createXml();
    xml->writeTo(presetXML);
    
    juce::ValueTree notesTree = juce::ValueTree(juce::Identifier("Notes"));
    
    mCurrentPresetIsSaved = true;
    mCurrentPresetName = inPresetName;
    
    storeLocalPreset();
}

void PresetManager::loadPreset(int inPresetIndex)
{
    mCurrentlyLoadedPreset = mLocalPresets[inPresetIndex];
    
    juce::MemoryBlock presetBinary;
    
    if (mCurrentlyLoadedPreset.loadFileAsData(presetBinary))
    {
        mCurrentPresetIsSaved = true;
        mCurrentPresetName = getPresetName(inPresetIndex);
        processor->setStateInformation(presetBinary.getData(),
                                        (int)presetBinary.getSize());
    }

}

//==============================================================================
bool PresetManager::getIsCurrentPresetSaved()
{
    return mCurrentPresetIsSaved;
}

juce::String PresetManager::getCurrentPresetName()
{
    return mCurrentPresetName;
}

//==============================================================================
void PresetManager::storeLocalPreset()
{
    mLocalPresets.clear();
            
    for (juce::DirectoryEntry entry: juce::RangedDirectoryIterator(juce::File(mPresetDirectory),
                                                                       false,
                                                                       "*" + (juce::String)PRESET_FILE_EXTENSION,
                                                                       juce::File::TypesOfFileToFind::findFiles))
    {
        juce::File preset = entry.getFile();
        mLocalPresets.add(preset);
    }
}

//==============================================================================
void PresetManager::populateViewItem(PresetViewItem* item)
{
    if (item->isDirectory) {
        juce::File directory(mPresetDirectory);
        
        juce::Array<juce::File> presets = directory.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.xml");
        
        for (juce::File f : presets) {
            item->addSubItem(new PresetViewItem(f.getFileName(), "", false, false));
        }
    }
}

//==============================================================================
void PresetManager::updateQuickPreset()
{
    // if this function was called from toggling the presets, exit.
    // this is necessary because calling toggleQuickPreset triggers
    // the listener for comboBoxChanged and sliderValueChanged which
    // then calls this function. This doesn't and shouldn't be done
    // so this boolean protects that from happening
    if (fromToggle) {
        return;
    }
    
    switch (quickPresetInUse) {
        case QuickPreset::Preset_A:
            
            DBG("Updated A");
            processor->getStateInformation(presetA);
            
            break;
        case QuickPreset::Preset_B:
            
            DBG("Updated B");
            processor->getStateInformation(presetB);
            
            break;
        default:
            jassertfalse;
    }
    
    fromToggle = false;
}

void PresetManager::toggleQuickPreset()
{
    fromToggle = true;
    
    switch (quickPresetInUse) {
        case QuickPreset::Preset_A:
            DBG("Swapped from A to B");
            // change the current preset and load the data
            quickPresetInUse = QuickPreset::Preset_B;
            processor->setStateInformation(presetB.getData(), (int)presetB.getSize());
            
            break;
        case QuickPreset::Preset_B:
            DBG("Swapped from B to A");
            // change the current preset and load the data
            quickPresetInUse = QuickPreset::Preset_A;
            processor->setStateInformation(presetA.getData(), (int)presetA.getSize());
            
            break;
        default:
            jassertfalse;
    }
}

void PresetManager::copyQuickPreset()
{
    switch (quickPresetInUse) {
        case QuickPreset::Preset_A:
            DBG("Copying A to B");
            
            // change the current preset and copy it to the other
            quickPresetInUse = QuickPreset::Preset_B;
            presetA.copyTo(presetB.getData(), 0, presetB.getSize());
            
            break;
        case QuickPreset::Preset_B:
            DBG("Copying B to A");
            
            // change the current preset and copy it to the other
            quickPresetInUse = QuickPreset::Preset_A;
            presetB.copyTo(presetA.getData(), 0, presetA.getSize());
            
            break;
        default:
            jassertfalse;
    }
}

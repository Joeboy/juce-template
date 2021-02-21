/*
  ==============================================================================

    PanelBase.cpp
    Created: 21 Feb 2021 6:50:45pm
    Author:  Nicholas Berriochoa

  ==============================================================================
*/

#include "PanelBase.h"

PanelBase::PanelBase(Template_AudioProcessor *inProcessor)
{
    mProcessor = inProcessor;
}

PanelBase::~PanelBase()
{
    
}

void PanelBase::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white);
    g.fillAll();
    
    g.setColour(juce::Colours::black);
    g.drawRoundedRectangle(0, 0, getWidth(), getHeight(), CORNER_SIZE, LINE_THICKNESS);
}
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/**
 * The same interface as ApplicationCommandTarget, but only the two methods to gets the Commands and its data,
 * the others have a default implementation that must NOT be called.
 * This is simply to decouple the Panels from the Commands, because declaring the Commands in the KeyMappingEditorComponent
 * must not be dynamic (else only the keys from already opened Panels are visible).
 *
 * EACH IMPLEMENTATION MUST BE DECLARED in createMainApplicationCommandManager method of main.cpp.
 */
class TargetCommands : public juce::ApplicationCommandTarget
{
public:
    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    bool perform(const InvocationInfo& info) override;
};

}   // namespace arkostracker


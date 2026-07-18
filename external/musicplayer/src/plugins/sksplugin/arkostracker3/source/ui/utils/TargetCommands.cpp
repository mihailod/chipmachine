#include "TargetCommands.h"

namespace arkostracker 
{

juce::ApplicationCommandTarget* TargetCommands::getNextCommandTarget()
{
    jassertfalse;       // Not implemented.
    return nullptr;
}

bool TargetCommands::perform(const juce::ApplicationCommandTarget::InvocationInfo& /*info*/)
{
    jassertfalse;       // Not implemented.
    return false;
}


}   // namespace arkostracker


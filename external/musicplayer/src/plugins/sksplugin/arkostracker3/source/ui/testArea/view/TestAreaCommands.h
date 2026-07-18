#pragma once

#include "../../utils/TargetCommands.h"

namespace arkostracker 
{

/** Groups the Commands of the Test Area. */
class TestAreaCommands final : public TargetCommands
{
public:

    // TargetCommands method implementations.
    // ==============================================
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
};

}   // namespace arkostracker


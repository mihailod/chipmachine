#pragma once

#include "../../utils/TargetCommands.h"

namespace arkostracker 
{

/** Groups the Commands of any Bar Editor. */
class BarEditorCommands final : public TargetCommands
{
public:

    // TargetCommands method implementations.
    // ==============================================
    void getAllCommands(juce::Array <juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandId, juce::ApplicationCommandInfo& result) override;
};

}   // namespace arkostracker

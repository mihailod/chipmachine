#pragma once

#include "../../utils/TargetCommands.h"

namespace arkostracker 
{

class PatternViewerController;

/** Groups the Commands of the PatternViewer Area. This does NOT include the virtual keyboard keys. */
class PatternViewerCommands final : public TargetCommands
{
public:
    explicit PatternViewerCommands(PatternViewerController& patternViewerController) noexcept;

    // TargetCommands method implementations.
    // ==============================================
    void getAllCommands(juce::Array <juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandId, juce::ApplicationCommandInfo& result) override;

private:
    PatternViewerController& patternViewerController;
};

}   // namespace arkostracker

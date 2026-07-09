#pragma once

#include "../utils/TargetCommands.h"

namespace arkostracker 
{

class LinkerController;

/** Groups the Commands of the Test Area. */
class LinkerPanelAreaCommands final : public TargetCommands
{
public:
    /**
     * Constructor
     * @param linkerController the Linker Controller.
     */
    explicit LinkerPanelAreaCommands(LinkerController& linkerController) noexcept;

    // TargetCommands method implementations.
    // ==============================================
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;

private:
    LinkerController& linkerController;
};

}   // namespace arkostracker

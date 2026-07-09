#pragma once

#include "../utils/TargetCommands.h"

namespace arkostracker 
{

class MainController;

/** Groups the Commands of the main application. */
class MainApplicationCommands final : public TargetCommands
{
public:
    /**
     * Constructor.
     * @param mainController to be able to check the enablement of some actions.
     */
    explicit MainApplicationCommands(MainController& mainController) noexcept;

    // TargetCommands method implementations.
    // ==============================================
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandId, juce::ApplicationCommandInfo& result) override;

private:
    /**
     * Defines data for the command info about Restore Layout.
     * @param layoutIndex the layout index (>=0).
     * @param keyPress the key to press.
     * @param result the command info to modify.
     * @param category the category of the command.
     */
    static void setResultForRestoreLayout(int layoutIndex, int keyPress, juce::ApplicationCommandInfo& result, const juce::String& category) noexcept;

    MainController& mainController;
};

}   // namespace arkostracker

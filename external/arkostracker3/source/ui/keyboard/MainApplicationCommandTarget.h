#pragma once

#include "MainApplicationCommands.h"
#include "VirtualKeyboardCommands.h"

namespace arkostracker 
{

class CommandPerformer;
class MainController;

/** Manages Commands for the Main Application. This is used by the MainComponent. */
class MainApplicationCommandTarget final : public juce::ApplicationCommandTarget
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param commandPerformer the objects that knows how to perform the commands.
     */
    explicit MainApplicationCommandTarget(MainController& mainController, CommandPerformer& commandPerformer) noexcept;

    // ApplicationCommandTarget method implementations.
    // ================================================
    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

private:
    MainController& mainController;
    CommandPerformer& commandPerformer;
    MainApplicationCommands mainApplicationCommands;
    VirtualKeyboardCommands virtualKeyboardCommands;            // The PatternViewer has one, the main component too!
};

}   // namespace arkostracker

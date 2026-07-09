#pragma once

#include "../utils/TargetCommands.h"

namespace arkostracker 
{

class OperationValidityProvider;

/** Groups the Commands of any list. */
class ListCommands final : public TargetCommands
{
public:
    /**
     * Constructor.
     * @param provider the provider of the validity of an operation.
     */
    explicit ListCommands(const OperationValidityProvider& provider) noexcept;

    // TargetCommands method implementations.
    // ==============================================
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;

protected:
    /** Asks the subclass to add its own command, if any. The default implementation adds load/save item commands. */
    static void getSubCommands(juce::Array<juce::CommandID>& commands) noexcept;
    /** Asks the subclass to add its own command info, if any. The default implementation defines the load/save item commands. */
    void getSubCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) const noexcept;

    const OperationValidityProvider& operationValidityProvider;
};

}   // namespace arkostracker

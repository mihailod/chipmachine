#include "CommandManagerCreator.h"

namespace arkostracker 
{

std::unique_ptr<juce::ApplicationCommandManager> CommandManagerCreator::create(Category categoryToKeep,
                                                                               const juce::ApplicationCommandManager& previousApplicationCommandManager) noexcept
{
    auto applicationCommandManager = std::make_unique<juce::ApplicationCommandManager>();

    // Takes all the Commands of the given Category.
    for (const auto commandId : previousApplicationCommandManager.getCommandsInCategory(CategoryUtils::getCategoryString(categoryToKeep))) {
        const auto* commandInfo = previousApplicationCommandManager.getCommandForID(commandId);

        // Puts the Command in the new ApplicationCommandManager.
        applicationCommandManager->registerCommand(*commandInfo);
    }

    return applicationCommandManager;
}


}   // namespace arkostracker


#include "TipsController.h"

#include "Tips.h"

namespace arkostracker 
{

TipsController::TipsController(juce::ApplicationCommandManager& pApplicationCommandManager) noexcept :
        applicationCommandManager(pApplicationCommandManager),
        lastTipIndex(0)
{
}

Tip TipsController::getRandomTip() const noexcept
{
    auto random = juce::Random();
    lastTipIndex = random.nextInt();
    //lastTipIndex = 0;       // Useful for testing.

    return getTip();
}

Tip TipsController::getNextTip() const noexcept
{
    ++lastTipIndex;
    return getTip();
}

Tip TipsController::getTip() const noexcept
{
    const auto& tips = Tips::getTips();
    const auto index = static_cast<size_t>(lastTipIndex) % tips.size();
    return buildTip(tips.at(index));
}

Tip TipsController::buildTip(const Tip& rawTip) const noexcept
{
    const auto* keyMappings = applicationCommandManager.getKeyMappings();
    if (keyMappings == nullptr) {
        jassertfalse;
        return Tip({ });
    }

    // Substitutes the possible CommandId placeholders.
    auto newText = rawTip.getText();
    for (const auto commandId : rawTip.getCommandIds()) {
        juce::String keyText;
        const auto keysPressed = keyMappings->getKeyPressesAssignedToCommand(commandId);
        if (keysPressed.isEmpty()) {
            // There is no key.
            keyText = juce::translate("[Unassigned]");
        } else {
            // Generates a readable description of the keys.
            auto isFirst = true;
            for (const auto& keyPressed : keysPressed) {
                // Adds a separator.
                if (!isFirst) {
                    keyText += " or ";
                }
                isFirst = false;
                keyText += "[" + keyPressed.getTextDescription() + "]";
            }
        }

        // Replaces the placeholder.
        static const juce::String placeholder("XXX");

        jassert(newText.contains(placeholder));     // More commands than placeholders?
        newText = newText.replaceFirstOccurrenceOf(placeholder, keyText);
    }

    // Makes the local path absolute.
    const auto fullImagePath = getFullPath(rawTip.getImageName());
    const auto fullZippedVideoPath = getFullPath(rawTip.getZippedVideoName());

    return Tip(newText, fullImagePath, fullZippedVideoPath, rawTip.getCommandIds(), rawTip.getImageWidth());
    // For testing the paths.
    //return Tip("image= " + fullImagePath + ", video = " + fullZippedVideoPath, fullImagePath, fullZippedVideoPath);
}

juce::String TipsController::getFullPath(const juce::String& localPath) noexcept
{
    if (localPath.isEmpty()) {
        return localPath;
    }

#ifdef JUCE_DEBUG
    const juce::String tipsPath("tips/");

    const auto fileAbsolutePath = juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentExecutableFile)
#ifdef JUCE_MAC
        .getParentDirectory().getParentDirectory().getParentDirectory()
#endif
        .getParentDirectory().getParentDirectory().getParentDirectory().getParentDirectory().getParentDirectory().getFullPathName() +
                                    juce::File::getSeparatorString() + tipsPath + localPath;
#else
    const juce::String tipsPath("private/tips/");

    const auto fileAbsolutePath = juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentExecutableFile)
#ifdef JUCE_MAC
        .getParentDirectory().getParentDirectory().getParentDirectory().getParentDirectory()
#else
        .getParentDirectory()       // Linux.
#endif
            .getFullPathName() + juce::File::getSeparatorString() + tipsPath + localPath;
#endif

    return fileAbsolutePath;
}

}   // namespace arkostracker

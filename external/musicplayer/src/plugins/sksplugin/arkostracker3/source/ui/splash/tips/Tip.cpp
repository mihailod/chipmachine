#include "Tip.h"

namespace arkostracker 
{

Tip::Tip(juce::String pText, juce::String pImageName, juce::String pZippedVideoName, std::vector<CommandIds> pCommandIds, const int pImageWidth) noexcept :
        text(std::move(pText)),
        imageName(std::move(pImageName)),
        zippedVideoName(std::move(pZippedVideoName)),
        commandIds(std::move(pCommandIds)),
        imageWidth(pImageWidth)
{
}

Tip Tip::buildTextTip(juce::String text, std::vector<CommandIds> commandIds) noexcept
{
    return Tip(std::move(text), juce::String(), juce::String(), std::move(commandIds));
}

Tip Tip::buildImageTip(juce::String text, juce::String imageName, std::vector<CommandIds> commandIds, const int imageWidth) noexcept
{
    return Tip(std::move(text), std::move(imageName), juce::String(), std::move(commandIds), imageWidth);
}

Tip Tip::buildVideoTip(juce::String text, juce::String zippedVideoName, std::vector<CommandIds> commandIds, const int videoWidth) noexcept
{
    return Tip(std::move(text), juce::String(), std::move(zippedVideoName), std::move(commandIds), videoWidth);
}

const juce::String& Tip::getText() const noexcept
{
    return text;
}

const juce::String& Tip::getImageName() const noexcept
{
    return imageName;
}

const juce::String& Tip::getZippedVideoName() const noexcept
{
    return zippedVideoName;
}

const std::vector<CommandIds>& Tip::getCommandIds() const noexcept
{
    return commandIds;
}

int Tip::getImageWidth() const noexcept
{
    return imageWidth;
}

}   // namespace arkostracker

#pragma once

#include <juce_core/juce_core.h>

#include "../../keyboard/CommandIds.h"

namespace arkostracker 
{

/** A tip (consisting of a text and a possible path to an image/video). */
class Tip
{

public:
    /**
     * Constructor.
     * @param text the text. May contain "XXX" for placeholders of commandIds.
     * @param imageName the local image name, if used, or empty ("myImage.jpg" for example).
     * @param zippedVideoName the local video name, if used, or empty ("myVideo.zip" for example).
     * @param commandIds possible CommandIds. Must match the "XXX" count in the text.
     * @param imageWidth possible image width, or <=0.
     */
    explicit Tip(juce::String text, juce::String imageName = juce::String(), juce::String zippedVideoName = juce::String(),
        std::vector<CommandIds> commandIds = { }, int imageWidth = 0) noexcept;

    /**
     * @return a Tip with a text only.
     * @param text the text.
     * @param commandIds possible CommandIds. Must match the "XXX" count in the text.
     */
    static Tip buildTextTip(juce::String text, std::vector<CommandIds> commandIds = { }) noexcept;

    /**
     * @return a Tip with an image.
     * @param text the text.
     * @param imageName the local image name, if used, or empty ("myImage.jpg" for example).
     * @param commandIds possible CommandIds. Must match the "XXX" count in the text.
     * @param imageWidth possible image width, or <=0.
     */
    static Tip buildImageTip(juce::String text, juce::String imageName, std::vector<CommandIds> commandIds = { }, int imageWidth = 0) noexcept;

    /**
     * @return a Tip with an image.
     * @param text the text.
     * @param zippedVideoName the local video name, if used, or empty ("myVideo.zip" for example).
     * @param commandIds possible CommandIds. Must match the "XXX" count in the text.
     * @param videoWidth possible video width, or <=0.
     */
    static Tip buildVideoTip(juce::String text, juce::String zippedVideoName, std::vector<CommandIds> commandIds = { }, int videoWidth = 0) noexcept;

    /** @return the text of the tip. */
    const juce::String& getText() const noexcept;
    /** @return the image name, if used, or empty. */
    const juce::String& getImageName() const noexcept;
    /** @return the video name, if used, or empty. */
    const juce::String& getZippedVideoName() const noexcept;
    /** @return the possible CommandIds. */
    const std::vector<CommandIds>& getCommandIds() const noexcept;
    /** @return the possible image width, or <=0. */
    int getImageWidth() const noexcept;

private:
    juce::String text;
    juce::String imageName;
    juce::String zippedVideoName;
    std::vector<CommandIds> commandIds;
    int imageWidth;
};

}   // namespace arkostracker

#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../backgroundOperation/BackgroundOperation.h"

namespace arkostracker
{

/** Displays a series of JPG image from inside a ZIP. */
class ZippedImages final : public juce::Component,
                           juce::Timer
{
public:
    /**
     * Constructor.
     * @param zipFile the file. It will be loaded asynchronously.
     * @param frameDurationMs how long to wait between each frame, in ms.
     */
    ZippedImages(juce::File zipFile, int frameDurationMs) noexcept;

    /** Destructor. */
    ~ZippedImages() override;

    /** Attempts at showing the image. This is done asynchronously. */
    void show() noexcept;

    // Component method implementations.
    // ====================================
    void resized() override;

private:
    static const int startEndFrameRepetition;              // How many repeats of the first and last frames, to simulate a pause.

    // Timer method implementations.
    // ====================================
    void timerCallback() override;

    // ====================================

    void onBackgroundOperationFinished(std::unique_ptr<std::vector<juce::Image>> result) noexcept;

    /** Shows the image, and updates the index to know which next one to display on the next time. */
    void showImageAndDetermineNextImage() noexcept;

    const int frameDurationMs;                              // How long to wait between each image, in ms.
    juce::File inputFile;
    std::unique_ptr<std::vector<juce::Image>> images;       // Empty if nothing can be shown. Never null.
    size_t shownImageIndex;
    juce::ImageComponent imageComponent;                    // Where each image is shown.
    int waitCounter;                                        // Starts at X and decreases to 0, in order to simulate a pause at the beginning and end.

    std::unique_ptr<BackgroundOperation<std::unique_ptr<std::vector<juce::Image>>>> backgroundOperation;
};

}   // namespace arkostracker

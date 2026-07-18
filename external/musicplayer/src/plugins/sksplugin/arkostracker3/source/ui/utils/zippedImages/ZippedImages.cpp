#include "ZippedImages.h"

#include "../../../utils/NumberUtil.h"
#include "../backgroundOperation/BackgroundOperationWithDialog.h"

namespace arkostracker
{

const int ZippedImages::startEndFrameRepetition = 5;

ZippedImages::ZippedImages(juce::File pZipFile, const int pFrameDurationMs) noexcept :
        frameDurationMs(pFrameDurationMs),
        inputFile(std::move(pZipFile)),
        images(std::make_unique<std::vector<juce::Image>>()),
        shownImageIndex(0),
        waitCounter(startEndFrameRepetition),
        backgroundOperation()
{
    addAndMakeVisible(imageComponent);
}

ZippedImages::~ZippedImages()
{
    stopTimer();
}

void ZippedImages::show() noexcept
{
    jassert(backgroundOperation == nullptr);

    backgroundOperation = std::make_unique<BackgroundOperation<std::unique_ptr<std::vector<juce::Image>>>>(
        [&](std::unique_ptr<std::vector<juce::Image>> result) {
            onBackgroundOperationFinished(std::move(result));
        },
        [&]() -> std::unique_ptr<std::vector<juce::Image>> {
            // Worker thread.

            // Tries to load the image.
            if (!inputFile.existsAsFile()) {
                DBG("Unable to load image path: " + inputFile.getFullPathName());
                return nullptr;
            }

            juce::ZipFile zipFile(inputFile);
            const auto numEntries = zipFile.getNumEntries();
            if (numEntries <= 0) {
                return nullptr;
            }

            auto readImages = std::make_unique<std::vector<juce::Image>>();

            // Extracts each image from the ZIP file.
            for (auto imageNumber = 1; imageNumber <= numEntries; ++imageNumber) {
                // Each image is called "frameXXX.jpg", starting at 001.
                const auto entryName("frame" + NumberUtil::toDecimalString(imageNumber, 3) + ".jpg");
                const auto* entry = zipFile.getEntry(entryName);
                if (entry == nullptr) {
                    jassertfalse; // Unable to read the entry!
                    continue;
                }
                // This is a ZIP file. WARNING, according to the doc, we must delete the stream by ourselves, and it WILL NOT outlive the ZipFile.
                // So wraps it with a unique_ptr (take care of the deletion part only).
                auto zipStream = std::unique_ptr<juce::InputStream>(zipFile.createStreamForEntry(*entry));
                if (zipStream == nullptr) {
                    jassertfalse; // Unable to read the entry!
                    continue;
                }
                // Decodes the image.
                auto image = juce::ImageFileFormat::loadFrom(*zipStream);
                if (image.isNull()) {
                    jassertfalse; // Unable to decode the entry!
                    continue;
                }
                readImages->push_back(image);
            }
            return readImages;
        }
    );

    backgroundOperation->performOperation();
}

void ZippedImages::onBackgroundOperationFinished(std::unique_ptr<std::vector<juce::Image>> result) noexcept
{
    // UI thread.

    images = std::move(result);
    if ((images == nullptr) || (images->empty())) {
        jassertfalse;           // Nothing loaded.
        return;
    }

    // Starts the timer and shows the image now that we know they are present.
    showImageAndDetermineNextImage();
    startTimer(frameDurationMs);
}


// Timer method implementations.
// ====================================

void ZippedImages::timerCallback()
{
    showImageAndDetermineNextImage();
}


// Component method implementations.
// ====================================

void ZippedImages::resized()
{
    imageComponent.setBounds(getLocalBounds());
}

void ZippedImages::showImageAndDetermineNextImage() noexcept
{
    imageComponent.setImage(images->at(shownImageIndex));

    // Needs to stay a bit on the latest image?
    if (--waitCounter > 0) {
        return;
    }

    const auto imageCount = images->size();
    shownImageIndex = (shownImageIndex + 1) % imageCount;

    // Stays a bit on this image? Only for the first and last.
    waitCounter = ((shownImageIndex == 0) || (shownImageIndex == (imageCount - 1))) ? startEndFrameRepetition : 0;
}

}   // namespace arkostracker

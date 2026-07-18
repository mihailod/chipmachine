#include "KeyboardView.h"

#include "../../../../utils/NumberUtil.h"

namespace arkostracker 
{

const float KeyboardView::whiteKeyDefaultWidth = 20.0F;
const float KeyboardView::blackKeyRatioHeight = 0.6F;
const float KeyboardView::blackKeyRatioWidth = 0.7F;

const float KeyboardView::scrollWheelMultiplier = 100.0F;
const float KeyboardView::scrollWheelMultiplierWhenZoomed = 2.0F;

const int KeyboardView::keyRefreshMs = 150;
const int KeyboardView::pressedDurationMs = keyRefreshMs;

const float KeyboardView::minimumZoom = 1.0F;
const float KeyboardView::maximumZoom = 1.5F;

KeyboardView::KeyboardView(const int pKeyCount, std::function<void(int)> pOnKeyboardKeyPressed) noexcept :
        keyCount(pKeyCount),
        keyboardKeyPressed(std::move(pOnKeyboardKeyPressed)),
        zoom(1.0F),
        viewport(),
        keysContainer(),
        keys(),
        previousHeight(-1),
        previousZoom(-1.0F),
        keyNumberToReleaseTimestamp()
{
    setMouseClickGrabsKeyboardFocus(false);
    setInterceptsMouseClicks(true, false);

    // The size of the Viewport and the container will be set on resize or on the building of the keyboard.
    viewport.setViewedComponent(&keysContainer, false);
    viewport.setScrollBarsShown(false, false, false, false);

    addAndMakeVisible(viewport);
    addAndMakeVisible(keysContainer);

    buildKeyboard();

    startTimer(keyRefreshMs);
}

KeyboardView::~KeyboardView()
{
    stopTimer();
}


// Component method implementations.
// ====================================

void KeyboardView::resized()
{
    // The keyboard must be refreshed.
    buildKeyboard();

    // Resizes the Viewport.
    viewport.setBounds(0, 0, getWidth(), getHeight());
}

void KeyboardView::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (!event.mods.isShiftDown()) {
        // No shift: scrolls the keyboard.
        // Adds a multiplier so that when zoomed, the scroll is faster.
        const auto tempZoom = std::max(zoom * ((zoom - 1.0F) * scrollWheelMultiplierWhenZoomed), 1.0F);
        viewport.setViewPosition(viewport.getViewPositionX() - static_cast<int>(wheel.deltaY * scrollWheelMultiplier * tempZoom), 0);
    } else {
        // Zooms.
        const auto tempZoom = zoom + wheel.deltaY;
        zoom = NumberUtil::correctNumber(tempZoom, minimumZoom, maximumZoom);
        // Recalculates the size and position of the keys.
        buildKeyboard();
    }
}


// juce::Timer method implementations.
// ====================================

void KeyboardView::timerCallback()
{
    manageKeyRelease();
}


// ====================================

void KeyboardView::buildKeyboard() noexcept
{
    // Don't build the keys if they are already built.
    if (keys.empty()) {
        for (auto noteIndex = 0; noteIndex < keyCount; ++noteIndex) {
            const auto noteInOctave = noteIndex % 12;
            const auto octave = noteIndex / 12;
            // Builds the 8 white keys and 4 black keys. They are NOT sized and located here.
            const auto octaveString = juce::String(octave);

            const auto white = ((noteInOctave != 1) && (noteInOctave != 3) && (noteInOctave != 6) && (noteInOctave != 8) && (noteInOctave != 10));
            const auto zOrder = white ? 0 : -1;            // Z keys are in front.
            // The first key has the octave on it.
            auto key = std::make_unique<KeyView>(octave * 12 + noteInOctave, white, (noteInOctave == 0) ? octaveString : juce::String(),
                                                 [&] (const int pressedNote) { onKeyboardKeyPressed(pressedNote); });
            keysContainer.addAndMakeVisible(*key, zOrder);
            // Stores the key.
            keys.push_back(std::move(key));
        }
    }

    // The keys are stored and shows. But the size and location must be refreshed!
    // However, nothing to do if the height and zoom haven't changed!
    const auto height = getHeight();
    if ((previousHeight == height) && juce::exactlyEqual(previousZoom, zoom)) {
        return;
    }
    previousZoom = zoom;
    previousHeight = height;

    const auto whiteKeysHeight = height;
    const auto blackKeysHeight = static_cast<int>(static_cast<float>(whiteKeysHeight) * blackKeyRatioHeight);

    const auto whiteKeysWidth = static_cast<int>(whiteKeyDefaultWidth * zoom);
    const auto blackKeysWidth = static_cast<int>(static_cast<float>(whiteKeysWidth) * blackKeyRatioWidth);
    const auto shiftBlackKeyX = blackKeysWidth / 2;

    // The keys are stored in order, so it's easy to locate them.
    auto x = 0;
    for (auto& viewPtr : keys) {
        if (auto& keyView = *viewPtr; keyView.isWhite()) {
            keyView.setBounds(x, 0, whiteKeysWidth, whiteKeysHeight);
            x += whiteKeysWidth;
        } else {
            // The key is black. X slightly shifted to the left.
            keyView.setBounds(x - shiftBlackKeyX, 0, blackKeysWidth, blackKeysHeight);
        }
    }

    // Finally, the container must be resized. This is done according to the last key, and the height of the current Component.
    const auto iterator = keys.crbegin();
    const auto containerWidth = (*iterator)->getRight();
    keysContainer.setSize(containerWidth, whiteKeysHeight);        // Use the white keys as height.
}

void KeyboardView::onKeyboardKeyPressed(const int keyNumber) const noexcept
{
    keyboardKeyPressed(keyNumber);
}

void KeyboardView::showKeyAsPressed(const int noteNumber) noexcept
{
    // Deletes the possibly stored item first, before creating a new one.
    keyNumberToReleaseTimestamp.erase(noteNumber);

    // Stores the moment where the key must be released.
    const auto releaseTime = juce::Time::currentTimeMillis() + pressedDurationMs;
    keyNumberToReleaseTimestamp[noteNumber] = releaseTime;

    setKeyboardKeyState(noteNumber, true);
}

void KeyboardView::setKeyboardKeyState(const int noteNumber, const bool pressed) const noexcept
{
    if ((noteNumber < 0) || (noteNumber >= static_cast<int>(keys.size()))) {
        jassertfalse;       // Out of bounds key! Abnormal.
        return;
    }

    keys.at(static_cast<size_t>(noteNumber))->setPressedState(pressed);
}

void KeyboardView::manageKeyRelease() noexcept
{
    const auto now = juce::Time::currentTimeMillis();

    // Browses through the map to see what keys must be released.
    for (auto iterator = keyNumberToReleaseTimestamp.begin(); iterator != keyNumberToReleaseTimestamp.end();) {     // No increment!
        const auto readTimeStamp = iterator->second;
        if (readTimeStamp <= now) {
            // The key is too "old". Removes it from the map, and releases it visually.
            const auto noteNumber = iterator->first;
            setKeyboardKeyState(noteNumber, false);

            iterator = keyNumberToReleaseTimestamp.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

}   // namespace arkostracker

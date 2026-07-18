#include "BarArea.h"

#include "../../../../utils/NumberUtil.h"
#include "../../controller/BarAreaController.h"

namespace arkostracker
{

const int BarArea::separatorWidth = 3;
const int BarArea::minimumHeightBeforeConsideredHidden = 40;

BarArea::BarArea(const AreaType pAreaType, BarAreaController& pController, const int pBarWidth) noexcept:
        areaType(pAreaType),
        controller(pController),
        barWidthWithSpacing(pBarWidth),
        captionedBars(),
        hiddenPlaceholder(static_cast<int>(LookAndFeelConstants::Colors::panelBackgroundUnfocused)),
        latestWheelEventTimestamp(0)
{
    // Asks to get all the events!
    addMouseListener(this, true);

    setWantsKeyboardFocus(true);

    addChildComponent(hiddenPlaceholder);

    hiddenPlaceholder.setTransformationOnColour([] (const juce::Colour color){
        return color.contrasting(0.2F);
    });
}

BarArea::~BarArea()
{
    removeMouseListener(this);
}


// Component method implementations.
// ====================================

void BarArea::resized()
{
    const auto hidden = isHidden();

    // The width of the bars does not depend on their parent. The height is, though, so it must be refreshed.
    for (const auto& bar: captionedBars) {
        bar->setSize(bar->getWidth(), getHeight());
        bar->setVisible(!hidden);         // Either the placeholder is here, either the bars.
    }

    locatePlaceholder();
}

void BarArea::mouseDown(const juce::MouseEvent& event)
{
    manageClick(event, false);
}

void BarArea::mouseDrag(const juce::MouseEvent& event)
{
    manageClick(event, true);
}

void BarArea::mouseMove(const juce::MouseEvent& originalEvent)
{
    const auto event = originalEvent.getEventRelativeTo(this);
    auto* component = getComponentAt(event.getPosition());
    // Only a Bar interests us.
    const auto* bar = dynamic_cast<Bar*>(component);
    if (bar != nullptr) {
        const auto barIndex = bar->getIndex();
        controller.onMouseMoveOverBar(areaType, barIndex);
    }
}

void BarArea::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    const auto consumed = manageWheel(event, wheel);
    if (!consumed) {
        Component::mouseWheelMove(event, wheel);
    }
}

void BarArea::mouseExit(const juce::MouseEvent& /*event*/)
{
    // The event is used to un-hover a bar.
    // Strangely enough, this method is called when the mouse is between two Bars... Good for us, even though strange.
    // TO IMPROVE: after a drag+set value, this is called, so the view might un-hover, then hover again on mouse move.
    // Couldn't find a trick to prevent that.
    controller.onMouseExit();
}


// ====================================

AreaType BarArea::getAreaType() const noexcept
{
    return areaType;
}

void BarArea::setBarData(const int index, const BarData& barData, const BarCaptionDisplayedData& captionData) const noexcept
{
    if (index >= static_cast<int>(captionedBars.size())) {
        return;         // Can happen and removing after creating bars, in the mouseExit, but hard to reproduce.
    }
    captionedBars.at(static_cast<size_t>(index))->setDisplayedData(barData, captionData);
}

void BarArea::setDisplayedBarCount(const int newBarCountInt) noexcept
{
    const auto newBarCount = static_cast<size_t>(newBarCountInt);
    const auto currentBarCount = captionedBars.size();
    if (newBarCount < currentBarCount) {
        // Removes bars.
        captionedBars.resize(newBarCount);
        locatePlaceholder();
    } else if (newBarCount > currentBarCount) {
        // Adds new bars with default data.
        const BarData barData(false, 0, 10, 0,
                              LookAndFeelConstants::Colors::barSoundTypeBackground, LookAndFeelConstants::Colors::barSoundTypeSoft, // Random.
                              false, false, false, false, true, false, true, false);
        const BarCaptionDisplayedData captionData(juce::Image(), juce::String(), true, true, false, false);

        auto barCountToCreate = newBarCount - currentBarCount;
        while (barCountToCreate > 0U) {
            // The location is set below.
            auto captionedBar = std::make_shared<CaptionedBar>(static_cast<int>(captionedBars.size()));
            captionedBar->setDisplayedData(barData, captionData);
            // Note: do NOT disable the mouse clicks on the children, else we won't be able to catch them. (bar->setInterceptsMouseClicks(false, false))
            addAndMakeVisible(*captionedBar);

            captionedBars.push_back(std::move(captionedBar));
            locateAndResizeBars();

            --barCountToCreate;
        }
    }
}

void BarArea::setBarWidth(const int barWidth) noexcept
{
    if (barWidth == barWidthWithSpacing) {
        return;
    }
    barWidthWithSpacing = barWidth;

    locateAndResizeBars();
}

int BarArea::getBarX(const int index) const noexcept
{
    if (index >= static_cast<int>(captionedBars.size())) {
        jassertfalse;
        return 0;
    }
    return captionedBars.at(static_cast<size_t>(index))->getX();
}

int BarArea::getBarWidth() const noexcept
{
    return barWidthWithSpacing;
}

CaptionedBar* BarArea::getCaptionedBarFromMouseEvent(const juce::MouseEvent& originalEvent) noexcept
{
    const auto event = originalEvent.getEventRelativeTo(this);
    auto* component = getComponentAt(event.getPosition());
    while (component != nullptr) {
        // Is it a Captioned Bar? If yes, success.
        auto* captionedBar = dynamic_cast<CaptionedBar*>(component);
        if (captionedBar != nullptr) {
            return captionedBar;
        }

        // Searches for the parent that can be a CaptionedBar, because the clicked Component can be low-level.
        component = component->getParentComponent();
    }

    return nullptr;         // Not found.
}

Bar* BarArea::getBarFromMouseEvent(const juce::MouseEvent& originalEvent, const bool isDrag) noexcept
{
    const auto event = originalEvent.getEventRelativeTo(this);
    const auto eventPosition = event.getPosition();
    // First, simple test within bounds.
    auto* component = dynamic_cast<Bar*>(getComponentAt(eventPosition));
    if (!isDrag) {
        return component;
    }
    // If dragging, we want to be able to target a Bar even if out of bounds vertically,
    // which is more user-friendly.
    // Using 0 as Y works fine because the bars are on the top of the area. This simple
    // test will take care of both upper and lower out of bounds, because this method only helps
    // targeting a Bar. The value to set is done from the event, which takes the original event
    // in account, not the test performed here.
    if (component == nullptr) {
        component = dynamic_cast<Bar*>(getComponentAt(eventPosition.withY(0)));
    }

    return component;
}

void BarArea::manageClick(const juce::MouseEvent& event, const bool isDrag) noexcept
{
    // What is the selected Bar? Only a Bar interests us (not a CaptionBar) because we don't want any click on the caption.
    const auto* bar = getBarFromMouseEvent(event, isDrag);
    if (bar == nullptr) {
        return;
    }

    const auto barIndex = bar->getIndex();
    const auto value = bar->determineValue(event.y);

    controller.onUserClickOnBar(areaType, barIndex, value, event, isDrag);
}

bool BarArea::manageWheel(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) noexcept
{
    // There must be shift/ctrl modifier, else we're not interested.
    if ((!event.mods.isShiftDown() && !event.mods.isCommandDown()) || juce::exactlyEqual(wheel.deltaY, 0.0F)) {
        return false;
    }

    // Strangely enough, wheel events are received multiple times. Discard those that are too close.
    const auto newEventTimestamp = event.eventTime.toMilliseconds();
    if (std::abs(latestWheelEventTimestamp - newEventTimestamp) < 30) {
        return true;                // Considers as "managed" anyway.
    }
    latestWheelEventTimestamp = newEventTimestamp;

    const auto* captionedBar = getCaptionedBarFromMouseEvent(event);
    if (captionedBar == nullptr) {
        return false;
    }

    const auto barIndex = captionedBar->getIndex();
    const auto value = captionedBar->getCurrentValue();

    // What speed? Slower to faster: shift, ctrl, s+c.
    const auto mods = event.mods;
    auto modifierSpeed = Speed::normal;
    if (mods.isShiftDown()) {
        if (mods.isCtrlDown()) {
            modifierSpeed = Speed::fastest;
        }
    } else if (mods.isCtrlDown()) {
        modifierSpeed = Speed::fast;
    }

    controller.onUserWantsToSlideValue(areaType, barIndex, value, modifierSpeed, (wheel.deltaY > 0.0));

    return true;
}

int BarArea::getValue(const int barIndex) const noexcept
{
    const auto barIndexU = static_cast<size_t>(barIndex);
    if (captionedBars.size() <= barIndexU) {
        return 0;
    }

    return captionedBars.at(barIndexU)->getCurrentValue();
}

void BarArea::locateAndResizeBars() noexcept
{
    const auto barHeight = getHeight();
    const auto barWidth = barWidthWithSpacing - separatorWidth;
    jassert(barWidth > 0);
    const auto hidden = isHidden();

    auto index = 0;
    for (const auto& captionedBar: captionedBars) {
        captionedBar->setBounds(index * barWidthWithSpacing, 0, barWidth, barHeight);
        captionedBar->setVisible(!hidden);
        ++index;
    }

    locatePlaceholder();
}

bool BarArea::isHidden() const noexcept
{
    return (getHeight() < minimumHeightBeforeConsideredHidden);
}

void BarArea::locatePlaceholder() noexcept
{
    constexpr auto placeholderHeight = 2;

    const auto height = getHeight();
    const auto hidden = isHidden();

    // Locates the placeholder, shown only when the size is too small (used when the row is hidden!).
    const auto barCount = captionedBars.size();
    hiddenPlaceholder.setVisible(hidden);
    const auto placeholderWidth = (barCount == 0) ? 0 : captionedBars[barCount - 1]->getRight();
    const auto placeholderLeft = (barCount == 0) ? 0 : captionedBars[0]->getX();
    hiddenPlaceholder.setBounds(placeholderLeft, (height - placeholderHeight) / 2, placeholderWidth - placeholderLeft, placeholderHeight);
}

}   // namespace arkostracker

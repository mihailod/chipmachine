#include "BarData.h"

namespace arkostracker 
{

BarData::BarData(const bool pShortBar, const int pMinimumValue, const int pMaximumValue, const int pValue, const LookAndFeelConstants::Colors pBarBackgroundColorId,
                 const LookAndFeelConstants::Colors pBarColorId, const bool pWithCursor, const bool pSelected, const bool pHovered, const bool pWithinLoop,
                 const bool pOutOfBounds, const bool pGenerated, const bool pIgnored, const bool pRetrig) noexcept :
        shortBar(pShortBar),
        minimumValue(pMinimumValue),
        maximumValue(pMaximumValue),
        value(pValue),
        barBackgroundColorId(pBarBackgroundColorId),
        barColorId(pBarColorId),
        withCursor(pWithCursor),
        selected(pSelected),
        hovered(pHovered),
        withinLoop(pWithinLoop),
        outOfBounds(pOutOfBounds),
        generated(pGenerated),
        ignored(pIgnored),
        retrig(pRetrig)
{
}

BarData::BarData() noexcept :
        shortBar(false),
        minimumValue(0),
        maximumValue(1),
        value(0),
        barBackgroundColorId(LookAndFeelConstants::Colors::barEnvelopeBackground),      // Random color...
        barColorId(LookAndFeelConstants::Colors::barEnvelopeSoft),
        withCursor(false),
        selected(false),
        hovered(false),
        withinLoop(false),
        outOfBounds(true),
        generated(false),
        ignored(false),
        retrig(false)
{
}

bool BarData::isShortBar() const noexcept
{
    return shortBar;
}

int BarData::getMinimumValue() const noexcept
{
    return minimumValue;
}

int BarData::getMaximumValue() const noexcept
{
    return maximumValue;
}

int BarData::getValue() const noexcept
{
    return value;
}

LookAndFeelConstants::Colors BarData::getBarBackgroundColorId() const noexcept
{
    return barBackgroundColorId;
}

LookAndFeelConstants::Colors BarData::getBarColorId() const noexcept
{
    return barColorId;
}

bool BarData::hasCursor() const noexcept
{
    return withCursor;
}

bool BarData::isSelected() const noexcept
{
    return selected;
}

bool BarData::isHovered() const noexcept
{
    return hovered;
}

bool BarData::isWithinLoop() const noexcept
{
    return withinLoop;
}

bool BarData::isOutOfBounds() const noexcept
{
    return outOfBounds;
}

bool BarData::isGenerated() const noexcept
{
    return generated;
}

bool BarData::isIgnored() const noexcept
{
    return ignored;
}

bool BarData::isRetrig() const noexcept
{
    return retrig;
}

bool BarData::operator==(const BarData& rhs) const
{
    return shortBar == rhs.shortBar &&
           minimumValue == rhs.minimumValue &&
           maximumValue == rhs.maximumValue &&
           value == rhs.value &&
           barBackgroundColorId == rhs.barBackgroundColorId &&
           barColorId == rhs.barColorId &&
           withCursor == rhs.withCursor &&
           selected == rhs.selected &&
           hovered == rhs.hovered &&
           withinLoop == rhs.withinLoop &&
           outOfBounds == rhs.outOfBounds &&
           generated == rhs.generated &&
           ignored == rhs.ignored &&
           retrig == rhs.retrig
           ;
}

bool BarData::operator!=(const BarData& rhs) const
{
    return !(rhs == *this);
}

}   // namespace arkostracker

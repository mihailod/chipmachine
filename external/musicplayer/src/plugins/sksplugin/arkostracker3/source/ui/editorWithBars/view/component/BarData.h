#pragma once

#include "../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

/** Data about how to display a Bar. It is immutable. */
class BarData
{
public:
    /**
     * Constructor.
     * @param shortBar true to show only a small bar of the current value. False to show the complete bar from 0.
     * @param minimumValue the minimum value. May be negative.
     * @param maximumValue the minimum value.
     * @param value the value. May be out of bounds.
     * @param barBackgroundColorId the color ID of the bar.
     * @param barColorId the color ID of the bar.
     * @param selected true if selected.
     * @param hovered true if hovered by the mouse.
     * @param withinLoop true if inside the loop.
     * @param outOfBounds true beyond the end of the instrument.
     * @param generated true if a copy of an existing value (because auto-spread).
     * @param ignored true if this value is not used (because not used in this sound type for example).
     * @param retrig true if Retrig is shown.
     * @param withCursor true if a manual cursor is here.
     */
    BarData(bool shortBar, int minimumValue, int maximumValue, int value, LookAndFeelConstants::Colors barBackgroundColorId,
            LookAndFeelConstants::Colors barColorId, bool withCursor, bool selected, bool hovered, bool withinLoop, bool outOfBounds, bool generated,
            bool ignored, bool retrig) noexcept;

    /** Constructor for a non-visible bar. */
    BarData() noexcept;

    bool isShortBar() const noexcept;
    int getMinimumValue() const noexcept;
    int getMaximumValue() const noexcept;
    int getValue() const noexcept;
    LookAndFeelConstants::Colors getBarBackgroundColorId() const noexcept;
    LookAndFeelConstants::Colors getBarColorId() const noexcept;
    bool hasCursor() const noexcept;
    bool isSelected() const noexcept;
    bool isHovered() const noexcept;
    bool isWithinLoop() const noexcept;
    bool isOutOfBounds() const noexcept;
    bool isGenerated() const noexcept;
    bool isIgnored() const noexcept;
    bool isRetrig() const noexcept;

    bool operator==(const BarData& rhs) const;
    bool operator!=(const BarData& rhs) const;

private:
    bool shortBar;
    int minimumValue;
    int maximumValue;
    int value;
    LookAndFeelConstants::Colors barBackgroundColorId;
    LookAndFeelConstants::Colors barColorId;
    bool withCursor;
    bool selected;
    bool hovered;
    bool withinLoop;
    bool outOfBounds;
    bool generated;
    bool ignored;
    bool retrig;
};

}   // namespace arkostracker

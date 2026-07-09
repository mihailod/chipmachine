#pragma once

#include "../../../../utils/OptionalValue.h"

namespace arkostracker 
{

/** The data to display for the left header. */
class LeftHeaderDisplayData
{
public:
    /**
     * Constructor.
     * @param plusEnabled true if the Plus Button is enabled.
     * @param minusEnabled true if the Minus Button is enabled.
     * @param autoSpreadOn true if the AutoSpread Button is on. If not present, the button is not visible.
     * @param hidden true if hidden.
     */
    LeftHeaderDisplayData(bool plusEnabled, bool minusEnabled, OptionalBool autoSpreadOn, bool hidden) noexcept;

    /** Default constructor, useful for maps. */
    LeftHeaderDisplayData() noexcept;

    bool isPlusEnabled() const noexcept;
    bool isMinusEnabled() const noexcept;
    const OptionalBool& getAutoSpreadOn() const noexcept;
    bool isHidden() const noexcept;

    bool operator==(const LeftHeaderDisplayData& rhs) const noexcept;
    bool operator!=(const LeftHeaderDisplayData& rhs) const noexcept;

private:
    bool plusEnabled;
    bool minusEnabled;
    OptionalBool autoSpreadOn;
    bool hidden;
};

}   // namespace arkostracker

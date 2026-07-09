#pragma once

#include "../view/AreaType.h"

namespace arkostracker
{

/** Generic location of a cursor in a Bar editor. */
class BarEditorCursorLocation
{
public:
    /**
     * Constructor.
     * @param x the X (>=0).
     * @param areaType where area where the cursor is (the "Y").
     */
    BarEditorCursorLocation(int x, AreaType areaType) noexcept;

    /**
     * @return a copy of the object with a new X.
     * @param newX the new X.
     */
    BarEditorCursorLocation withX(int newX) const noexcept;

    /**
     * @return a copy of the object with a new area type.
     * @param newAreaType the new area type.
     */
    BarEditorCursorLocation withAreaType(AreaType newAreaType) const noexcept;

    int getX() const noexcept;
    AreaType getAreaType() const noexcept;

    bool operator==(const BarEditorCursorLocation& rhs) const;
    bool operator!=(const BarEditorCursorLocation& rhs) const;

private:
    int x;
    AreaType areaType;
};

}   // namespace arkostracker

#include "BarEditorCursorLocation.h"

namespace arkostracker
{

BarEditorCursorLocation::BarEditorCursorLocation(const int pX, const AreaType pAreaType) noexcept :
        x(pX),
        areaType(pAreaType)
{
}

int BarEditorCursorLocation::getX() const noexcept
{
    return x;
}

AreaType BarEditorCursorLocation::getAreaType() const noexcept
{
    return areaType;
}

BarEditorCursorLocation BarEditorCursorLocation::withX(int newX) const noexcept
{
    return { newX, areaType };
}

BarEditorCursorLocation BarEditorCursorLocation::withAreaType(AreaType newAreaType) const noexcept
{
    return { x, newAreaType };
}

bool BarEditorCursorLocation::operator==(const BarEditorCursorLocation& rhs) const
{
    return (x == rhs.x) &&
        (areaType == rhs.areaType);
}

bool BarEditorCursorLocation::operator!=(const BarEditorCursorLocation& rhs) const
{
    return !(rhs == *this);
}

}   // namespace arkostracker

#include "ArrangerView.h"

#include "../../utils/SimpleId.h"

namespace arkostracker 
{

ArrangerView::ArrangerView() noexcept :
    arrangerId(SimpleId::getNextIdentifier())
{
}

int ArrangerView::getArrangerId() const noexcept
{
    return arrangerId;
}

int ArrangerView::getCurrentSize(bool width) const noexcept
{
    return width ? getWidth() : getHeight();
}

int ArrangerView::getMaximumSize(bool width) const noexcept
{
    return width ? getArrangerViewWidth().getMaximum() : getArrangerViewHeight().getMaximum();
}

int ArrangerView::getMinimumSize(bool width) const noexcept
{
    return width ? getArrangerViewWidth().getMinimum() : getArrangerViewHeight().getMinimum();
}

void ArrangerView::addToSize(bool toWidth, int valueToAdd) noexcept
{
    if (toWidth) {
        setSize(getWidth() + valueToAdd, getHeight());
    } else {
        setSize(getWidth(), getHeight() + valueToAdd);
    }
}

void ArrangerView::addToPosition(bool toWidth, int valueToAdd) noexcept
{
    if (toWidth) {
        setTopLeftPosition(getX() + valueToAdd, getY());
    } else {
        setTopLeftPosition(getX(), getY() + valueToAdd);
    }
}

bool ArrangerView::isHiddenHeader() const
{
    return false;
}


}   // namespace arkostracker


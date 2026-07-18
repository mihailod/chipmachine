#include "Tab.h"

#include "../../../utils/SimpleId.h"

namespace arkostracker 
{

Tab::Tab() noexcept :
        tabId(SimpleId::getNextIdentifier()),
        selected(false)
{
}

int Tab::getArrangerId() const noexcept
{
    return tabId;
}

bool Tab::isSelected() const noexcept
{
    return selected;
}

// =================================

void Tab::setSelected(const bool newSelected) noexcept
{
    // Don't do anything if the tab was already in the same state.
    if (selected == newSelected) {
        return;
    }
    selected = newSelected;

    // Updates the UI.
    repaint();
}

}   // namespace arkostracker

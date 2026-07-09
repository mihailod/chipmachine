#include "VisibilityHandler.h"

#include "../EditorWithBarsControllerSpecific.h"

namespace arkostracker
{
VisibilityHandler::VisibilityHandler(EditorWithBarsControllerSpecific& pSpecificController) noexcept :
        specificController(pSpecificController),
        areaTypeToInitialSize(pSpecificController.getAreaTypesToInitialSize()),
        areaTypeToMaximumSize(pSpecificController.getAreaTypesToMaximumSize()),
        itemIdToAreaTypeToCurrentSize(),
        itemIdToHiddenAreaTypes()
{
}

BarAreaSize VisibilityHandler::getAreaSize(const OptionalId& optionalItemId, const AreaType areaType) const noexcept
{
    if (optionalItemId.isAbsent()) {
        jassertfalse;
        return BarAreaSize::normal;
    }
    const auto& itemId = optionalItemId.getValueRef();

    // Does the item exist? If not, uses a template.
    const auto it = itemIdToAreaTypeToCurrentSize.find(itemId);
    const auto& areaTypeToSize = (it == itemIdToAreaTypeToCurrentSize.cend()) ? areaTypeToInitialSize : it->second;

    // Gets the size, or "normal" if not found.
    const auto it2 = areaTypeToSize.find(areaType);
    return (it2 == areaTypeToSize.cend()) ? BarAreaSize::normal : it2->second;
}

BarAreaSize VisibilityHandler::getAreaMaximumSize(const AreaType areaType) const noexcept
{
    // Does the item exist? If not, uses a template.
    if (const auto it = areaTypeToMaximumSize.find(areaType); it != areaTypeToMaximumSize.cend()) {
        return it->second;
    }

    return BarAreaSize::allUnusual;
}

bool VisibilityHandler::isAreaHidden(const OptionalId& optionalItemId, const AreaType areaType) const noexcept
{
    if (optionalItemId.isAbsent()) {
        return false;
    }
    const auto& itemId = optionalItemId.getValueRef();

    // Does the item exist in the "item to hidden area map"?
    if (const auto it = itemIdToHiddenAreaTypes.find(itemId); it == itemIdToHiddenAreaTypes.cend()) {
        // Item not found. So it is not hidden.
        return false;
    } else {
        const auto& hiddenAreaTypes = it->second;
        return hiddenAreaTypes.find(areaType) != hiddenAreaTypes.cend();
    }
}

bool VisibilityHandler::isEnlargeAreaAllowed(const OptionalId& itemId, const AreaType areaType) const noexcept
{
    // Each Area has its own maximum size set.
    const auto currentSize = getAreaSize(itemId, areaType);
    const auto maximumSize = getAreaMaximumSize(areaType);

    jassert(currentSize <= maximumSize);        // Should never assert!
    return (currentSize < maximumSize);
}

void VisibilityHandler::setAreaSize(const OptionalId& optionalItemId, const AreaType areaType, const BarAreaSize barAreaSize) noexcept
{
    if (optionalItemId.isAbsent()) {
        jassertfalse;
        return;
    }
    const auto& itemId = optionalItemId.getValueRef();

    // Does the item exist?
    if (const auto it = itemIdToAreaTypeToCurrentSize.find(itemId); it == itemIdToAreaTypeToCurrentSize.cend()) {
        // Does not exist. Creates one with the template values.
        auto map = areaTypeToInitialSize;
        map[areaType] = barAreaSize;
        itemIdToAreaTypeToCurrentSize.insert({ itemId, map });
    } else {
        // It exists.
        it->second[areaType] = barAreaSize;
    }
}

void VisibilityHandler::setAreaHidden(const OptionalId& optionalItemId, const AreaType areaType, const bool hidden) noexcept
{
    if (optionalItemId.isAbsent()) {
        jassertfalse;
        return;
    }
    const auto& itemId = optionalItemId.getValueRef();

    // Does the item exist?
    if (const auto it = itemIdToHiddenAreaTypes.find(itemId); it == itemIdToHiddenAreaTypes.cend()) {
        // Does not exist. Creates an empty structure (so, all rows all visible).
        std::set<AreaType> map;
        if (hidden) {
            map.insert(areaType);
        }
        itemIdToHiddenAreaTypes.insert({ itemId, map });
    } else {
        // The ID entry already exists.
        auto& areaTypes = it->second;
        if (hidden) {
            // To hide, can add it without testing.
            areaTypes.insert(areaType);
        } else {
            // Removes, because not hidden.
            if (const auto it2 = areaTypes.find(areaType); it2 != areaTypes.cend()) {
                // Found, so removes it.
                areaTypes.erase(it2);
            }
        }
    }
}

bool VisibilityHandler::toggleHiddenAreaType(const OptionalId& optionalItemId, const AreaType areaType) noexcept
{
    if (optionalItemId.isAbsent()) {
        jassertfalse;
        return false;
    }
    const auto& itemId = optionalItemId.getValueRef();

    // Can hide the row? If not, don't try anything.
    if (!specificController.canHideRow(itemId, areaType)) {
        return false;
    }

    // Toggles.
    const auto hidden = isAreaHidden(optionalItemId, areaType);
    setAreaHidden(optionalItemId, areaType, !hidden);

    return true;
}

std::unordered_map<AreaType, BarAreaSize> VisibilityHandler::getCurrentSizes(const OptionalId& optionalItemId) const noexcept
{
    if (optionalItemId.isAbsent()) {
        return areaTypeToInitialSize;
    }
    const auto& itemId = optionalItemId.getValueRef();

    // Does the item exist? If not, uses a template.
    const auto it = itemIdToAreaTypeToCurrentSize.find(itemId);
    return (it == itemIdToAreaTypeToCurrentSize.cend()) ? areaTypeToInitialSize : it->second;
}

void VisibilityHandler::hideRows(const OptionalId& optionalItemId, const bool skipIfNeverDone, const bool forceShrinkFull) noexcept
{
    if (optionalItemId.isAbsent()) {
        return;     // Happens when switching to an expression and there are none (only the empty).
    }
    const auto& itemId = optionalItemId.getValueRef();

    // Don't do anything if never shrunk before, if wanted.
    if (skipIfNeverDone && (itemIdToHiddenAreaTypes.find(itemId) != itemIdToHiddenAreaTypes.cend())) {
        return;
    }

    for (const auto [areaType, _] : areaTypeToInitialSize) {
        const auto hidden = specificController.doesContainOnlyDefaultData(itemId, areaType);
        setAreaHidden(optionalItemId, areaType, hidden);        // Not very optimized, who cares.

        const auto barAreaSize = specificController.getMinimumSizeForData(itemId, areaType, forceShrinkFull);
        setAreaSize(optionalItemId, areaType, barAreaSize);
    }
}

}   // namespace arkostracker

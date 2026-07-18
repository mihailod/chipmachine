#pragma once

#include "../../../../utils/Id.h"
#include "../../../../utils/OptionalValue.h"
#include "../../view/AreaType.h"
#include "../../view/component/BarAreaSize.h"

namespace arkostracker
{

class EditorWithBarsControllerSpecific;

/** Helps handle the size and visibility of areas in editors with bars. */
class VisibilityHandler
{
public:
    /**
     * Constructor.
     * @param specificController controller to the bars.
     */
    explicit VisibilityHandler(EditorWithBarsControllerSpecific& specificController) noexcept;

    /**
     * @return the size of the given area. It should exist, else asserts and a default value is returned.
     * @param itemId the ID of the item. If not present, asserts and returns false.
     * @param areaType the area.
     */
    BarAreaSize getAreaSize(const OptionalId& itemId, AreaType areaType) const noexcept;

    /**
     * @return true if the area is hidden. If not found, considered true.
     * @param itemId the ID of the item. If not present, asserts and returns false.
     * @param areaType the area type to find.
     */
    bool isAreaHidden(const OptionalId& itemId, AreaType areaType) const noexcept;

    /**
     * @return true if the given AreaType can be enlarged.
     * @param itemId the ID of the item. If not present, asserts and returns false.
     * @param areaType the area type to find.
     */
    bool isEnlargeAreaAllowed(const OptionalId& itemId, AreaType areaType) const noexcept;

    /**
     * Sets the new size. Its validity is not checked.
     * @param itemId the ID of the item. If not present, asserts and does nothing.
     * @param areaType the area type to find. May not exist.
     * @param barAreaSize the new size.
     */
    void setAreaSize(const OptionalId& itemId, AreaType areaType, BarAreaSize barAreaSize) noexcept;

    /**
     * Sets the hidden flag.
     * @param itemId the ID of the item. If not present, asserts and does nothing.
     * @param areaType the area type to find. May not exist.
     * @param hidden true if hidden.
     */
    void setAreaHidden(const OptionalId& itemId, AreaType areaType, bool hidden) noexcept;

    /**
     * Toggles the hidden flag of an area type.
     * @param itemId the ID of the item. If not present, asserts and does nothing.
     * @param areaType the area type to find. May not exist.
     * @return true if the toggles could be performed.
     */
    bool toggleHiddenAreaType(const OptionalId& itemId, AreaType areaType) noexcept;

    /**
     * @return a map of the current sizes. It is always complete.
     * @param itemId the ID of the item. If not present, asserts but returns template sizes.
     */
    std::unordered_map<AreaType, BarAreaSize> getCurrentSizes(const OptionalId& itemId) const noexcept;

    /**
     * Hides all the rows according to their content ("useful" contents will not be hidden). Some rows cannot be shrunk.
     * @param itemId the ID of the item. If not present, asserts but returns template sizes.
     * @param skipIfNeverDone if true, nothing is done if the item has never been shrunk before. False in most case.
     * @param forceSmallestSize if true, some sizes may be smaller because smaller bars would be used. Most of the times, false is enough.
     */
    void hideRows(const OptionalId& itemId, bool skipIfNeverDone = false, bool forceSmallestSize = false) noexcept;

private:
    /**
     * @return the size of the given area. If does not exist, returns the maximum value.
     * @param areaType the area.
     */
    BarAreaSize getAreaMaximumSize(AreaType areaType) const noexcept;

    EditorWithBarsControllerSpecific& specificController;

    const std::unordered_map<AreaType, BarAreaSize> areaTypeToInitialSize;
    const std::unordered_map<AreaType, BarAreaSize> areaTypeToMaximumSize;

    /** The size (min, max, collapsed, etc.) for each Area, for each item. Absent sizes are considered "normal". */
    std::unordered_map<Id, std::unordered_map<AreaType, BarAreaSize>> itemIdToAreaTypeToCurrentSize;
    /** Stores what areas are hidden for each item. Absent items are considered as the template. */
    std::unordered_map<Id, std::set<AreaType>> itemIdToHiddenAreaTypes;
};

}   // namespace arkostracker

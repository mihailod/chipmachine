#pragma once

#include "../../../utils/Id.h"

namespace arkostracker 
{

class SongController;

/** Helper for actions about Subsongs. */
class SubsongActionHelper
{
public:
    /**
     * Notifies the listeners of a change in the Subsongs (ordering, count...), and shows the Subsong which index is given.
     * @param songController the Song Controller.
     * @param subsongIdToShow the id of the Subsong to show.
     */
    static void notifyAndShowSubsong(SongController& songController, const Id& subsongIdToShow) noexcept;
};

}   // namespace arkostracker

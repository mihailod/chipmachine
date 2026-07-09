#pragma once

#include <memory>

#include "../../../../song/instrument/psg/PsgInstrumentCell.h"
#include "../../../editorWithBars/view/AreaType.h"
#include "../../../../song/instrument/psg/PsgSection.h"

namespace arkostracker 
{

class SongController;

/** Holds some logic about the PSG Instrument Editor Controller. */
class PsgInstrumentEditorControllerLogic
{
public:
    /**
     * Checks the given value, and creates a new Cell if it is worth it.
     * @param areaType the AreaType of the value change.
     * @param barIndex the index where the change is.
     * @param value the desired value. It may be out of bounds (because of mouse wheel for example).
     * @param songController the Song Controller.
     * @param instrumentId the ID of the instrument.
     * @return the new Cell if worth it, else nullptr.
     */
    static std::unique_ptr<PsgInstrumentCell> checkValueAndChangeInstrumentCell(AreaType areaType, int barIndex, int value, const SongController& songController,
                                                                                const Id& instrumentId) noexcept;

    /** @return the PSG Section converted from the given AreaType. */
    static PsgSection convertAreaType(AreaType areaType) noexcept;
};

}   // namespace arkostracker

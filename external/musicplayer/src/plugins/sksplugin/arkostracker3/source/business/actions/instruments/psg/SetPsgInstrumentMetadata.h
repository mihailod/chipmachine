#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <unordered_map>

#include "../../../../song/instrument/psg/PsgPart.h"
#include "../../../../utils/OptionalValue.h"

namespace arkostracker
{

class SongController;

/** Sets the metadata (loop, retrig, speed, etc.) of a PSG Instrument. Does NOT concern the cells. */
class SetPsgInstrumentMetadata final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param id the instrument ID. It must exist and be a PSG instrument.
     * @param newLoopStart if present, the new loop start. Must be valid.
     * @param newEnd if present, the new end. Must be valid.
     * @param newIsLoop if present, the new loop state.
     * @param newIsRetrig if present, the new retrig state.
     * @param newSpeed if present, the new speed.
     * @param modifiedSectionToAutoSpreadLoop the possible modified auto-spread sections.
     */
    SetPsgInstrumentMetadata(SongController& songController, Id id, OptionalInt newLoopStart, OptionalInt newEnd, OptionalBool newIsLoop,
                             OptionalBool newIsRetrig, OptionalInt newSpeed, std::unordered_map<PsgSection, Loop> modifiedSectionToAutoSpreadLoop) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the listeners of a change in the Cell. */
    void notify() const noexcept;

    SongController& songController;
    const Id id;
    OptionalInt loopStart;
    OptionalInt end;
    OptionalBool loop;
    OptionalBool retrig;
    OptionalInt speed;
    std::unordered_map<PsgSection, Loop> modifiedSectionToAutoSpreadLoop;

    Loop oldLoop;
    bool oldRetrig;
    int oldSpeed;
    std::unordered_map<PsgSection, Loop> oldSectionToAutoSpreadLoop;
};

}   // namespace arkostracker

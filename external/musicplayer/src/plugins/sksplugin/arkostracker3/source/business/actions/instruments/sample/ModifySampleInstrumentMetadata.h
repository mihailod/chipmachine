#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../../utils/OptionalValue.h"
#include "../../../model/Loop.h"

namespace arkostracker
{

class SongController;

/** Modifies a sample metadata (loop, amplification). */
class ModifySampleInstrumentMetadata final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param id the instrument ID. It must exist and be a Sample instrument.
     * @param newLoopStart if present, the new loop start. Must be valid.
     * @param newEnd if present, the new end. Must be valid.
     * @param newIsLoop if present, the new loop state.
     * @param newAmplification if present, the new amplification.
     * @param newDigiNote if present, the new diginote.
     */
    ModifySampleInstrumentMetadata(SongController& songController, Id id, OptionalInt newLoopStart, OptionalInt newEnd, OptionalBool newIsLoop,
                                   OptionalFloat newAmplification, OptionalInt newDigiNote) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the listeners of a change. */
    void notify() const noexcept;

    SongController& songController;
    const Id id;

    OptionalInt loopStart;
    OptionalInt end;
    OptionalBool isLooping;
    OptionalFloat amplification;
    OptionalInt digiNote;

    Loop oldLoop;
    float oldAmplification;
    int oldDigiNote;
};

}   // namespace arkostracker

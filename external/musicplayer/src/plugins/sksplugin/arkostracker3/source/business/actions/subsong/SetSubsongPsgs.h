#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/subsong/Subsong.h"
#include "../../../utils/Id.h"
#include "../../model/Properties.h"

namespace arkostracker 
{

class SongController;

/** Action to change the PSGs and metadata. This will possibly change Positions if the Channel count changes! */
class SetSubsongPsgs final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param subsongId the id of the Subsong. Should be valid.
     * @param psgs the PSGs. Must be at least one.
     * @param metadata the metadata.
     */
    SetSubsongPsgs(SongController& songController, Id subsongId, std::vector<Psg> psgs, Properties metadata) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the change. */
    void notify() const noexcept;

    SongController& songController;
    const Id subsongId;
    const std::vector<Psg> newPsgs;
    std::vector<Psg> oldPsgs;
    const Properties newMetadata;
    std::unique_ptr<Properties> oldMetadata;
    std::unique_ptr<Subsong> oldSubsong;                // Used if the PSG count changes.
};

}   // namespace arkostracker

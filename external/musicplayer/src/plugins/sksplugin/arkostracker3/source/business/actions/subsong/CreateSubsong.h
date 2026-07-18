#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/psg/Psg.h"
#include "../../model/Properties.h"
#include "../../../utils/Id.h"

namespace arkostracker 
{

class SongController;

/** Action to create a new Subsong after the others. */
class CreateSubsong final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param metadata the metadata.
     * @param psgs the psgs.
     */
    CreateSubsong(SongController& songController, Properties metadata, std::vector<Psg> psgs) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /**
     * Notifies the listeners and shows the Subsong which id is given.
     * @param subsongIdToShow the id of the Subsong to show.
     */
    void notifyAndShowSubsong(const Id& subsongIdToShow) const noexcept;

    SongController& songController;
    const Properties metadata;
    const std::vector<Psg> psgs;

    Id newSubsongId;
    Id previousSubsongId;
};

}   // namespace arkostracker

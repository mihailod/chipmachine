#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../utils/Id.h"
#include "../../model/Properties.h"

namespace arkostracker 
{

class SongController;

/** Action to set the metadata of a Subsong (PSGs not included). */
class SetSubsongMetadata final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param subsongId the id of the Subsong. Should be valid.
     * @param metadata the metadata.
     */
    SetSubsongMetadata(SongController& songController, Id subsongId, Properties metadata) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the change. */
    void notify() const noexcept;

    SongController& songController;
    const Id subsongId;
    const Properties newMetadata;
    std::unique_ptr<Properties> oldMetadata;
};

}   // namespace arkostracker

#pragma once

#include <memory>

#include <juce_data_structures/juce_data_structures.h>

#include "../../../utils/Id.h"
#include "../../../song/subsong/Pattern.h"

namespace arkostracker 
{

class SongController;

/** Action to modify a Pattern (replace a Pattern with another one). */
class ModifyPattern final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the ID of the Subsong. Must be valid.
     * @param patternIndex the index of the Pattern to modify. Must be valid.
     * @param newPattern the Pattern to replace the old Pattern with.
     */
    ModifyPattern(SongController& songController, Id subsongId, int patternIndex, Pattern newPattern) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() const noexcept;

    SongController& songController;

    const Id subsongId;
    const int patternIndex;
    const Pattern newPattern;

    std::unique_ptr<Pattern> oldPattern;
};

}   // namespace arkostracker

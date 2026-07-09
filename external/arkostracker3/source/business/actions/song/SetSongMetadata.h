#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../utils/OptionalValue.h"

namespace arkostracker 
{

class SongController;

/** Sets the metadata of the Song. Nothing happens if they are all absent. */
class SetSongMetadata final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param newTitle the possible new title.
     * @param newAuthor the possible new author.
     * @param newComposer the possible new composer.
     * @param newComments the possible new comments.
     */
    SetSongMetadata(SongController& songController, const OptionalValue<juce::String>& newTitle, const OptionalValue<juce::String>& newAuthor,
                    const OptionalValue<juce::String>& newComposer, const OptionalValue<juce::String>& newComments) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Song Metadata listeners about a change in the song name. */
    void notifyForName() noexcept;

    SongController& songController;

    OptionalValue<juce::String> newTitle;
    OptionalValue<juce::String> newAuthor;
    OptionalValue<juce::String> newComposer;
    OptionalValue<juce::String> newComments;

    juce::String oldTitle;
    juce::String oldAuthor;
    juce::String oldComposer;
    juce::String oldComments;
};

}   // namespace arkostracker

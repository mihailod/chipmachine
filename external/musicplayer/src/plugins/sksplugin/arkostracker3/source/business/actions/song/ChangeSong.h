#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/Song.h"

namespace arkostracker
{

class SongController;

/**
 * Changes the current Song from the given one. Useful when an action changes a lot of things and it's simpler to change everything.
 * Useful for Merge.
 */
class ChangeSong final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param newSong the song to apply over the current one. It is copied.
     * @param subsongIdToGoTo where to go when after doing the action. On Undo, the first Subsong is used.
     */
    ChangeSong(SongController& songController, const Song& newSong, Id subsongIdToGoTo) noexcept;

    bool perform() override;
    bool undo() override;

private:
    void switchToSong(const Song& songToApply, const Id& subsongIdToGoTo) const noexcept;
    static void addExpressions(bool isArpeggio, Song& currentSong, const Song& songToApply) noexcept;

    SongController& songController;
    const Song newSong;
    Id subsongIdToGoToInNewSong;
    std::unique_ptr<Song> oldSong;
};

}   // namespace arkostracker

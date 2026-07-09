#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/Location.h"
#include "../../../song/cells/SpecialCell.h"
#include "../../../ui/patternViewer/controller/dataItem/Digit.h"

namespace arkostracker 
{

class SongController;

/** Action to set the data on a SpecialCell, mixing its content with the existing data. Useful when writing data from the PatternViewer. */
class SetSpecialCellData final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param location the location of the position.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param digit the digit to write.
     */
    SetSpecialCellData(SongController& songController, Location location, bool isSpeedTrack, const Digit& digit) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() noexcept;

    SongController& songController;

    const Location location;
    const bool isSpeedTrack;
    Digit digit;

    SpecialCell oldSpecialCell;
};



}   // namespace arkostracker


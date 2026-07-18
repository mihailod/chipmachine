#include "MovePositionMarker.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"

namespace arkostracker 
{

MovePositionMarker::MovePositionMarker(SongController& pSongController, Id pSubsongId, int pSourcePosition, int pDestinationPosition) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        sourcePosition(pSourcePosition),
        destinationPosition(pDestinationPosition),
        sourceOldName(),
        sourceOldColor(),

        destinationOldName(),
        destinationOldColor()
{
}

bool MovePositionMarker::perform()
{
    // Gets the previous data of both markers, useful for redo.
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        const auto& pairSource = subsong.getPositionMarker(sourcePosition);
        sourceOldName = pairSource.first;
        sourceOldColor = juce::Colour(pairSource.second);
        const auto& pairDestination = subsong.getPositionMarker(destinationPosition);
        destinationOldName = pairDestination.first;
        destinationOldColor = juce::Colour(pairDestination.second);

        // Sets the new data.
        subsong.setPositionMarkerData(sourcePosition, "", juce::Colours::blue.getARGB());       // Empty name, the color is irrelevant.
        subsong.setPositionMarkerData(destinationPosition, sourceOldName, sourceOldColor.getARGB());  // The destination marker uses the data of the source.
    });

    // Notifies.
    notifyListeners();

    return true;
}

bool MovePositionMarker::undo()
{
    // Puts the old data back.
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        subsong.setPositionMarkerData(sourcePosition, sourceOldName, sourceOldColor.getARGB());
        subsong.setPositionMarkerData(destinationPosition, destinationOldName, destinationOldColor.getARGB());
    });

    notifyListeners();

    return true;
}

void MovePositionMarker::notifyListeners() const noexcept
{
    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) noexcept {
        observer->onLinkerPositionsChanged(subsongId, { sourcePosition, destinationPosition }, LinkerObserver::What::marker);
    });
}

}   // namespace arkostracker

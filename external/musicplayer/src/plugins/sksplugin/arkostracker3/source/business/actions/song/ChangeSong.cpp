#include "ChangeSong.h"

#include "../../../controllers/MainController.h"
#include "../../../controllers/SongController.h"

namespace arkostracker
{

ChangeSong::ChangeSong(SongController& pSongController, const Song& pNewSong, Id pSubsongIdToGoTo) noexcept :
        songController(pSongController),
        newSong(pNewSong),
        subsongIdToGoToInNewSong(std::move(pSubsongIdToGoTo)),
        oldSong()
{
}

bool ChangeSong::perform()
{
    // Stores the current song.
    oldSong = std::make_unique<Song>(*songController.getSong());

    switchToSong(newSong, subsongIdToGoToInNewSong);

    return true;
}

bool ChangeSong::undo()
{
    switchToSong(*oldSong, oldSong->getFirstSubsongId());

    return true;
}

void ChangeSong::switchToSong(const Song& songToApply, const Id& subsongIdToGoTo) const noexcept
{
    // No lock on currentSong, because the inner functions would provoke a deadlock (no "no-lock" methods for now).
    // But this shouldn't be a problem, since only one action is performed at the time.
    const auto currentSong = songController.getSong();

    // Sets the Subsongs.
    // First, adds the new Subsongs.
    const auto subsongIdsToAdd = songToApply.getSubsongIds();
    const auto subsongIdsToRemove = currentSong->getSubsongIds();

    // Gets the Subsongs from the new Song into the current one.
    for (const auto& subsongId : subsongIdsToAdd) {
        songToApply.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) {
            auto subsongToAdd = std::make_unique<Subsong>(subsong);
            currentSong->addSubsong(std::move(subsongToAdd));
        });
    }
    // Then removes the old subsongs in the new Song.
    for (const auto& subsongId : subsongIdsToRemove) {
        currentSong->deleteSubsong(subsongId);
    }

    // Sets the Instruments.
    currentSong->performOnInstruments([&] (std::vector<std::unique_ptr<Instrument>>& instruments) {
        // Removes all the instruments, then adds the new ones, including the 0th (since there is a lock, no problem).
        instruments.clear();

        songToApply.performOnConstInstruments([&](const std::vector<std::unique_ptr<Instrument>>& readInstruments) {
            for (const auto& readInstrument : readInstruments) {
                instruments.emplace_back(std::make_unique<Instrument>(*readInstrument));
            }
        });
    });

    // Sets the Expressions.
    addExpressions(true, *currentSong, songToApply);
    addExpressions(false, *currentSong, songToApply);

    // Sets the name/composer/etc.
    currentSong->setAuthor(songToApply.getAuthor());
    currentSong->setComposer(songToApply.getComposer());
    currentSong->setComments(songToApply.getComments());
    currentSong->setTitle(songToApply.getTitle());
    currentSong->resetDatesMs(songToApply.getCreationDateMs(), songToApply.getModificationDateMs());

    // Goes to the Subsong.
    auto& mainController = songController.getMainController();
    mainController.switchToSubsong(subsongIdToGoTo);
    mainController.notifyAllChanges(subsongIdToGoTo);
    mainController.stopPlaying(false);
}

void ChangeSong::addExpressions(const bool isArpeggio, Song& currentSong, const Song& songToApply) noexcept
{
    auto& expressionHandler = currentSong.getExpressionHandler(isArpeggio);
    expressionHandler.performOnExpressions([&] (std::vector<std::unique_ptr<Expression>>& expressions) noexcept {
        // Removes all the expressions, then adds the new ones, including the 0th (since there is a lock, no problem).
        expressions.clear();

        songToApply.getConstExpressionHandler(isArpeggio).performOnConstExpressions([&] (const std::vector<std::unique_ptr<Expression>>& readExpressions) noexcept {
            for (const auto& readExpression : readExpressions) {
                expressionHandler.addExpression(*readExpression);
            }
        });
    });
}

}   // namespace arkostracker

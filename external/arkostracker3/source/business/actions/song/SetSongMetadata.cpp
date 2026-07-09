#include "SetSongMetadata.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/SongMetadataObserver.h"

namespace arkostracker 
{

SetSongMetadata::SetSongMetadata(SongController& pSongController, const OptionalValue<juce::String>& pNewTitle, const OptionalValue<juce::String>& pNewAuthor,
                                 const OptionalValue<juce::String>& pNewComposer, const OptionalValue<juce::String>& pNewComments) noexcept :
        songController(pSongController),
        newTitle(pNewTitle),
        newAuthor(pNewAuthor),
        newComposer(pNewComposer),
        newComments(pNewComments)
{
}

bool SetSongMetadata::perform()
{
    // If no change, don't do anything.
    if (newTitle.isAbsent() && newAuthor.isAbsent() && newComposer.isAbsent() && newComments.isAbsent()) {
        return false;
    }

    auto song = songController.getSong();

    // Gets the old data. No transaction... Shouldn't be a problem for such data.
    oldTitle = songController.getTitle();
    oldAuthor = songController.getAuthor();
    oldComposer = songController.getComposer();
    oldComments = songController.getComments();

    if (newAuthor.isPresent()) {
        song->setAuthor(newAuthor.getValue());
    }
    if (newComposer.isPresent()) {
        song->setComposer(newComposer.getValue());
    }
    if (newComments.isPresent()) {
        song->setComments(newComments.getValue());
    }
    if (newTitle.isPresent()) {
        song->setTitle(newTitle.getValue());

        notifyForName();
    }

    return true;
}

bool SetSongMetadata::undo()
{
    // Only changes the values that has been changed before.
    auto song = songController.getSong();

    if (newAuthor.isPresent()) {
        song->setAuthor(oldAuthor);
    }
    if (newComposer.isPresent()) {
        song->setComposer(oldComposer);
    }
    if (newComments.isPresent()) {
        song->setComments(oldComments);
    }
    if (newTitle.isPresent()) {
        song->setTitle(oldTitle);

        notifyForName();
    }

    return true;
}

void SetSongMetadata::notifyForName() noexcept
{
    songController.getSongMetadataObservers().applyOnObservers([](SongMetadataObserver* observer) noexcept {
        observer->onSongMetadataChanged(SongMetadataObserver::What::name);
    });
}


}   // namespace arkostracker


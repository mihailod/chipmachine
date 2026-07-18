#include "MergeSong.h"

#include "../../../../business/actions/song/ChangeSong.h"
#include "../../../../controllers/MainController.h"
#include "../../../../business/song/tool/MergeSongProcess.h"

namespace arkostracker
{

MergeSong::MergeSong(MainController& pMainController) noexcept :
        mainController(pMainController),
        modalDialog(),
        loadSong()
{
}

void MergeSong::merge()
{
    loadSong = std::make_unique<LoadSong>(mainController, *this, false);
    loadSong->startLoadSongProcess(juce::String());
}

void MergeSong::onLoadedSongFailure()
{
    // Nothing to do.
}

void MergeSong::onLoadedSongSuccess(const std::unique_ptr<Song> songToMerge)
{
    merge(*songToMerge);
}

void MergeSong::merge(const Song& songToMerge) noexcept
{
    auto& songController = mainController.getSongController();
    const auto currentSong = songController.getSong();
    const auto initialSubsongCount = currentSong->getSubsongCount();

    // Performs the merge of both songs.
    const auto newSong = MergeSongProcess::merge(*currentSong, songToMerge);    // To improve: Async? Oh well...
    // It seems there are too many items (instruments, expressions...).
    if (newSong == nullptr) {
        modalDialog = SuccessOrErrorDialog::buildForError(juce::translate("Merging failed: too many instruments, arpeggios or pitches."),
                                                          [&] {
                                                              modalDialog.reset();
                                                          });
        return;
    }

    const auto firstNewSubsongId = newSong->getSubsongIds().at(static_cast<size_t>(initialSubsongCount));

    auto action = std::make_unique<ChangeSong>(songController, *newSong, firstNewSubsongId);
    songController.performAction(std::move(action), "Merge");
}

}   // namespace arkostracker

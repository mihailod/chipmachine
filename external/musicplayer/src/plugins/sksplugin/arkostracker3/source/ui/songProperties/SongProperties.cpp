#include "SongProperties.h"

#include "../../controllers/MainController.h"

namespace arkostracker 
{

SongProperties::SongProperties(MainController& pMainController) noexcept :
        mainController(pMainController),
        songPropertiesDialog()
{
}

void SongProperties::openSongProperties() noexcept
{
    jassert(songPropertiesDialog == nullptr);       // Already shown?
    songPropertiesDialog = std::make_unique<SongPropertiesDialog>(mainController,
                                                                  [&](SongPropertiesDialog::Result result) { onResult(std::move(result)); },
                                                                  [&] { songPropertiesDialog.reset(); });
}

void SongProperties::onResult(SongPropertiesDialog::Result result) noexcept // NOLINT(performance-unnecessary-value-param)  // By value to delete the dialog safely.
{
    songPropertiesDialog.reset();

    mainController.getSongController().setSongMetadata(result.newTitle, result.newAuthor, result.newComposer, result.newComments);
}


}   // namespace arkostracker


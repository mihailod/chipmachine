#pragma once

#include <memory>

#include "../components/dialogs/ModalDialog.h"
#include "SongPropertiesDialog.h"

namespace arkostracker 
{

class MainController;

/** Handles the UI and process of showing the Song Properties, but also the Subsongs. */
class SongProperties
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     */
    explicit SongProperties(MainController& mainController) noexcept;

    /** Opens the Song Properties panel. */
    void openSongProperties() noexcept;

private:
    /**
     * Called when the Song Properties returns a result.
     * @param result the result, containing the changes (if any).
     */
    void onResult(SongPropertiesDialog::Result result) noexcept;

    MainController& mainController;

    std::unique_ptr<ModalDialog> songPropertiesDialog;
};


}   // namespace arkostracker


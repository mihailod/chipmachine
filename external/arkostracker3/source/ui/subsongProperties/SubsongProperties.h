#pragma once

#include <memory>

#include "../../business/model/Properties.h"
#include "../../utils/Id.h"
#include "../components/dialogs/ModalDialog.h"
#include "SubsongPropertiesDialog.h"

namespace arkostracker 
{

class MainController;

/** Handles the UI and process of showing the Subsong Properties, but also the Subsongs. */
class SubsongProperties final : public SubsongPropertiesDialog::Listener
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     */
    explicit SubsongProperties(MainController& mainController) noexcept;

    /**
     * Opens the Subsong Properties panel.
     * @param subsongId the subsong id. Must be valid.
     */
    void openSubsongProperties(const Id& subsongId) noexcept;

    /** Opens the Add New Subsong panel. */
    void openAddNewSubsongPanel() noexcept;


    // SubsongPropertiesDialog::Listener method implementations.
    // ============================================================
    void onSubsongPropertiesDialogOk(std::vector<Psg> newPsgs, Properties newMetadata) noexcept override;
    void onSubsongPropertiesDialogCanceled() noexcept override;
    void onSubsongPropertiesDialogDeleteWanted() noexcept override;

private:
    /**
     * Creates a new Subsong.
     * @param psgs the PSGs.
     * @param metadata the metadata.
     */
    void createNewSubsong(const std::vector<Psg>& psgs, const Properties& metadata) const noexcept;

    /**
     * Modifies the Subsong (both PSGs and metadata). This may delete the tracks!
     * @param psgs the PSGs.
     * @param metadata the metadata.
     */
    void modifySubsong(const std::vector<Psg>& psgs, const Properties& metadata) const noexcept;

    /**
     * Modifies the Subsong (metadata only).
     * @param metadata the metadata.
     */
    void modifySubsongMetadata(const Properties& metadata) const noexcept;

    MainController& mainController;
    std::unique_ptr<ModalDialog> dialog;

    OptionalValue<Id> editedSubsongId;             // The edited Subsong index, or empty if Add New Subsong.
};

}   // namespace arkostracker

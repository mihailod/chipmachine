#include "SubsongProperties.h"

#include "../../controllers/MainController.h"

namespace arkostracker 
{

SubsongProperties::SubsongProperties(MainController& pMainController) noexcept :
        mainController(pMainController),
        dialog(),
        editedSubsongId()
{
}

void SubsongProperties::openSubsongProperties(const Id& subsongId) noexcept
{
    jassert(dialog == nullptr);

    editedSubsongId = subsongId;

    // Gets the Subsong metadata, to transmit it to the Dialog.
    std::unique_ptr<Subsong::Metadata> metadata;
    std::vector<Psg> psgs;
    const auto& songController = mainController.getSongController();
    songController.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        metadata = std::make_unique<Subsong::Metadata>(subsong.getMetadata());
        psgs = subsong.getPsgs();
    });
    if ((metadata == nullptr) || psgs.empty()) {
        jassertfalse;           // Should never happen!
        return;
    }
    const auto subsongCount = songController.getSubsongCount();
    const auto canDelete = (subsongCount > 1);      // Cannot delete if only one Subsong remaining.

    dialog = std::make_unique<SubsongPropertiesDialog>(subsongId, *metadata, psgs, *this, canDelete);
}

void SubsongProperties::openAddNewSubsongPanel() noexcept
{
    jassert(dialog == nullptr);
    editedSubsongId = OptionalValue<Id>();

    // Builds default metadata for the new Subsong.
    const auto metadata = Subsong::Metadata::buildDefault();
    const std::vector psgs = { Psg() };
    dialog = std::make_unique<SubsongPropertiesDialog>(editedSubsongId, metadata, psgs, *this, false);
}


// SubsongPropertiesDialog::Listener method implementations.
// ============================================================

void SubsongProperties::onSubsongPropertiesDialogOk(const std::vector<Psg> newPsgs, const Properties newMetadata) noexcept
{
    dialog.reset();

    if (editedSubsongId.isAbsent()) {
        createNewSubsong(newPsgs, newMetadata);
        return;
    }

    // Modification of a Subsong. But has there been any change?
    const auto subsongIndex = editedSubsongId.getValue();
    std::unique_ptr<Properties> metadata;
    std::vector<Psg> psgs;
    mainController.getSongController().performOnConstSubsong(subsongIndex, [&](const Subsong& subsong) {
        metadata = std::make_unique<Properties>(subsong.getMetadata());
        psgs = subsong.getPsgs();
    });
    if ((metadata == nullptr) || psgs.empty()) {
        jassertfalse;           // Should never happen!
        return;
    }

    const auto sameMetadata = (newMetadata == *metadata);
    const auto samePsgs = (psgs == newPsgs);

    if (sameMetadata && samePsgs) {
        return;             // Nothing to do.
    }
    if (!samePsgs) {
        modifySubsong(newPsgs, newMetadata);
    } else /*if (!sameMetadata)*/ {
        modifySubsongMetadata(newMetadata);
    }
}

void SubsongProperties::onSubsongPropertiesDialogCanceled() noexcept
{
    dialog.reset();
}

void SubsongProperties::onSubsongPropertiesDialogDeleteWanted() noexcept
{
    dialog.reset();

    jassert(editedSubsongId.isPresent());                // Must never happen!
    mainController.getSongController().deleteSubsong(editedSubsongId.getValue());
}


// ================================================

void SubsongProperties::createNewSubsong(const std::vector<Psg>& psgs, const Properties& metadata) const noexcept
{
    mainController.getSongController().createNewSubsong(psgs, metadata);
}

void SubsongProperties::modifySubsong(const std::vector<Psg>& psgs, const Properties& metadata) const noexcept
{
    jassert(editedSubsongId.isPresent());                // Must never happen!
    mainController.getSongController().changeSubsongPsgAndMetadata(editedSubsongId.getValue(), psgs, metadata);
}

void SubsongProperties::modifySubsongMetadata(const Properties& metadata) const noexcept
{
    jassert(editedSubsongId.isPresent());                // Must never happen!
    mainController.getSongController().changeSubsongMetadata(editedSubsongId.getValue(), metadata);
}

}   // namespace arkostracker

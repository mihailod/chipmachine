#include "ErrorCheckerDialog.h"

#include "../../../business/song/tool/errorChecker/ErrorChecker.h"
#include "../../../controllers/MainController.h"
#include "../../components/BlankComponent.h"
#include "../../patternViewer/controller/PatternViewerController.h"

namespace arkostracker
{
ErrorCheckerDialog::ErrorCheckerDialog(MainController& mMainController, std::function<void()> pListener) noexcept :
        ModalDialog(juce::translate("Error checker"), 460, 480,
                    [&] { onOkButtonClicked(); },
                    [&] { /* Never used. */ }),
        mainController(mMainController),
        modalCallback(std::move(pListener)),
        errorGroup(juce::String()),
        shownItems()
{
    const auto bounds = getUsableModalDialogBounds();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto x = bounds.getX();

    errorGroup.setBounds(x, top, width, 400);

    addComponentToModalDialog(errorGroup);

    fillErrorGroup();
}

void ErrorCheckerDialog::closeButtonPressed()
{
    modalCallback();
}

void ErrorCheckerDialog::onOkButtonClicked() const noexcept
{
    modalCallback();
}

void ErrorCheckerDialog::fillErrorGroup() noexcept
{
    shownItems.clear();

    // Gets the errors.
    const auto& song = mainController.getSongController().getSong();
    const auto foundErrors = ErrorChecker::findErrors(*song);

    const auto errorCount = foundErrors.size();
    errorGroup.setGroupTitle(errorCount > 0 ? (juce::translate("Error found: ") + juce::String(errorCount)) : "No error found.");

    if (errorCount <= 0) {
        return;
    }

    const auto subsongIds = song->getSubsongIds();

    // Displays the errors.
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;
    const auto width = errorGroup.getGroupInnerArea().getWidth() - static_cast<int>(margins * 3.5);
    const auto locationX = margins * 2;
    auto y = margins;

    for (const auto& [errorLocation, displayableError] : foundErrors) {
        constexpr auto separatorHeight = 1;
        constexpr auto goButtonWidth = 60;
        constexpr auto x = 0;

        const auto subsongId = errorLocation.getSubsongId();
        const auto subsongIndexOptional = song->getSubsongIndex(subsongId);
        const auto subsongIndex = subsongIndexOptional.isPresent() ? subsongIndexOptional.getValue() : 0;
        jassert(subsongIndexOptional.isPresent());      // Should never happen!

        const auto locationText = juce::String("Subsong ") + juce::String(subsongIndex + 1) +
            juce::String(", position: ") + juce::String::toHexString(errorLocation.getPositionIndex()) +
            juce::String(", line: ") + juce::String::toHexString(errorLocation.getLineIndex()) +
            juce::String(", channel: ") + juce::String(errorLocation.getChannelIndex() + 1);
        auto locationLabel = std::make_unique<juce::Label>(juce::String(), locationText);
        auto errorLabel = std::make_unique<juce::Label>(juce::String(), displayableError);
        auto gotoButton = std::make_unique<juce::TextButton>(juce::translate("Go"));
        auto separator = std::make_unique<BlankComponent>(static_cast<int>(LookAndFeelConstants::Colors::dialogBorder));

        errorLabel->setBounds(x, y, width, labelsHeight);
        y += labelsHeight;

        gotoButton->setBounds(width - goButtonWidth, y, goButtonWidth, labelsHeight);
        locationLabel->setBounds(locationX, gotoButton->getY(), width - goButtonWidth - locationX - margins, labelsHeight);
        y += labelsHeight + margins;

        separator->setBounds(x, y, width, separatorHeight);
        y += separatorHeight + margins;

        errorGroup.addComponentToGroup(*locationLabel);
        errorGroup.addComponentToGroup(*errorLabel);
        errorGroup.addComponentToGroup(*gotoButton);
        errorGroup.addComponentToGroup(*separator);

        gotoButton->addListener(this);

        shownItems.emplace_back(errorLocation, std::move(locationLabel), std::move(errorLabel), std::move(gotoButton), std::move(separator));
    }
}


// juce::Button::Listener method implementation
// =================================================

void ErrorCheckerDialog::buttonClicked(juce::Button* button)
{
    // Browses through the items.
    for (const auto& item : shownItems) {
        if (&item.getGotoButton() == button) {
            gotoLocation(item.getCellLocation());
            return;
        }
    }

    jassertfalse;       // Not found? Abnormal.
}


// =================================================

void ErrorCheckerDialog::gotoLocation(const CellLocationInPosition& location) const noexcept
{
    // Hack, if not present, the position doesn't change between a goto between two Subsongs (even if unlikely to be done...).
    mainController.switchToSubsong(location.getSubsongId());

    mainController.getSongController().setCurrentLocation(
        Location(location.getSubsongId(), location.getPositionIndex(), location.getLineIndex()),
        true
    );
    mainController.getPatternViewerControllerInstance().onUserWantsToGoToChannel(location.getChannelIndex());

    modalCallback();
}

}   // namespace arkostracker

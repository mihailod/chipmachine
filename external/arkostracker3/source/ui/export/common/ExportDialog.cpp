#include "ExportDialog.h"

#include "../../../business/song/tool/errorChecker/ErrorChecker.h"
#include "../../components/UiUtil.h"
#include "BinaryData.h"

namespace arkostracker
{

ExportDialog::ExportDialog(const MainController& pMainController, std::function<void()> pListener,
    const juce::String& title, const int width, const int height,
    const bool showOkButton, const bool showCancelButton, const bool resizable) noexcept :
        ModalDialog(title, width, height,
                            [&] { onExportButtonClicked(); },
                            [&] { onCancelButtonClicked(); },
                            showOkButton, showCancelButton, resizable),
        mainController(pMainController),
        songController(pMainController.getSongController()),
        listener(std::move(pListener)),
        warningImage(BinaryData::IconWarning_png, BinaryData::IconWarning_pngSize, static_cast<int>(LookAndFeelConstants::Colors::patternViewerError), 1.0F),
        warningBubble()
{
    setOkButtonText(juce::translate("Export"));
    setOkButtonWidth(70);
    setCancelButtonText(juce::translate("Close"));

    const auto margins = LookAndFeelConstants::margins;

    // Checks for error in the Song, shows a warning image if there are.
    if (const auto errors = ErrorChecker::findErrors(*pMainController.getSongController().getConstSong()); !errors.empty()) {
        const auto warningImageWidth = warningImage.getImageWidth();

        warningImage.setBounds(getOkButtonsX() - warningImageWidth - margins, getButtonsY(), warningImageWidth,
            std::max(warningImage.getImageHeight(), getButtonsHeight()));
        addComponentToModalDialog(warningImage);

        warningImage.onClick = [&] (bool) {
            warningBubble = UiUtil::createBubble(warningImage,
                juce::translate("There are errors in your song.\nTools > Check for errors."), true);
        };
    }
}

}   // namespace arkostracker

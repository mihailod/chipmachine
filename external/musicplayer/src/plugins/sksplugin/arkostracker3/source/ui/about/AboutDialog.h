#pragma once

#include "../components/dialogs/ModalDialog.h"

namespace arkostracker
{

/** Shows the credits. */
class AboutDialog : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param exitCallback called when the exiting the dialog.
     */
    explicit AboutDialog(const std::function<void()>& exitCallback) noexcept;

private:
    juce::ImageComponent logoImage;

    juce::Label welcomeLabel;
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label versionLabel;

    juce::Label mainText;
};

}   // namespace arkostracker

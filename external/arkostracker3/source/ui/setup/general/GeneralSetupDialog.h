#pragma once

#include <functional>

#include "../../components/dialogs/ModalDialog.h"

namespace arkostracker 
{

/** The General page of the setup. */
class GeneralSetupDialog final : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param exitCallback called when the exiting the dialog.
     */
    explicit GeneralSetupDialog(const std::function<void()>& exitCallback) noexcept;

private:
    /** Called to validate the changes and call the callback. */
    void validate() const noexcept;

    std::function<void()> exitCallback;

    juce::ToggleButton toggleShowSongInfoAfterLoad;
    juce::ToggleButton toggleUseNativeTitleBar;
    juce::ToggleButton toggleUseOpenGl;
    juce::ToggleButton toggleCompressXml;
    juce::ToggleButton toggleShowSplash;
    juce::ToggleButton toggleForceNonNativeFileChooserWhenLoadInstrument;
};

}   // namespace arkostracker

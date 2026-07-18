#pragma once

#include <functional>

#include "ModalDialog.h"

namespace arkostracker 
{

/** Dialog shown when the song is going to be erased, but its changes haven't been saved. */
class SongNotSavedDialog final : public ModalDialog
{
public:
    /** Constructor. */
    SongNotSavedDialog(const std::function<void()>& onSaveClicked, const std::function<void()>& onDontSaveClicked, const std::function<void()>& onCancelClicked) noexcept;

private:
    juce::Label textLabel;
    juce::TextButton dontSaveButton;
};

}   // namespace arkostracker

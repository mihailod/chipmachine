#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../utils/Id.h"

namespace arkostracker 
{

class MainController;

/** A reusable view showing a label and a ComboBox with the Subsong to choose. */
class SubsongChooser final : public juce::Component
{
public:
    static const int desiredHeight;                 // The height the client should be giving this Component.

    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param onSubsongChanged called when a Subsong changed. Are given the Subsong name and ID. May be nullptr.
     */
    SubsongChooser(const MainController& mainController, std::function<void(const std::pair<juce::String, Id>&)> onSubsongChanged) noexcept;

    /** @return the index of the selected Subsong. */
    int getSelectedSubsongIndex() const noexcept;

    /** @return the ID of the selected Subsong. */
    Id getSelectedSubsongId() const noexcept;

    // Component method implementations.
    // ====================================
    void resized() override;

private:
    std::function<void(const std::pair<juce::String, Id>&)> onSubsongChanged;

    juce::Label label;
    juce::ComboBox comboBox;

    std::vector<std::pair<juce::String, Id>> subsongNamesAndIds;
};

}   // namespace arkostracker

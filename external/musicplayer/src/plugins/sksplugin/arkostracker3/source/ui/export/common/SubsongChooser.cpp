#include "SubsongChooser.h"

#include "../../../controllers/MainController.h"

namespace arkostracker 
{

const int SubsongChooser::desiredHeight = 50;

SubsongChooser::SubsongChooser(const MainController& pMainController, std::function<void(const std::pair<juce::String, Id>&)> pOnSubsongChanged) noexcept :
        onSubsongChanged(std::move(pOnSubsongChanged)),
        label(juce::String(), juce::translate("Select the subsong to export:")),
        comboBox(),
        subsongNamesAndIds(pMainController.getSongController().getSong()->getSubsongNamesAndIds())
{
    jassert(!subsongNamesAndIds.empty());           // Going to crash!

    comboBox.onChange = [&] {
        // Notifies the listener.
        const auto selectedIndex = comboBox.getSelectedItemIndex();
        const auto& [name, id] = subsongNamesAndIds.at(static_cast<size_t>(selectedIndex));
        if (onSubsongChanged != nullptr) {
            onSubsongChanged({ name, id });
        }
    };

    // Fills the ComboBox.
    auto index = 1;
    for (const auto&[name, _] : subsongNamesAndIds) {

        comboBox.addItem(juce::String(index) + ": " + name, index);
        ++index;
    }
    comboBox.setSelectedItemIndex(0, juce::NotificationType::sendNotification);

    addAndMakeVisible(label);
    addAndMakeVisible(comboBox);
}


// Component method implementations.
// ====================================

void SubsongChooser::resized()
{
    const auto width = getWidth();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    label.setBounds(0, 0, width, labelsHeight);

    constexpr auto margins = 3;         // A small margin to match the text looks better.
    comboBox.setBounds(margins, label.getBottom(), width - 2 * margins, labelsHeight);
}


// ====================================

int SubsongChooser::getSelectedSubsongIndex() const noexcept
{
    return comboBox.getSelectedItemIndex();
}

Id SubsongChooser::getSelectedSubsongId() const noexcept
{
    const auto index = comboBox.getSelectedItemIndex();
    jassert(index < static_cast<int>(subsongNamesAndIds.size()));

    return subsongNamesAndIds.at(static_cast<size_t>(index)).second;
}

}   // namespace arkostracker

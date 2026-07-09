#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker
{

class ComboBoxItem
{
public:
    /**
     * Constructor.
     * @param text the text to display.
     * @param id the id. Must be >0 (Juce limitation).
     */
    ComboBoxItem(juce::String text, int id) noexcept;

    /**
     * Constructor with an index displayed in hexadecimal, followed by a ": ".
     * @param index an index (>=0).
     * @param text the text to display.
     * @param id the id. Must be >0 (Juce limitation).
     */
    ComboBoxItem(int index, const juce::String& text, int id) noexcept;

    /** @return the text to display. */
    juce::String getText() const noexcept;
    /** @return the id (!=0). */
    int getId() const noexcept;

    /** Adds the given ComboBoxItems to the given ComboBox. The ComboBox content is not cleared first. */
    static void addItemsToComboBox(juce::ComboBox& comboBox, const std::vector<ComboBoxItem>& items) noexcept;

    /** Adds the given ComboBoxItem to the given ComboBox. The ComboBox content is not cleared first. */
    static void addItemToComboBox(juce::ComboBox& comboBox, const ComboBoxItem& item) noexcept;

private:
    juce::String text;              // The text to display.
    int id;                         // The id (!=0).
};

}   // namespace arkostracker

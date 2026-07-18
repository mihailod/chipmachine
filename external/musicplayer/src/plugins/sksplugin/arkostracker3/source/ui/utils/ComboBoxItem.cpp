#include "ComboBoxItem.h"

#include "../../utils/NumberUtil.h"

namespace arkostracker
{

ComboBoxItem::ComboBoxItem(juce::String pText, int pId) noexcept :
        text(std::move(pText)),
        id(pId)
{
    jassert(pId != 0);
}

ComboBoxItem::ComboBoxItem(int pIndex, const juce::String& pOriginalText, int pId) noexcept :
        text(juce::String()),
        id(pId)
{
    jassert(pId != 0);
    jassert(pIndex >= 0);

    // Adds the index before the text, with a separator.
    text = NumberUtil::toHexByte(pIndex) + ": " + pOriginalText;
}

juce::String ComboBoxItem::getText() const noexcept
{
    return text;
}

int ComboBoxItem::getId() const noexcept
{
    return id;
}

void ComboBoxItem::addItemsToComboBox(juce::ComboBox& comboBox, const std::vector<ComboBoxItem>& items) noexcept
{
    for (const auto& item : items) {
        addItemToComboBox(comboBox, item);
    }
}

void ComboBoxItem::addItemToComboBox(juce::ComboBox& comboBox, const ComboBoxItem& item) noexcept
{
    comboBox.addItem(item.getText(), item.getId());
}

}   // namespace arkostracker

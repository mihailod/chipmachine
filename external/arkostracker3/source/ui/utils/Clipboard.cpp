#include "Clipboard.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

void Clipboard::clear() noexcept
{
    juce::SystemClipboard::copyTextToClipboard(juce::String());
}

void Clipboard::storeXmlNode(const juce::XmlElement* xmlElement) noexcept
{
    // Nothing to copy? Then clears the clipboard.
    if (xmlElement == nullptr) {
        juce::SystemClipboard::copyTextToClipboard(juce::String());
        return;
    }

    juce::MemoryOutputStream outputStream;
    xmlElement->writeTo(outputStream);
    juce::SystemClipboard::copyTextToClipboard(outputStream.toString());
}

std::unique_ptr<juce::XmlElement> Clipboard::retrieveXmlNode() noexcept
{
    const auto originalText = juce::SystemClipboard::getTextFromClipboard();

    return juce::XmlDocument::parse(originalText);
}

}   // namespace arkostracker

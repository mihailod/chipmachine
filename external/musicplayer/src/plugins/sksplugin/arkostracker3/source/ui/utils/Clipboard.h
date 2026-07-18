#pragma once

#include <memory>

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** A helper object to manage the clipboard. */
class Clipboard
{
public:
    /** Prevents instantiation. */
    Clipboard() = delete;

    /** Clears the clipboard. */
    static void clear() noexcept;

    /**
     * Stores an XML element into the clipboard.
     * @param xmlElement the element to store. If nullptr, the clipboard is cleared.
     */
    static void storeXmlNode(const juce::XmlElement* xmlElement) noexcept;

    /**
     * Retrieves an XML element from the clipboard.
     * @return the XML element, or nullptr if nothing could be extracted.
     */
    static std::unique_ptr<juce::XmlElement> retrieveXmlNode() noexcept;
};

}   // namespace arkostracker

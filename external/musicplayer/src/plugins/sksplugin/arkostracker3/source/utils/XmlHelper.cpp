#include "XmlHelper.h"

#include "MemoryBlockUtil.h"
#include "StringUtil.h"

namespace arkostracker 
{

juce::XmlElement& XmlHelper::addNode(juce::XmlElement& parent, const juce::String& nodeName) noexcept
{
    // Creates the new child node, adds it to the parent.
    auto* childXmlElement = new juce::XmlElement(nodeName);       // Ownership to the parent.
    parent.addChildElement(childXmlElement);

    return *childXmlElement;
}

std::vector<juce::XmlElement*> XmlHelper::getChildrenList(const juce::XmlElement* xmlElement, const juce::String& nodeNames) noexcept
{
    if (xmlElement == nullptr) {
        return { };
    }

    return getChildrenList(*xmlElement, nodeNames);
}

std::vector<juce::XmlElement*> XmlHelper::getChildrenList(const juce::XmlElement& xmlElement, const juce::String& nodeNames) noexcept
{
    std::vector<juce::XmlElement*> elements;

    // Gets a first child.
    auto* foundElement = xmlElement.getChildByName(nodeNames);
    while ((foundElement != nullptr) && (foundElement->getTagName() == nodeNames)) {        // The comparison is a security for the next iterations.
        elements.push_back(foundElement);

        // Finds the next one.
        foundElement = foundElement->getNextElementWithTagName(nodeNames);
    }

    return elements;
}

bool XmlHelper::readBool(const juce::XmlElement& xmlElement, const juce::String& nodeName, const bool fallbackValue) noexcept
{
    const auto* childElement = xmlElement.getChildByName(nodeName);
    if (childElement == nullptr) {
        return fallbackValue;
    }

    return StringUtil::stringToBool(childElement->getAllSubText());
}

int XmlHelper::readInt(const juce::XmlElement& xmlElement, const juce::String& nodeName, const int fallbackValue) noexcept
{
    const auto* childElement = xmlElement.getChildByName(nodeName);
    if (childElement == nullptr) {
        return fallbackValue;
    }

    // An empty node must be considered as "null".
    const auto subText = childElement->getAllSubText();
    if (subText.isEmpty()) {
        return fallbackValue;
    }

    return subText.getIntValue();
}

int XmlHelper::readInt(const juce::XmlElement& xmlElement, const int fallbackValue) noexcept
{
    const auto string = xmlElement.getAllSubText();
    if (string.isEmpty()) {
        return fallbackValue;
    }

    return string.getIntValue();
}

float XmlHelper::readFloat(const juce::XmlElement& xmlElement, const juce::String& nodeName, const float fallbackValue) noexcept
{
    const auto* childElement = xmlElement.getChildByName(nodeName);
    if (childElement == nullptr) {
        return fallbackValue;
    }

    // An empty node must be considered as "null".
    const auto subText = childElement->getAllSubText();
    if (subText.isEmpty()) {
        return fallbackValue;
    }

    return subText.getFloatValue();
}

unsigned int XmlHelper::readUnsignedInt(const juce::XmlElement& xmlElement, const juce::String& nodeName, const unsigned int fallbackValue) noexcept
{
    const auto* childElement = xmlElement.getChildByName(nodeName);
    if (childElement == nullptr) {
        return fallbackValue;
    }

    // An empty node must be considered as "null".
    const auto subText = childElement->getAllSubText();
    if (subText.isEmpty()) {
        return fallbackValue;
    }

    return static_cast<unsigned int>(subText.getLargeIntValue());
}

juce::String XmlHelper::readString(const juce::XmlElement& xmlElement, const juce::String& nodeName) noexcept
{
    const auto* childElement = xmlElement.getChildByName(nodeName);
    if (childElement == nullptr) {
        return { };
    }

    return childElement->getAllSubText();
}

bool XmlHelper::compare(const juce::XmlElement& inputXmlElement, const void* data, const size_t dataSize) noexcept
{
    // Converts the input file into an XML Document.
    bool success;    // NOLINT(*-init-variables)
    const juce::MemoryBlock expectedMemoryBlock(data, dataSize);
    const auto stringExpected = MemoryBlockUtil::extractString(expectedMemoryBlock, 0,
                                                               static_cast<juce::int64>(expectedMemoryBlock.getSize()), success, false);
    if (!success || stringExpected.isEmpty()) {
        jassertfalse;
        return false;
    }

    juce::XmlDocument document(stringExpected);
    document.setEmptyTextElementsIgnored(false);            // Does not work!!??
    const auto expectedXml = document.getDocumentElement();

    if (expectedXml == nullptr) {
        jassertfalse;
        return false;
    }

    // Compares the nodes.
    return compareNodeAndChildren(inputXmlElement, *expectedXml);
}

bool XmlHelper::compareNodeAndChildren(const juce::XmlElement& leftXmlElement, const juce::XmlElement& rightXmlElement) noexcept // NOLINT(*-no-recursion)
{
    if (leftXmlElement.getTagName() != rightXmlElement.getTagName()) {
        DBG("Node tag differs. Left is: " + leftXmlElement.getTagName() + ", right is: " + rightXmlElement.getTagName());
        return false;
    }
    // Useful for debugging.
    //DBG("Node tag: " + leftXmlElement.getTagName());
    //DBG("    SUBTEXT: " + leftXmlElement.getAllSubText());

    // Not the greatest algorithm of all time, but rendered difficult by JUCE inability to remove or keep ALL empty fields...

    // Compares each child, via recursion.
    const auto leftChildCount = leftXmlElement.getNumChildElements();
    const auto rightChildCount = rightXmlElement.getNumChildElements();
    auto leftIndex = 0;
    auto rightIndex = 0;

    while (true) {
        const auto* leftChild = leftXmlElement.getChildElement(leftIndex);
        const auto* rightChild = rightXmlElement.getChildElement(rightIndex);
        // If at a leaf, compares the text.
        if ((leftChild == nullptr) || (rightChild == nullptr)) {
            const auto leftText = leftXmlElement.getAllSubText();
            const auto rightText = rightXmlElement.getAllSubText();
            // Useful for debugging. See also above.
            if (leftText != rightText) {
                DBG("Comparison: " + leftText + " with " + rightText);
            }
            jassert(leftText == rightText);
            return (leftText == rightText);
        }

        // Skips the left child if empty.
        while (((leftChild == nullptr) || leftChild->getAllSubText().isEmpty()) && (leftIndex < leftChildCount)) {
            ++leftIndex;
            leftChild = leftXmlElement.getChildElement(leftIndex);
        }
        while (((rightChild == nullptr) || rightChild->getAllSubText().isEmpty()) && (rightIndex < rightChildCount)) {
            ++rightIndex;
            rightChild = rightXmlElement.getChildElement(rightIndex);
        }
        // End? Must be for both.
        if ((leftIndex >= leftChildCount) && (rightIndex >= rightChildCount)) {
            return true;
        }
        if ((leftChild == nullptr) || (rightChild == nullptr)) {
            jassertfalse; // Abnormal
            return false;
        }

        if (!compareNodeAndChildren(*leftChild, *rightChild)) {
            return false;
        }

        ++leftIndex;
        ++rightIndex;

        // Dirty...
        if ((leftIndex >= leftChildCount) && (rightIndex >= rightChildCount)) {
            return true;
        }
        if ((leftIndex >= leftChildCount) || (rightIndex >= rightChildCount)) {
            jassertfalse;       // One has finished, but not the other!!
            return false;
        }
    }
}

bool XmlHelper::hasOnlyText(const juce::XmlElement& node) noexcept
{
    const auto childCount = node.getNumChildElements();
    return ((childCount == 1) && node.getFirstChildElement()->isTextElement());
}

std::vector<std::pair<juce::String, juce::String>> XmlHelper::readTextOnlyNodes(const juce::XmlElement& rootNode) noexcept
{
    std::vector<std::pair<juce::String, juce::String>> keyValues;

    for (auto nodeIndex = 0, nodeCount = rootNode.getNumChildElements(); nodeIndex < nodeCount; ++nodeIndex) {
        const auto* node = rootNode.getChildElement(nodeIndex);

        if (XmlHelper::hasOnlyText(*node)) {
            keyValues.emplace_back(node->getTagName(), node->getAllSubText());
        }
    }

    return keyValues;
}

}   // namespace arkostracker

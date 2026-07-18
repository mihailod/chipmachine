#pragma once

#include <vector>

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Helper methods for XML. */
class XmlHelper
{
public:
    /** Prevents instantiation. */
    XmlHelper() = delete;

    /**
     * Adds a simple node with one bool.
     * @param parent the parent node.
     * @param nodeName the name of the new node.
     * @param value the boolean value.
     * @return the new node. It is owned by the parent node.
     */
    static juce::XmlElement& addValueNode(juce::XmlElement& parent, const juce::String& nodeName, bool value) noexcept
    {
        // Creates the new child node.
        auto& childXmlElement = addNode(parent, nodeName);
        childXmlElement.addTextElement(juce::String(value ? "true" : "false"));

        return childXmlElement;
    }

    /**
     * Adds a simple node with one value.
     * @tparam V the type of the value.
     * @param parent the parent node.
     * @param nodeName the name of the new node.
     * @param value the value (may be a String, an int, etc.).
     * @return the new node. It is owned by the parent node.
     */
    template<typename V>
    static juce::XmlElement& addValueNode(juce::XmlElement& parent, const juce::String& nodeName, V value) noexcept
    {
        // Creates the new child node.
        auto& childXmlElement = addNode(parent, nodeName);
        childXmlElement.addTextElement(juce::String(value));

        return childXmlElement;
    }

    /**
     * Adds a Node.
     * @param parent the parent node.
     * @param nodeName the name of the new node.
     * @return the new node. It is owned by the parent node.
     */
    static juce::XmlElement& addNode(juce::XmlElement& parent, const juce::String& nodeName) noexcept;

    /**
     * Returns all elements that have the node name, at the level of the given Xml Element.
     * @param xmlElement the Xml Element to browse.
     * @param nodeNames the node names to find at this level.
     * @return the elements. May be empty.
     */
    static std::vector<juce::XmlElement*> getChildrenList(const juce::XmlElement& xmlElement, const juce::String& nodeNames) noexcept;

    /**
     * Returns all elements that have the node name, at the level of the given Xml Element.
     * @param xmlElement the Xml Element to browse. As a convenience, if nullptr, empty is returned.
     * @param nodeNames the node names to find at this level.
     * @return the elements. May be empty.
     */
    static std::vector<juce::XmlElement*> getChildrenList(const juce::XmlElement* xmlElement, const juce::String& nodeNames) noexcept;

    /**
     * Reads a boolean from inside a node.
     * @param xmlElement the Xml Element where the child node should have a boolean.
     * @param nodeName the name of the child node.
     * @param fallbackValue used if the node is not found.
     * @return the boolean.
     */
    static bool readBool(const juce::XmlElement& xmlElement, const juce::String& nodeName, bool fallbackValue = false) noexcept;

    /**
     * Reads an int (32 bits!) value inside a node.
     * @param xmlElement the Xml Element where the child node should have an int.
     * @param nodeName the name of the child node.
     * @param fallbackValue used if the node is not found.
     * @return the int.
     */
    static int readInt(const juce::XmlElement& xmlElement, const juce::String& nodeName, int fallbackValue = 0) noexcept;

    /**
     * Reads an int (32 bits!) value directly inside a node.
     * @param xmlElement the Xml Element where the child node should have an int.
     * @param fallbackValue used if the node is not found.
     * @return the int.
     */
    static int readInt(const juce::XmlElement& xmlElement, int fallbackValue = 0) noexcept;

    /**
     * Reads a float value directly inside a node.
     * @param xmlElement the Xml Element where the child node should have a float.
     * @param nodeName the name of the child node.
     * @param fallbackValue used if the node is not found.
     * @return the int.
     */
    static float readFloat(const juce::XmlElement& xmlElement, const juce::String& nodeName, float fallbackValue = 0.0F) noexcept;

    /**
     * Reads an unsigned int value inside a node.
     * @param xmlElement the Xml Element where the child node should have an unsigned int.
     * @param nodeName the name of the child node.
     * @param fallbackValue used if the node is not found.
     * @return the unsigned int.
     */
    static unsigned int readUnsignedInt(const juce::XmlElement& xmlElement, const juce::String& nodeName, unsigned int fallbackValue = 0U) noexcept;

    /**
     * Reads the String value inside a node. If not found, an empty String is returned.
     * @param xmlElement the Xml Element where the child node should have a String.
     * @param nodeName the name of the child node.
     * @return the String.
     */
    static juce::String readString(const juce::XmlElement& xmlElement, const juce::String& nodeName) noexcept;

    /**
     * Compares an XML node and its children with the one from the given data.
     * @param inputXmlElement the XML element.
     * @param data the expected data.
     * @param dataSize the size of the expected data.
     * @return true if they are equal.
     */
    static bool compare(const juce::XmlElement& inputXmlElement, const void* data, size_t dataSize) noexcept;

    /**
     * Compares the nodes and their children. This method is recursive.
     * @param leftXmlElement the input XML element.
     * @param rightXmlElement the expected XML element.
     * @return true if all is equal.
     */
    static bool compareNodeAndChildren(const juce::XmlElement& leftXmlElement, const juce::XmlElement& rightXmlElement) noexcept;

    /**
     * @return true if the node has only text inside. Then, you can use juce::getAllSubText to get it.
     * @param node the Node.
     */
    static bool hasOnlyText(const juce::XmlElement& node) noexcept;

    /**
     * @return all the key/value of the sub-node of the given node. If the child has more than one item and it is not text, it is skipped.
     * @param rootNode the node which children are browsed.
     */
    static std::vector<std::pair<juce::String, juce::String>> readTextOnlyNodes(const juce::XmlElement& rootNode) noexcept;
};

}   // namespace arkostracker


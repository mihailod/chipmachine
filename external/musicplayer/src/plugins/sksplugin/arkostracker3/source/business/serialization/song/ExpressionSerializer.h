#pragma once

#include <memory>
#include <vector>

#include "../../../song/Expression.h"

namespace arkostracker 
{

/** Serializes/deserializes Expressions. */
class ExpressionSerializer
{
public:
    /** Prevents instantiation. */
    ExpressionSerializer() = delete;

    static const juce::String nodeExpressionsRoot;
    static const juce::String nodeArpeggiosRoot;
    static const juce::String nodePitchesRoot;

    static const juce::String nodeExpressionRoot;

    static const juce::String nodeName;
    static const juce::String nodeIsArpeggio;
    static const juce::String nodeSpeed;
    static const juce::String nodeShift;
    static const juce::String nodeLoopStartIndex;
    static const juce::String nodeEndIndex;

    static const juce::String nodeValues;
    static const juce::String nodeValue;

    /**
     * Serializes to XML the given Expressions.
     * @param expressions the Expressions to serialize.
     * @param useTypedNode if false, use "expressions" root node. Else use "arpeggios" or "pitches" according to the next parameters.
     * @param isArpeggioNode only used if "useTypedNode" is true. True if Arpeggio, false if Pitch.
     * @return the XML root node.
     */
    static std::unique_ptr<juce::XmlElement> serialize(const std::vector<Expression>& expressions, bool useTypedNode = false, bool isArpeggioNode = false) noexcept;

    /**
     * Deserializes an XML into Expressions.
     * @param root the XML root Element.
     * @return the Expressions, or empty if an error occurred.
     */
    static std::vector<std::unique_ptr<Expression>> deserializeExpressions(const juce::XmlElement& root) noexcept;

    /**
     * Serializes to XML the given Expression.
     * @param expression the Expression to serialize.
     * @return the XML node of the Expression.
     */
    static std::unique_ptr<juce::XmlElement> serialize(const Expression& expression) noexcept;

    /**
     * Deserializes an XML into one Expression.
     * @param expressionRoot the XML root Element of the Expression.
     * @return the Expression, or empty if an error occurred.
     */
    static std::unique_ptr<Expression> deserializeExpression(const juce::XmlElement& expressionRoot) noexcept;
};

}   // namespace arkostracker

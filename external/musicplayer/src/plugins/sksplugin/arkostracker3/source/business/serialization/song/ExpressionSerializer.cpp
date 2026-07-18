#include "ExpressionSerializer.h"

#include "../../../utils/XmlHelper.h"

namespace arkostracker 
{

const juce::String ExpressionSerializer::nodeExpressionsRoot = "expressions";           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String ExpressionSerializer::nodeArpeggiosRoot = "arpeggios";               // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String ExpressionSerializer::nodePitchesRoot = "pitches";                   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String ExpressionSerializer::nodeExpressionRoot = "expression";             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String ExpressionSerializer::nodeName = "name";                             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String ExpressionSerializer::nodeIsArpeggio = "isArpeggio";                 // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String ExpressionSerializer::nodeSpeed = "speed";                           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String ExpressionSerializer::nodeShift = "shift";                           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String ExpressionSerializer::nodeLoopStartIndex = "loopStartIndex";         // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String ExpressionSerializer::nodeEndIndex = "endIndex";                     // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String ExpressionSerializer::nodeValues = "values";                         // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String ExpressionSerializer::nodeValue = "value";                           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

std::unique_ptr<juce::XmlElement> ExpressionSerializer::serialize(const std::vector<Expression>& expressions, const bool useTypedNode, const bool isArpeggioNode) noexcept
{
    const auto rootNodeTag = !useTypedNode ? nodeExpressionsRoot : (isArpeggioNode ? nodeArpeggiosRoot : nodePitchesRoot);
    auto rootXml = std::make_unique<juce::XmlElement>(rootNodeTag);

    // As soon as an error occurs, returns an empty pointer.
    for (const auto& expression : expressions) {
        auto xmlPtr = serialize(expression);
        if (xmlPtr == nullptr) {
            jassertfalse;       // Serialization failed.
            return nullptr;
        }

        rootXml->addChildElement(xmlPtr.release());
    }

    return rootXml;
}

std::unique_ptr<juce::XmlElement> ExpressionSerializer::serialize(const Expression& expression) noexcept
{
    auto rootXml = std::make_unique<juce::XmlElement>(nodeExpressionRoot);

    const auto length = expression.getLength();
    XmlHelper::addValueNode(*rootXml, nodeName, expression.getName());
    XmlHelper::addValueNode(*rootXml, nodeIsArpeggio, expression.isArpeggio());
    XmlHelper::addValueNode(*rootXml, nodeSpeed, expression.getSpeed());
    XmlHelper::addValueNode(*rootXml, nodeShift, expression.getShift());
    XmlHelper::addValueNode(*rootXml, nodeLoopStartIndex, expression.getLoopStart());
    XmlHelper::addValueNode(*rootXml, nodeEndIndex, expression.getEnd());

    // Encodes the values.
    auto& xmlNodeValues = XmlHelper::addNode(*rootXml, nodeValues);

    for (auto index = 0; index < length; ++index) {
        const auto value = expression.getValue(index);
        XmlHelper::addValueNode(xmlNodeValues, nodeValue, value);
    }

    return rootXml;
}

std::vector<std::unique_ptr<Expression>> ExpressionSerializer::deserializeExpressions(const juce::XmlElement& root) noexcept
{
    std::vector<std::unique_ptr<Expression>> expressions;

    // Gets every Expression node.
    auto* expressionNode = root.getChildByName(nodeExpressionRoot);
    while (expressionNode != nullptr) {
        auto expression = deserializeExpression(*expressionNode);
        if (expression == nullptr) {
            return { };              // Stops as soon as an error is detected.
        }
        expressions.push_back(std::move(expression));

        // Next sibling.
        expressionNode = expressionNode->getNextElementWithTagName(nodeExpressionRoot);
    }

    return expressions;
}

std::unique_ptr<Expression> ExpressionSerializer::deserializeExpression(const juce::XmlElement& expressionRoot) noexcept
{
    const auto name = XmlHelper::readString(expressionRoot, nodeName);
    const auto isArpeggio = XmlHelper::readBool(expressionRoot, nodeIsArpeggio);
    const auto speed = XmlHelper::readInt(expressionRoot, nodeSpeed, 0);
    const auto shift = XmlHelper::readInt(expressionRoot, nodeShift, 0);
    const auto loopStart = XmlHelper::readInt(expressionRoot, nodeLoopStartIndex, 0);
    const auto end = XmlHelper::readInt(expressionRoot, nodeEndIndex, 0);

    auto expression = std::make_unique<Expression>(isArpeggio, name, false);

    // Reads the values.
    const auto* valuesNodes = expressionRoot.getChildByName(nodeValues);
    if (valuesNodes == nullptr) {
        jassertfalse;           // No value node!?
        return nullptr;
    }
    const auto valueNodes = XmlHelper::getChildrenList(*valuesNodes, nodeValue);
    for (const auto* valueNode : valueNodes) {
        const auto value = XmlHelper::readInt(*valueNode, 0);
        expression->addValue(value);
    }

    expression->setSpeed(speed);
    expression->setShift(shift);
    expression->setLoopStartAndEnd(loopStart, end);

    return expression;
}

}   // namespace arkostracker

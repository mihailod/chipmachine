#include "ExpressionHandler.h"

namespace arkostracker 
{

ExpressionHandler::ExpressionHandler(const bool pIsArpeggio, const bool pCreateDefaultExpression) noexcept :
        isArpeggio(pIsArpeggio),
        expressions(),
        mutex()
{
    if (pCreateDefaultExpression) {
        expressions.push_back(std::make_unique<Expression>(isArpeggio, juce::translate("Default"), true));
    }
}

ExpressionHandler::ExpressionHandler(const ExpressionHandler& pSource) noexcept :
        isArpeggio(pSource.isArpeggio),
        expressions(),
        mutex()
{
    // Copies the Expressions.
    expressions.reserve(pSource.expressions.size());
    for (const auto& expression : pSource.expressions) {
        expressions.push_back(std::make_unique<Expression>(*expression));
    }
}

void ExpressionHandler::performOnExpressions(const std::function<void(std::vector<std::unique_ptr<Expression>>&)>& lockedOperation) noexcept
{
    const std::lock_guard lock(mutex);        // Lock!
    lockedOperation(expressions);
}

void ExpressionHandler::performOnConstExpressions(const std::function<void(const std::vector<std::unique_ptr<Expression>>&)>& lockedOperation) const noexcept
{
    const std::lock_guard lock(mutex);        // Lock!
    lockedOperation(expressions);
}

void ExpressionHandler::performOnConstExpressionFromIndex(const int expressionIndex, const std::function<void(const Expression&)>& lockedOperation) const noexcept
{
    const std::lock_guard lock(mutex);        // Lock!

    const auto* expression = findExpressionFromIndex_noLock(expressionIndex);
    if (expression == nullptr) {
        jassertfalse;       // Abnormal!
        return;
    }

    lockedOperation(*expression);
}

void ExpressionHandler::performOnExpression(const Id& expressionId, const std::function<void(Expression&)>& lockedOperation) noexcept
{
    const std::lock_guard lock(mutex);        // Lock!

    auto* expression = findExpression_noLock(expressionId);
    if (expression == nullptr) {
        jassertfalse;           // Instrument doesn't exist!
        return;
    }
    lockedOperation(*expression);
}

void ExpressionHandler::performOnConstExpression(const Id& expressionId, const std::function<void(const Expression&)>& lockedOperation) const noexcept
{
    const std::lock_guard lock(mutex);        // Lock!

    const auto* expression = findExpression_noLock(expressionId);
    if (expression == nullptr) {
        jassertfalse;           // Expression doesn't exist!
        return;
    }
    lockedOperation(*expression);
}

int ExpressionHandler::addExpression(const Expression& newExpression, const int index) noexcept
{
    jassert(isArpeggio == newExpression.isArpeggio());

    auto newExpressionPtr = std::make_unique<Expression>(newExpression);

    // Inserts at the end or at a specific location.
    int newIndex;           // NOLINT(*-init-variables)
    if (index < 0) {
        newIndex = static_cast<int>(expressions.size());
        expressions.push_back(std::move(newExpressionPtr));
    } else {
        newIndex = index;
        expressions.insert(expressions.cbegin() + index, std::move(newExpressionPtr));
    }

    return newIndex;
}

std::vector<juce::String> ExpressionHandler::getNames() const noexcept
{
    std::vector<juce::String> result;
    {
        const std::lock_guard lock(mutex);        // Lock!

        result.reserve(expressions.size());
        for (const auto& expression : expressions) {
            result.push_back(expression->getName());
        }
    }

    return result;
}

std::vector<Id> ExpressionHandler::getIds() const noexcept
{
    std::vector<Id> ids;
    {
        const std::lock_guard lock(mutex);        // Lock!

        ids.reserve(expressions.size());
        for (const auto& expression : expressions) {
            ids.push_back(expression->getId());
        }
    }

    return ids;
}

OptionalId ExpressionHandler::getId(const int index) const noexcept
{
    const std::lock_guard lock(mutex);        // Lock!
    if (static_cast<size_t>(index) < expressions.size()) {
        return expressions.at(static_cast<size_t>(index))->getId();
    }

    return { };
}

int ExpressionHandler::getCount() const noexcept
{
    const std::lock_guard lock(mutex);        // Lock!
    return static_cast<int>(expressions.size());
}

juce::String ExpressionHandler::getName(const Id& expressionId) const noexcept
{
    const std::lock_guard lock(mutex);        // Lock!

    const auto* expression = findExpression_noLock(expressionId);
    if (expression == nullptr) {
        jassertfalse;       // Expression not found?
        return { };
    }

    return expression->getName();
}

juce::String ExpressionHandler::getNameFromIndex(const int expressionIndex) const noexcept
{
    const std::lock_guard lock(mutex);        // Lock!

    const auto* expression = findExpressionFromIndex_noLock(expressionIndex);
    if (expression == nullptr) {
        jassertfalse;       // Abnormal!
        return { };
    }

    return expression->getName();
}

bool ExpressionHandler::isReadOnly(const Id& expressionId) const noexcept
{
    const auto expressionIndex = find(expressionId);
    if (expressionIndex.isAbsent()) {
        jassertfalse;       // Searching for a non-existing Instrument?
        return true;
    }

    return (expressionIndex == 0);
}

OptionalInt ExpressionHandler::find(const Id& expressionId) const noexcept
{
    const std::lock_guard lock(mutex);        // Lock!

    const auto findIterator = std::find_if(expressions.cbegin(), expressions.cend(), [&] (const std::unique_ptr<Expression>& expression) {
        return (expression->getId() == expressionId);
    });
    if (findIterator == expressions.cend()) {
        return { };
    }

    return static_cast<int>(std::distance(expressions.cbegin(), findIterator));
}

OptionalId ExpressionHandler::find(const int expressionIndex) const noexcept
{
    const std::lock_guard lock(mutex);        // Lock!

    const auto* expression = findExpressionFromIndex_noLock(expressionIndex);
    if (expression == nullptr) {
        jassertfalse;      // Doesn't exist anymore, but can still be referenced in a pattern.
        return { };
    }

    return expression->getId();
}

Id ExpressionHandler::getLastId() const noexcept
{
    const std::lock_guard lock(mutex);        // Lock!
    jassert(!expressions.empty());

    return (*expressions.crbegin())->getId();

}

void ExpressionHandler::setName(const Id& expressionId, const juce::String& newName) noexcept
{
    const std::lock_guard lock(mutex);        // Lock!

    auto* expression = findExpression_noLock(expressionId);
    if (expression == nullptr) {
        jassertfalse;       // Expression not found?
        return;
    }

    expression->setName(newName);
}

Expression ExpressionHandler::getExpressionCopy(const Id& expressionId) const noexcept
{
    const std::lock_guard lock(mutex);        // Lock!

    const auto* expression = findExpression_noLock(expressionId);
    if (expression == nullptr) {
        jassertfalse;       // Expression not found?
        return Expression();
    }

    return *expression;
}

Expression ExpressionHandler::getExpressionCopy(const int expressionIndex) const noexcept
{
    const std::lock_guard lock(mutex);        // Lock!

    const auto* expression = findExpressionFromIndex_noLock(expressionIndex);
    if (expression == nullptr) {
        jassertfalse;       // Expression not found?
        return Expression();
    }

    return *expression;
}

const Expression* ExpressionHandler::findExpression_noLock(const Id& expressionId) const noexcept
{
    const auto findIterator = std::find_if(expressions.cbegin(), expressions.cend(), [&] (const std::unique_ptr<Expression>& expression) {
        return (expression->getId() == expressionId);
    });
    return (findIterator == expressions.cend()) ? nullptr : findIterator->get();
}

Expression* ExpressionHandler::findExpression_noLock(const Id& expressionId) noexcept
{
    return const_cast<Expression*>(static_cast<const ExpressionHandler&>(*this).findExpression_noLock(expressionId));   // See Effective C++ by Scott Meyer, item 3. NOLINT(*-pro-type-const-cast)
}

const Expression* ExpressionHandler::findExpressionFromIndex_noLock(const int expressionIndex) const noexcept
{
    if (expressionIndex >= static_cast<int>(expressions.size())) {
        return nullptr;
    }

    return expressions.at(static_cast<size_t>(expressionIndex)).get();
}


}   // namespace arkostracker


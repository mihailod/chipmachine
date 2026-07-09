#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "Expression.h"

namespace arkostracker 
{

/**
 * Holder and manages one type of Expressions in a Song (Arpeggios and Pitches).
 * Manages concurrent access, but like the Song, does NOT manage the observations.
 */
class ExpressionHandler
{
public:
    friend class SongTestHelper;

    /**
     * Constructor.
     * @param isArpeggio true if this is about Arpeggio, false if about Pitch.
     * @param createDefaultExpression true to create a first valid Expression.
     */
    explicit ExpressionHandler(bool isArpeggio, bool createDefaultExpression) noexcept;

    /** Copy constructor. */
    ExpressionHandler(const ExpressionHandler&) noexcept;

    /**
     * Allows to perform a const operation an Expression list. A lock is used during the whole operation on ALL the expressions of this kind.
     * Even though using an index, this method can still be useful for objects manipulating only indexes, such as the player.
     * @param expressionId the ID of the Expression. If not found, nothing happens (but asserts).
     * @param lockedOperation the operation to perform. Be as short as possible! Do not call any locked operation,
     * it will provoke a deadlock!
     */
    void performOnConstExpression(const Id& expressionId, const std::function<void(const Expression&)>& lockedOperation) const noexcept;

    /**
     * Allows to perform a const operation an Expression list. A lock is used during the whole operation on ALL the expressions of this kind.
     * Even though using an index, this method can still be useful for objects manipulating only indexes, such as the player.
     * @param expressionIndex the index of the Expression. Safe, if out of bounds, nothing happens (but asserts).
     * @param lockedOperation the operation to perform. Be as short as possible! Do not call any locked operation,
     * it will provoke a deadlock!
     */
    void performOnConstExpressionFromIndex(int expressionIndex, const std::function<void(const Expression&)>& lockedOperation) const noexcept;

    /**
     * Allows to perform an operation an Expression list. A lock is used during the whole operation on ALL the expressions of this kind.
     * @param expressionId the ID of the Expression. If not found, nothing happens (but asserts).
     * @param lockedOperation the operation to perform. Be as short as possible! Do not call any locked operation,
     * it will provoke a deadlock!
     */
    void performOnExpression(const Id& expressionId, const std::function<void(Expression&)>& lockedOperation) noexcept;

    /**
     * Allows to perform an operation on the Expression list. A lock is used during the whole operation.
     * @param lockedOperation the operation to perform. Be as short as possible! Do not call any locked operation,
     * it will provoke a deadlock!
     */
    void performOnExpressions(const std::function<void(std::vector<std::unique_ptr<Expression>>&)>& lockedOperation) noexcept;

    /**
     * Allows to perform an operation on the Expression list. A lock is used during the whole operation.
     * @param lockedOperation the operation to perform. Be as short as possible! Do not call any locked operation,
     * it will provoke a deadlock!
     */
    void performOnConstExpressions(const std::function<void(const std::vector<std::unique_ptr<Expression>>&)>& lockedOperation) const noexcept;

    /**
     * Adds an expression.
     * @param newExpression the Expression to add. A copy is performed.
     * @param index the index (>0) or -1 to put at the end. The index must NOT be after the end!
     * @return the index of the new Expression.
     */
    int addExpression(const Expression& newExpression, int index = -1) noexcept;

    /** @return the names of all the Expressions, except for the first one. */
    std::vector<juce::String> getNames() const noexcept;

    /** @return the IDs of all the Expressions. */
    std::vector<Id> getIds() const noexcept;
    /**
     * @return an ID of an Expressions.
     * @param index the index (>=0). If out of found, "absent" is returned.
     */
    OptionalId getId(int index) const noexcept;

    /** @return how many there are (>=0, but in practice, should be >=1 as the default one is always present). */
    int getCount() const noexcept;
    /**
     * @returns the name of the Expression. If not found, an empty String is returned (abnormal).
     * @param expressionId the ID of the expression.
     */
    juce::String getName(const Id& expressionId) const noexcept;

    /**
     * @returns the name of the Expression. If not found, an empty String is returned (abnormal).
     * @param expressionIndex the index. Should be valid.
     */
    juce::String getNameFromIndex(int expressionIndex) const noexcept;

    /**
     * @return true if the Expression is read-only (i.e. is the first one).
     * @param expressionId the ID of the Expression. If not found, returns true.
     */
    bool isReadOnly(const Id& expressionId) const noexcept;

    /**
     * @return the index of the expression which ID is given, or empty if not found.
     * @param expressionId the ID of the Expression.
     */
    OptionalInt find(const Id& expressionId) const noexcept;

    /**
     * @return the ID of the expression which index is given, or empty if not found.
     * @param expressionIndex the index of the Expression.
     */
    OptionalId find(int expressionIndex) const noexcept;

    /** @return the ID of the last Expression. */
    Id getLastId() const noexcept;

    /**
     * Renames an Arpeggio.
     * @param expressionId the ID of the expression. Must be valid.
     * @param newName the new name.
     */
    void setName(const Id& expressionId, const juce::String& newName) noexcept;

    /**
     * Gets a copy of an Expression. It is safe: if not found, an empty one is returned.
     * @param expressionId the ID of the expression.
     * @return the copy of the Expression.
     */
    Expression getExpressionCopy(const Id& expressionId) const noexcept;

    /**
     * Gets a copy of an Expression. It is safe: if not found, an empty one is returned.
     * @param expressionIndex the index of the expression.
     * @return the copy of the Expression.
     */
    Expression getExpressionCopy(int expressionIndex) const noexcept;

private:
    /**
     * @return the Expression from its ID, or nullptr if not found. This does NOT lock, so the caller MUST do it!
     * @param expressionId the ID of the Expression.
     */
    const Expression* findExpression_noLock(const Id& expressionId) const noexcept;

    /**
     * @return the Expression from its ID, or nullptr if not found. This does NOT lock, so the caller MUST do it!
     * @param expressionId the ID of the Expression.
     */
    Expression* findExpression_noLock(const Id& expressionId) noexcept;

    /**
     * @return the Expression from its index, or nullptr if not found. This does NOT lock, so the caller MUST do it!
     * @param expressionIndex the index of the Expression.
     */
    const Expression* findExpressionFromIndex_noLock(int expressionIndex) const noexcept;

    bool isArpeggio;                                            // True if this is about Arpeggio, false if about Pitch.
    std::vector<std::unique_ptr<Expression>> expressions;       // The 0th is the default one. There must be at least one!

    mutable std::mutex mutex;                                   // Protects EACH access to all the Expressions.

    JUCE_LEAK_DETECTOR (ExpressionHandler)

    // WARNING! The COPY CONSTRUCTOR must be updated if one field is added!
};

}   // namespace arkostracker

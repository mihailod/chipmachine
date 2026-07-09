#pragma once

#include "Id.h"

namespace arkostracker 
{

/**
 * Holder of a possible value.
 * @tparam TYPE the type of the value.
 */
template<typename TYPE>
class OptionalValue             // TODO TU this.
{
public:
    /** Constructor for an absent value. */
    OptionalValue() noexcept :
            value()
    {
    }

    /**
     * Constructor for a present value.
     * @param pValue the value.
     */
    OptionalValue(TYPE pValue) noexcept :            // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
            value(pValue),
            present(true)
    {
    }

    /** @return the value. If not present, a default value is used! Check the isPresent method first! */
    TYPE getValue() const noexcept
    {
        // TODO Should be put, but provokes many warnings, I don't understand why yet.
        // jassert(isPresent());                   // Reading an absent value! A problem in your algorithm, my dear!
        return value;
    }
    const TYPE& getValueRef() const noexcept
    {
        return value;
    }

    /** @return true if the value is present. */
    bool isPresent() const noexcept
    {
        return present;
    }

    /** @return true if the value is not present. */
    bool isAbsent() const noexcept
    {
        return !present;
    }

    bool operator==(const OptionalValue& rhs) const         // NOLINT(fuchsia-overloaded-operator)
    {
        // If not present, no need to compare the value.
        if (present != rhs.present) {
            return false;
        }
        if (!present) {
            return true;
        }

        // Both present values.
        return (value == rhs.value);
    }

    bool operator!=(const OptionalValue& rhs) const         // NOLINT(fuchsia-overloaded-operator)
    {
        return !rhs.operator==(*this);                      // Written this way to prevent a stupid warning.
    }

private:
    TYPE value;
    bool present{false};
};

/** Equality comparator. */
template<typename L, typename R>
bool operator==(const OptionalValue<L>& rhs, R lhs)
{
    if (rhs.isAbsent()) {
        return false;
    }
    return rhs.getValueRef() == lhs;
}

using OptionalInt = OptionalValue<int>;
using OptionalBool = OptionalValue<bool>;
using OptionalLong = OptionalValue<juce::int64>;
using OptionalFloat = OptionalValue<float>;
using OptionalId = OptionalValue<Id>;           // FIXME not great, this should rather be declared in Id, no?

}   // namespace arkostracker


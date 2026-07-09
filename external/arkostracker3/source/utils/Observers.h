#pragma once

#include <algorithm>
#include <functional>
#include <vector>

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Simple Observer class to put Observers. */
template<typename TYPE>
class Observers
{
public:
    /** Constructor. */
    Observers() :
        observers()
    {
    }

    /** @return true if no Observer is present. */
    bool empty() const noexcept
    {
        return observers.empty();
    }

    /** @return true if at least one Observer is present. */
    bool arePresent() const noexcept
    {
        return !empty();
    }

    /**
     * Adds an Observer. It shouldn't be already here, but if that's the case, it is not added (assertion).
     * @param observerToAdd the Observer to add.
     */
    void addObserver(TYPE* observerToAdd) noexcept
    {
        // The observer must not already exist!
        if (std::find(observers.cbegin(), observers.cend(), observerToAdd) == observers.cend()) {
            observers.push_back(observerToAdd);
        } else {
            jassertfalse;           // Should not happen! The observer is already present!
        }
    }

    /**
     * Removes an Observer. It should be present. If not, an assertion is raised.
     * @param observerToRemove the Observer to remove.
     */
    void removeObserver(TYPE* observerToRemove)
    {
        auto iterator = std::find(observers.cbegin(), observers.cend(), observerToRemove);

        // The observer must exist!
        if (iterator != observers.cend()) {
            observers.erase(iterator);
        } else {
            jassertfalse;           // Should not happen! The observer is not present!
        }
    }

    /**
     * Applies an action on each Observer.
     * @param action the action.
     */
    void applyOnObservers(std::function<void(TYPE*)> action) noexcept                // TODO Would be cleaner is was a reference.
    {
        for (TYPE* observer : observers) {
            action(observer);
        }
    }

private:
    std::vector<TYPE*> observers;
};

}   // namespace arkostracker


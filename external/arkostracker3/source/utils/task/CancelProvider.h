#pragma once

namespace arkostracker 
{

/** An pure abstract class for objects to know if a task (for example) has been cancelled, via an object implementing the CancelProvider class. */
class CancelProvider
{
public:
    /** Destructor. */
    virtual ~CancelProvider() = default;

    /** @return true if the task has been cancelled. */
    virtual bool isCanceled() const = 0;
};

}   // namespace arkostracker


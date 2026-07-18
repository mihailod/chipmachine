#pragma once

namespace arkostracker 
{

/** Listener to know how a task progresses. */
class TaskProgressListener
{
public:
    /** Destructor.*/
    virtual ~TaskProgressListener() = default;

    /**
     * Called when a progress has been made.
     * @param progress the progress.
     * @param progressMaximumValue the progress maximum value.
     */
    virtual void onTaskProgressed(int progress, int progressMaximumValue) noexcept = 0;
};

}   // namespace arkostracker


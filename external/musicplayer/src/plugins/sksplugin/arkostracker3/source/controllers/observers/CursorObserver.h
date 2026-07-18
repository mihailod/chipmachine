#pragma once

namespace arkostracker 
{

class CursorLocation;

/** Observer for objects wanting to be notified when the cursor changes. */
class CursorObserver
{
public:
    /** Destructor. */
    virtual ~CursorObserver() = default;

    /**
     * Called when the cursor location changes. This does not include the line, only the horizontal position.
     * @param cursorLocation the new location.
     */
    virtual void onCursorChannelMoved(const CursorLocation& cursorLocation) = 0;
};

}   // namespace arkostracker


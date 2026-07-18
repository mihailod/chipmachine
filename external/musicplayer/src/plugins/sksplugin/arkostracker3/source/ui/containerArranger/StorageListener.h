#pragma once

namespace arkostracker 
{

/** To interact with the object that knows how to store/restore the arrangements. */
class StorageListener
{
public:
    /** Destructor. */
    virtual ~StorageListener() = default;

    /** @return the number of the current arrangement. */
    virtual int getCurrentArrangementNumber() const = 0;

    /**
     * Called to recall an arrangement.
     * @param arrangementNumber the number of the arrangement.
     */
    virtual void onRestoreArrangement(int arrangementNumber) = 0;
};

}   // namespace arkostracker

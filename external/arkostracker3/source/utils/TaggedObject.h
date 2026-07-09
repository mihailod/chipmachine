#pragma once

namespace arkostracker 
{

/** Holder of a tag and any object. It's like a specialized pair. */
template<typename TAG, typename OBJECT>
class TaggedObject
{
public:
    /**
     * Constructor.
     * @param pObject the object to store. A copy is performed.
     * @param pTag the tag to store.
     */
    explicit TaggedObject(const OBJECT& pObject, TAG pTag) :
            object(pObject),
            tag(pTag)
    {
    }

    /** @return the tag of the object. */
    const TAG& getTag() const noexcept {
        return tag;
    }

    /** @return the object. */
    const OBJECT& getObjectRef() const noexcept {
        return object;
    }

private:
    const OBJECT object;
    const TAG tag;
};

}   // namespace arkostracker

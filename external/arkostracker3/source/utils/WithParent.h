#pragma once

namespace arkostracker 
{

/**
 * Template class for objects that want to declare their parent. This is useful when having internal subclasses, so that the
 * overridden methods can access easily the method of the their parent.
 * @tparam PARENT the class of the Parent class.
 */
template<typename PARENT>
class WithParent
{
public:
    /**
     * Constructor.
     * @param parent the parent.
     */
    explicit WithParent(PARENT& parent) :
            parentObject(parent)
    {
    }

    PARENT& parentObject;           // The parent.   // NOLINT(clion-misra-cpp2008-11-0-1, *-non-private-member-variables-in-classes)
};

}   // namespace arkostracker


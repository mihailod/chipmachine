#pragma once

namespace arkostracker 
{

/**
 * Dumb GCC can't use unordered_map of enumeration without a hasher, so here is a generic one.
 * Stolen here:
 * http://stackoverflow.com/questions/18837857/cant-use-enum-class-as-unordered-map-key
 */
class EnumHasher
{
public:
    template<typename TYPE>
    std::size_t operator()(TYPE item) const
    {
        return static_cast<std::size_t>(item);
    }
};

}   // namespace arkostracker


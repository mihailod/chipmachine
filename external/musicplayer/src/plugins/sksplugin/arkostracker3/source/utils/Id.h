#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** A unique ID. Directly uses the one from JUCE, but the default int constructor was error-prone, so I wanted to hide it. */
class Id : public juce::Uuid
{
public:
    /** Constructor. */
    Id() noexcept = default;
};

}   // namespace arkostracker

/** Specialization of the Hash. Stolen directly from juce::Uuid, adapted to this class. */
namespace std
{
    template<>
    struct hash<::arkostracker::Id>
    {
        size_t operator()(const juce::Uuid& uuid) const noexcept
        {
            return static_cast<size_t>(uuid.hash());
        }
    };
}   // namespace std

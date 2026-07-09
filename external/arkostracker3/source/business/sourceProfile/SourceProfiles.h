#pragma once

#include <vector>

#include "SourceProfile.h"

namespace arkostracker 
{

/** Returns the template source profiles. */
class SourceProfiles
{
public:
    /** Prevents instantiation. */
    SourceProfiles() = delete;

    /** @return the template source profiles. */
    static std::vector<SourceProfile> getTemplateSourceProfiles() noexcept;

    /** @return a default source profile (Z80). */
    static SourceProfile buildZ80Default() noexcept;
};



}   // namespace arkostracker


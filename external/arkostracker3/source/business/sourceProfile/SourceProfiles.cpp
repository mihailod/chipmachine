#include "SourceProfiles.h"

namespace arkostracker 
{

std::vector<SourceProfile> SourceProfiles::getTemplateSourceProfiles() noexcept
{
    std::vector<SourceProfile> profiles;

    profiles.push_back(buildZ80Default());
    profiles.emplace_back(SourceGeneratorConfiguration::build68000(), "68000", true);
    profiles.emplace_back(SourceGeneratorConfiguration::build6502Acme(), "6502 (ACME)", true);
    profiles.emplace_back(SourceGeneratorConfiguration::build6502Mads(), "6502 (MADS)", true);

    return profiles;
}

SourceProfile SourceProfiles::buildZ80Default() noexcept
{
    return { SourceGeneratorConfiguration::buildZ80(), "Z80", true };
}

}   // namespace arkostracker

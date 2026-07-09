#pragma once

#include <set>

#include "PlayerConfigurationFlag.h"

namespace arkostracker 
{

/** A player configuration. Represents possible flags to configure how the player will compile, optimizing it. */
class PlayerConfiguration
{
public:
    /**
     * Constructor.
     * @param isMusic true for music, false for sfx.
     */
    explicit PlayerConfiguration(bool isMusic = true) noexcept;

    /**
     * Adds a flag. It may already be here. This may trigger the adding of other flags.
     * @param flag the flag.
     */
    void addFlag(PlayerConfigurationFlag flag) noexcept;

    /**
     * Adds a flag, if wanted (useful to avoid IFs). It may already be here. This may trigger the adding of other flags.
     * @param addFlag true to add the flag.
     * @param flag the flag.
     */
    void addFlag(bool addFlag, PlayerConfigurationFlag flag) noexcept;

    /** @return the already added flags, in the order they were added. */
    std::set<PlayerConfigurationFlag> getFlags() const noexcept;

private:
    std::set<PlayerConfigurationFlag> flags;
};

}   // namespace arkostracker

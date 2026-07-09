#pragma once

#include <memory>

#include "../serialization/StoredLookAndFeel.h"

namespace arkostracker
{

/** Builds the B&W theme. */
class ThemeBlackAndWhiteBuilder
{
public:
    /** Prevents instantiation; */
    ThemeBlackAndWhiteBuilder() = delete;

    static const juce::String themeName;

    /** @return the look and feel. */
    static std::unique_ptr<StoredLookAndFeel> build() noexcept;
};

}   // namespace arkostracker

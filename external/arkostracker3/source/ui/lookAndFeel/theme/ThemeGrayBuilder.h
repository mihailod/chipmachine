#pragma once

#include <memory>

#include "../serialization/StoredLookAndFeel.h"

namespace arkostracker
{

/** Builds the Gray theme. */
class ThemeGrayBuilder
{
public:
    /** Prevents instantiation; */
    ThemeGrayBuilder() = delete;

    static const juce::String themeName;

    /** @return the look and feel. */
    static std::unique_ptr<StoredLookAndFeel> build() noexcept;
};

}   // namespace arkostracker

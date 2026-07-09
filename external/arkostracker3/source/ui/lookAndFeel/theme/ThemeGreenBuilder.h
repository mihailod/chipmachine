#pragma once

#include <memory>

#include "../serialization/StoredLookAndFeel.h"

namespace arkostracker
{

/** Builds the Green theme. */
class ThemeGreenBuilder
{
public:
    /** Prevents instantiation; */
    ThemeGreenBuilder() = delete;

    static const juce::String themeName;

    /** @return the look and feel. */
    static std::unique_ptr<StoredLookAndFeel> build() noexcept;
};

}   // namespace arkostracker

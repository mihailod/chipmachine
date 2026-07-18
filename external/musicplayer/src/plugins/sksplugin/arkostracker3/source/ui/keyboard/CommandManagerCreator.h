#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Category.h"

namespace arkostracker 
{

/** Creates an ApplicationCommandManager, filtering the given KeyPressMappingSet with the selected category. */
class CommandManagerCreator
{
public:
    /** Prevents instantiation. */
    CommandManagerCreator() = delete;

    static std::unique_ptr<juce::ApplicationCommandManager> create(Category categoryToKeep, const juce::ApplicationCommandManager& previousApplicationCommandManager) noexcept;
};

}   // namespace arkostracker


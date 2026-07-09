#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** The various category. Only really useful to avoid having to use Strings. */
enum class Category
{
    general,
    play,
    testArea,
    lists,
    linker,
    patternViewer,
    barEditor,
    virtualKeyboard,
};

class CategoryUtils
{
public:
    /** Prevents instantiation. */
    CategoryUtils() = delete;

    /**
     * @returns the localized String to use in the Keyboard mapping Component, from the given Category.
     * @param category the Category.
     */
    static juce::String getCategoryString(Category category) {
        switch (category) {
            default:
                jassertfalse;       // Shouldn't happen.
            case Category::general:
                return juce::translate("General");
            case Category::play:
                return juce::translate("Play");
            case Category::testArea:
                return juce::translate("Test area");
            case Category::lists:
                return juce::translate("Lists");
            case Category::linker:
                return juce::translate("Linker");
            case Category::patternViewer:
                return juce::translate("Pattern viewer");
            case Category::barEditor:
                return juce::translate("Bar editor");
            case Category::virtualKeyboard:
                return juce::translate("Virtual keyboard");
        }
    }
};

}   // namespace arkostracker


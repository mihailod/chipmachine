#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "serialization/StoredLookAndFeel.h"

namespace arkostracker 
{

/** Creates various looks and feels. */
class LookAndFeelFactory
{
public:
    /** Prevents instantiation. */
    LookAndFeelFactory() = delete;

    static const juce::String themeNameBlackAndWhite;

    /** @return the IDs of native themes. */
    static std::vector<int> getNativeThemeIds() noexcept;

    /**
     * @return the StoredLookAndFeel from the given Theme ID, either from the Factory (if a factory one), or the preferences (if a custom one).
     * If any problem, the default one is used.
     * @param themeId the ID of the Theme.
     */
    static std::unique_ptr<StoredLookAndFeel> retrieveLookAndFeel(int themeId) noexcept;

    /**
     * @return the native (only) theme from its ID. If not found, a fallback default theme is used.
     * @param id the ID. See the ThemeId class.
     */
    static std::unique_ptr<StoredLookAndFeel> buildNativeThemeFromId(int id) noexcept;

    /**
     * Builds a LookAndFeel model from the given LookAndFeel.
     * @param lookAndFeel the JUCe LookAndFeel.
     * @param profileId the ID of the Profile.
     * @param name the name.
     * @param readOnly true if read only.
     * @return the model.
     */
    static std::unique_ptr<StoredLookAndFeel> buildStoredLookAndFeelFromGivenLookAndFeel(const juce::LookAndFeel& lookAndFeel,
                                                                                         int profileId, const juce::String& name, bool readOnly) noexcept;

private:
    /**
     * Convenience method to get the ARGB color from the given look and feel. It shouldn't be absent!
     * @tparam COLOR_ENUM the enumeration of the color, to be converted to Int (Label::ColourIds::textColourId or
     * LookAndFeelConstants::Colors::panelBackground for example).
     * @param lookAndFeel the look and feel to find the colour into.
     * @param colorToFind the color enum.
     * @return the ARGB color.
     */
    template<typename COLOR_ENUM>
    static unsigned int getColourArgb(const juce::LookAndFeel& lookAndFeel, COLOR_ENUM colorToFind) noexcept
    {
        return lookAndFeel.findColour(static_cast<int>(colorToFind)).getARGB();
    }
};

}   // namespace arkostracker

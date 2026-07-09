#pragma once

#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

namespace arkostracker 
{

class SourceProfile;
class SourceGeneratorConfiguration;

/** Validates that the given SourceProfile is valid. */
class SourceProfileValidator
{
public:
    enum class Error : uint8_t
    {
        missingPlaceholder,
        tooManyPlaceholders,
        /** For tabulation size. */
        invalidValue,
        /** For extension. */
        mustNotStartWithDot,
        /** For extension. */
        empty
    };

    enum class Location : uint8_t
    {
        changeAddressLocationDeclaration,
        byteDeclaration,
        wordDeclaration,
        addressDeclaration,
        stringDeclaration,
        constantDeclaration,
        labelDeclaration,
        commentDeclaration,
        tabulationSize,
        asmExtension,
        binExtension,
    };

    /** Prevents instantiation. */
    SourceProfileValidator() = delete;

    /**
     * @return the possible errors, or empty if there are none.
     * @param sourceProfile the SourceProfile to check.
     */
    static std::vector<std::pair<Location, Error>> validate(const SourceProfile& sourceProfile) noexcept;

    /**
     * @return a corrected tabulation size.
     * @param tabulationSize the input size.
     */
    static int correctTabulationSize(int tabulationSize) noexcept;

    /**
     * @return a human-readable error String from the given error.
     * @param error the error.
     */
    static juce::String errorToDisplayableError(Error error) noexcept;

    /**
     * @return a human-readable error String from the given location.
     * @param location the location.
     */
    static juce::String locationToDisplayableLocation(Location location) noexcept;

private:
    static const int minimumTabulationLength;
    static const int maximumTabulationLength;

    /**
     * Checks the given extension and fills the given vector accordingly.
     * @param errorsToFill the errors to fill.
     * @param extension the extension.
     * @param location the location in the configuration.
     */
    static void checkExtension(std::vector<std::pair<Location, Error>>& errorsToFill, const juce::String& extension,
                               Location location) noexcept;

    /**
     * Checks the given declaration ("db {x}" for example) and fills the given vector accordingly.
     * @param errorsToFill the errors to fill.
     * @param declaration the declaration.
     * @param location the location in the configuration.
     */
    static void checkDeclaration(std::vector<std::pair<Location, Error>>& errorsToFill, const juce::String& declaration,
                               Location location) noexcept;

};

}   // namespace arkostracker

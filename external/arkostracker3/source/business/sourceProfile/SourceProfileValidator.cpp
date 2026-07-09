#include "SourceProfileValidator.h"

#include "../../utils/NumberUtil.h"
#include "SourceProfile.h"

namespace arkostracker
{

const int SourceProfileValidator::minimumTabulationLength = 1;
const int SourceProfileValidator::maximumTabulationLength = 16;

std::vector<std::pair<SourceProfileValidator::Location, SourceProfileValidator::Error>> SourceProfileValidator::validate(const SourceProfile& sourceProfile) noexcept
{
    std::vector<std::pair<Location, Error>> errors;

    const auto& configuration = sourceProfile.getSourceGeneratorConfiguration();

    // Checks the extensions.
    checkExtension(errors, configuration.getSourceFileExtension(), Location::asmExtension);
    checkExtension(errors, configuration.getBinaryFileExtension(), Location::binExtension);

    // Checks the tabulation.
    if ((configuration.getTabulationLength() < minimumTabulationLength) || (configuration.getTabulationLength() > maximumTabulationLength)) {
        errors.emplace_back(Location::tabulationSize, Error::invalidValue);
    }

    // Checks the declarations.
    checkDeclaration(errors, configuration.getAddressChangeDeclaration(), Location::changeAddressLocationDeclaration);
    checkDeclaration(errors, configuration.getByteDeclaration(), Location::byteDeclaration);
    checkDeclaration(errors, configuration.getWordDeclaration(), Location::wordDeclaration);
    checkDeclaration(errors, configuration.getAddressDeclaration(), Location::addressDeclaration);
    checkDeclaration(errors, configuration.getStringDeclaration(), Location::stringDeclaration);
    checkDeclaration(errors, configuration.getConstantDeclaration(), Location::constantDeclaration);
    checkDeclaration(errors, configuration.getLabelDeclaration(), Location::labelDeclaration);
    checkDeclaration(errors, configuration.getCommentDeclaration(), Location::commentDeclaration);

    return errors;
}

void SourceProfileValidator::checkExtension(std::vector<std::pair<Location, Error>>& errorsToFill, const juce::String& extension,
                                            Location location) noexcept
{
    if (extension.trim().isEmpty()) {
        errorsToFill.emplace_back(location, Error::empty);
    } else if (extension.startsWith(".")) {
        errorsToFill.emplace_back(location, Error::mustNotStartWithDot);
    }
}

void SourceProfileValidator::checkDeclaration(std::vector<std::pair<Location, Error>>& errorsToFill, const juce::String& declaration,
                                              Location location) noexcept
{
    const auto placeholder = SourceGeneratorConfiguration::placeholder;
    const auto firstPlaceholderIndex = declaration.indexOf(placeholder);
    const auto lastPlaceholderIndex = declaration.lastIndexOf(placeholder);

    if (firstPlaceholderIndex < 0) {
        errorsToFill.emplace_back(location, Error::missingPlaceholder);
    } else if (firstPlaceholderIndex != lastPlaceholderIndex) {
        errorsToFill.emplace_back(location, Error::tooManyPlaceholders);
    }
}

int SourceProfileValidator::correctTabulationSize(const int tabulationSize) noexcept
{
    return NumberUtil::correctNumber(tabulationSize, minimumTabulationLength, maximumTabulationLength);
}

juce::String SourceProfileValidator::errorToDisplayableError(const Error error) noexcept
{
    switch (error) {
        case Error::empty:
            return juce::translate("The field must not be empty.");
        case Error::invalidValue:
            return juce::translate("The value is invalid.");
        case Error::mustNotStartWithDot:
            return juce::translate("Remove the '.' at the beginning.");
        case Error::missingPlaceholder:
            return juce::translate("The placeholder " + SourceGeneratorConfiguration::placeholder + " must be present.");
        case Error::tooManyPlaceholders:
            return juce::translate("Only one placeholder must be present.");
        default:
            jassertfalse;               // Unknown error??
            return juce::translate("Unknown error. Please report this to the author of Arkos Tracker!");
    }
}

juce::String SourceProfileValidator::locationToDisplayableLocation(const Location location) noexcept
{
    switch (location) {
        case Location::tabulationSize:
            return juce::translate("tabulation size");
        case Location::changeAddressLocationDeclaration:
            return juce::translate("current address declaration");
        case Location::byteDeclaration:
            return juce::translate("byte declaration");
        case Location::wordDeclaration:
            return juce::translate("word declaration");
        case Location::addressDeclaration:
            return juce::translate("address declaration");
        case Location::stringDeclaration:
            return juce::translate("string declaration");
        case Location::constantDeclaration:
            return juce::translate("constant declaration");
        case Location::labelDeclaration:
            return juce::translate("label declaration");
        case Location::commentDeclaration:
            return juce::translate("comment declaration");
        case Location::asmExtension:
            return juce::translate("source file extension");
        case Location::binExtension:
            return juce::translate("binary file extension");
        default:
            jassertfalse;               // Unknown location??
            return juce::translate("Unknown location. Please report this to the author of Arkos Tracker!");
    }
}

}   // namespace arkostracker

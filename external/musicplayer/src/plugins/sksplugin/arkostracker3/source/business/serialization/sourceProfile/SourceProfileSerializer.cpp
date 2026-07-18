#include "SourceProfileSerializer.h"

#include "../../../utils/XmlHelper.h"
#include "SourceProfileXmlNodes.h"

namespace arkostracker 
{

std::unique_ptr<juce::XmlElement> SourceProfileSerializer::serialize(const std::vector<SourceProfile>& sourceProfiles) noexcept
{
    auto sourceProfilesRoot = std::make_unique<juce::XmlElement>(SourceProfileXmlNodes::sourceProfilesRoot);

    for (const auto& sourceProfile : sourceProfiles) {
        auto sourceProfileXml = serialize(sourceProfile);
        jassert(sourceProfileXml != nullptr);                           // Should never happen!
        sourceProfilesRoot->addChildElement(sourceProfileXml.release());        // Ownership to the new root.
    }

    return sourceProfilesRoot;
}

std::unique_ptr<juce::XmlElement> SourceProfileSerializer::serialize(const SourceProfile& sourceProfile) noexcept
{
    auto sourceProfileRoot = std::make_unique<juce::XmlElement>(SourceProfileXmlNodes::sourceProfileRoot);

    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::name, sourceProfile.getName());
    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::readOnly, sourceProfile.isReadOnly());
    const auto& configuration = sourceProfile.getSourceGeneratorConfiguration();
    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::littleEndian, configuration.isLittleEndian());
    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::generateComments, configuration.doesGenerateComments());
    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::commentDeclaration, configuration.getCommentDeclaration());
    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::addressChangeDeclaration, configuration.getAddressChangeDeclaration());
    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::addressDeclaration, configuration.getAddressDeclaration());
    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::byteDeclaration, configuration.getByteDeclaration());
    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::wordDeclaration, configuration.getWordDeclaration());
    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::labelDeclaration, configuration.getLabelDeclaration());
    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::stringDeclaration, configuration.getStringDeclaration());
    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::constantDeclaration, configuration.getConstantDeclaration());
    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::binaryFileExtension, configuration.getBinaryFileExtension());
    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::sourceFileExtension, configuration.getSourceFileExtension());
    XmlHelper::addValueNode(*sourceProfileRoot, SourceProfileXmlNodes::tabulationLength, configuration.getTabulationLength());

    return sourceProfileRoot;
}

std::vector<SourceProfile> SourceProfileSerializer::deserialize(const juce::XmlElement& sourceProfilesNode) noexcept
{
    std::vector<SourceProfile> sourceProfiles;

    for (const auto* sourceProfileXml : XmlHelper::getChildrenList(sourceProfilesNode, SourceProfileXmlNodes::sourceProfileRoot)) {
        sourceProfiles.push_back(deserializeOne(*sourceProfileXml));
    }

    return sourceProfiles;
}

SourceProfile SourceProfileSerializer::deserializeOne(const juce::XmlElement& sourceProfileNode) noexcept
{
    const auto name = XmlHelper::readString(sourceProfileNode, SourceProfileXmlNodes::name);
    const auto readOnly = XmlHelper::readBool(sourceProfileNode, SourceProfileXmlNodes::readOnly);
    const auto littleEndian = XmlHelper::readBool(sourceProfileNode, SourceProfileXmlNodes::littleEndian);
    const auto generateComments = XmlHelper::readBool(sourceProfileNode, SourceProfileXmlNodes::generateComments);
    const auto commentDeclaration = XmlHelper::readString(sourceProfileNode, SourceProfileXmlNodes::commentDeclaration);
    const auto addressChangeDeclaration = XmlHelper::readString(sourceProfileNode, SourceProfileXmlNodes::addressChangeDeclaration);
    const auto addressDeclaration = XmlHelper::readString(sourceProfileNode, SourceProfileXmlNodes::addressDeclaration);
    const auto byteDeclaration = XmlHelper::readString(sourceProfileNode, SourceProfileXmlNodes::byteDeclaration);
    const auto wordDeclaration = XmlHelper::readString(sourceProfileNode, SourceProfileXmlNodes::wordDeclaration);
    const auto labelDeclaration = XmlHelper::readString(sourceProfileNode, SourceProfileXmlNodes::labelDeclaration);
    const auto stringDeclaration = XmlHelper::readString(sourceProfileNode, SourceProfileXmlNodes::stringDeclaration);
    auto constantDeclaration = XmlHelper::readString(sourceProfileNode, SourceProfileXmlNodes::constantDeclaration);
    if (constantDeclaration.isEmpty()) {        // Could be empty, because was added later!
        constantDeclaration = SourceGeneratorConfiguration::defaultConstantPlaceholder;
    }
    const auto binaryFileExtension = XmlHelper::readString(sourceProfileNode, SourceProfileXmlNodes::binaryFileExtension);
    const auto sourceFileExtension = XmlHelper::readString(sourceProfileNode, SourceProfileXmlNodes::sourceFileExtension);
    const auto tabulationLength = XmlHelper::readInt(sourceProfileNode, SourceProfileXmlNodes::tabulationLength);

    const SourceGeneratorConfiguration configuration(addressChangeDeclaration, commentDeclaration, generateComments,
                                               byteDeclaration, wordDeclaration, addressDeclaration,
                                               stringDeclaration, constantDeclaration, labelDeclaration, littleEndian,
                                               sourceFileExtension, binaryFileExtension, tabulationLength);
    return { configuration, name, readOnly };
}

}   // namespace arkostracker

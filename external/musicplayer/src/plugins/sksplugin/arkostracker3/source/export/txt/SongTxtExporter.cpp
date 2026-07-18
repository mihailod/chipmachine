#include "SongTxtExporter.h"

#include "../../song/Song.h"
#include "../../utils/StreamUtil.h"
#include "../../utils/XmlHelper.h"
#include "../at3/SongExporter.h"

namespace arkostracker
{

const juce::String SongTxtExporter::firstLineHeader = "Arkos Tracker text format V1.0";             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongTxtExporter::sectionBase = "SECTION ";                                       // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongTxtExporter::endSectionBase = "ENDSECTION ";                                 // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongTxtExporter::sectionIndentation = "-";                                       // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SongTxtExporter::keyValueSeparator = " ";                                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

SongTxtExporter::SongTxtExporter(const Song& pSong, std::unique_ptr<juce::OutputStream> pOutputStream) noexcept :
        song(pSong),
        outputStream(std::move(pOutputStream))
{
}

std::pair<bool, std::unique_ptr<bool>> SongTxtExporter::performTask() noexcept
{
    const auto success = exportSong();
    return { success, std::make_unique<bool>(success) };
}

bool SongTxtExporter::exportSong() noexcept
{
    // Instead of re-doing the XML job, simply uses it and produces TXT instead.
    const auto xmlRootNode = SongExporter::exportSong(song);
    if (xmlRootNode == nullptr) {
        jassertfalse;
        return false;
    }

    // Format header.
    write(firstLineHeader);

    encodeNode(*xmlRootNode, 0);

    return true;
}

void SongTxtExporter::writeKeyValues(const juce::XmlElement& node) const noexcept
{
    for (const auto& [key, value] : XmlHelper::readTextOnlyNodes(node)) {
        write(key, value);
    }
}

void SongTxtExporter::write(const juce::String& line) const noexcept
{
    StreamUtil::write(*outputStream, line);
}

void SongTxtExporter::write(const juce::String& key, const juce::String& value) const noexcept
{
    StreamUtil::write(*outputStream, key + keyValueSeparator + value);
}

void SongTxtExporter::write(const juce::String& key, const juce::int64 value) const noexcept
{
    StreamUtil::write(*outputStream, key + keyValueSeparator + juce::String(value));
}

void SongTxtExporter::writeSection(const juce::String& sectionName, const int indentationLevel, const bool isEndSection) const noexcept
{
    for (auto indentationIndex = 0; indentationIndex < indentationLevel; ++indentationIndex) {
        StreamUtil::write(*outputStream, sectionIndentation, false);
    }
    StreamUtil::write(*outputStream, (isEndSection ? endSectionBase : sectionBase) + sectionName);
}

void SongTxtExporter::writeEmptyLine() const noexcept
{
    StreamUtil::write(*outputStream, juce::String());
}

void SongTxtExporter::encodeNode(const juce::XmlElement& rootNode, const int indentationLevel) noexcept     // NOLINT(*-no-recursion)
{
    // Recursive method.
    writeEmptyLine();
    writeSection(rootNode.getTagName(), indentationLevel, false);
    writeKeyValues(rootNode);

    for (auto childIndex = 0, childCount = rootNode.getNumChildElements(); childIndex < childCount; ++childIndex) {
        const auto* childNode = rootNode.getChildElement(childIndex);
        if (!XmlHelper::hasOnlyText(*childNode)) {
            encodeNode(*childNode, indentationLevel + 1);
        }
    }

    writeSection(rootNode.getTagName(), indentationLevel, true);
}

}   // namespace arkostracker

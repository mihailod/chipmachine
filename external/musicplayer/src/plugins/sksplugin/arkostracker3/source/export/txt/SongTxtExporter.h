#pragma once

#include <juce_core/juce_core.h>

#include "../../utils/task/Task.h"

namespace arkostracker
{

class Song;

/**
 * Exports the Song as a human-readable text. The nodes are the same as in XML.
 */
class SongTxtExporter final : public Task<std::unique_ptr<bool>>
{
public:
    /**
     * Constructor.
     * @param song the Song. No need to optimize it.
     * @param outputStream where to write the data.
     */
    SongTxtExporter(const Song& song, std::unique_ptr<juce::OutputStream> outputStream) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<bool>> performTask() noexcept override;

private:
    static const juce::String firstLineHeader;
    static const juce::String sectionBase;
    static const juce::String endSectionBase;
    static const juce::String sectionIndentation;

    static const juce::String keyValueSeparator;

    /**
     * Exports a Song to a human-readable text.
     * @return true if the export was successful.
     */
    bool exportSong() noexcept;

    /** Writes a line. */
    void write(const juce::String& line) const noexcept;
    /** Writes a key/value. */
    void write(const juce::String& key, const juce::String& value) const noexcept;
    /** Writes a key/value (int). */
    void write(const juce::String& key, juce::int64 value) const noexcept;

    /**
     * Writes a section (only the header).
     * @param sectionName the name of the section. Prefix will be added.
     * @param indentationLevel the indentation level on the section (>=0).
     * @param isEndSection true if end section, false if start section.
     */
    void writeSection(const juce::String& sectionName, int indentationLevel, bool isEndSection) const noexcept;
    /** Writes an empty line. */
    void writeEmptyLine() const noexcept;

    /** Writes key/values read in the given node. Only simple texts are retrieved. */
    void writeKeyValues(const juce::XmlElement& node) const noexcept;

    /**
     * Encodes the given node, recursively.
     * @param rootNode the node to explore. It generates a section.
     * @param indentationLevel the indentation level on the section (>=0).
     */
    void encodeNode(const juce::XmlElement& rootNode, int indentationLevel) noexcept;

    const Song& song;
    std::unique_ptr<juce::OutputStream> outputStream;
};

}   // namespace arkostracker

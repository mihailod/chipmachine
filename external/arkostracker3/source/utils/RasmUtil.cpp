#include "RasmUtil.h"

namespace arkostracker
{

juce::MemoryBlock RasmUtil::commentIncludes(const juce::MemoryBlock& source) noexcept
{
    juce::MemoryInputStream playerSourceInputStream(source, false);

    // Reads all the lines, as long as we can.
    juce::MemoryOutputStream playerSourceOutputStreamWithoutInclude;
    while (!playerSourceInputStream.isExhausted()) {
        auto line = playerSourceInputStream.readNextLine();       // This excludes the end of lines, we have to add them below.
        if (line.containsIgnoreCase("include \"") || line.containsIgnoreCase("include\"")) {
            line = ";" + line;
        }
        playerSourceOutputStreamWithoutInclude << line << "\r\n";
    }

    return playerSourceOutputStreamWithoutInclude.getMemoryBlock();
}

void RasmUtil::addVariableDeclaration(juce::MemoryBlock& source, const juce::String& variable, const juce::String& value) noexcept
{
    const auto lineToInsert = juce::String("    ") + variable + " = " + value + "\r\n";
    source.insert(lineToInsert.toRawUTF8(), static_cast<size_t>(lineToInsert.length()), 0);
}

}       // namespace arkostracker

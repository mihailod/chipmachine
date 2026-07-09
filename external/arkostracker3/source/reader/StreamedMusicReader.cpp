#include "StreamedMusicReader.h"

namespace arkostracker 
{

StreamedMusicReader::StreamedMusicReader(juce::InputStream& pInputStream) noexcept :
        inputStream(pInputStream)
{
}

juce::InputStream& StreamedMusicReader::getInputStream() const noexcept
{
    return inputStream;
}

}   // namespace arkostracker

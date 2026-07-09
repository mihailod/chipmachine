#include "Sample.h"

namespace arkostracker
{

Sample::Sample(juce::MemoryBlock pData) noexcept :
        data(std::move(pData)),
        uuid()
{
    jassert(data.getSize() <= static_cast<size_t>(maximumLength));     // Sample should be 128k max!
}

int Sample::getLength() const noexcept
{
    return static_cast<int>(data.getSize());
}

const juce::MemoryBlock& Sample::getData() const noexcept
{
    return data;
}

unsigned char Sample::operator[](const int offset) const noexcept
{
    if (static_cast<size_t>(offset) >= data.getSize()) {
        jassertfalse;
        return 0U;
    }
    return static_cast<unsigned char>(data[offset]);
}

bool Sample::operator==(const Sample& other) const noexcept
{
    if (uuid == other.uuid) {
        return true;
    }
    // The UUID may be different, but the data the same.
    return (data == other.data);
}

bool Sample::operator!=(const Sample& other) const noexcept
{
    return !(other == *this);
}

}   // namespace arkostracker

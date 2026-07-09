#include "KeysUtil.h"

#include <unordered_map>

namespace arkostracker 
{

OptionalInt KeysUtil::getHexNumberFromKey(const juce::KeyPress& key) noexcept
{
    static const std::unordered_map<juce::juce_wchar, int> keyToNumber = {
            { '0', 0 },
            { '1', 1 },
            { '2', 2 },
            { '3', 3 },
            { '4', 4 },
            { '5', 5 },
            { '6', 6 },
            { '7', 7 },
            { '8', 8 },
            { '9', 9 },
            { 'A', 10 },
            { 'a', 10 },
            { 'B', 11 },
            { 'b', 11 },
            { 'C', 12 },
            { 'c', 12 },
            { 'D', 13 },
            { 'd', 13 },
            { 'E', 14 },
            { 'e', 14 },
            { 'F', 15 },
            { 'f', 15 },
    };

    auto iterator = keyToNumber.find(key.getTextCharacter());
    if (iterator == keyToNumber.cend()) {
        return { };
    }

    return iterator->second;
}


}   // namespace arkostracker


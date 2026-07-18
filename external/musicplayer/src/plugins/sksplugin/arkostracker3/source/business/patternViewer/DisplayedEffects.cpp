#include "DisplayedEffects.h"

namespace arkostracker 
{

const std::unordered_map<Effect, juce::juce_wchar>& DisplayedEffects::getDefaultEffectToDisplayedChar()
{
    static const std::unordered_map<Effect, juce::juce_wchar> defaultEffectToDisplayedChar = {
        {Effect::pitchUp,              'u'},
        {Effect::pitchDown,            'd'},
        {Effect::pitchGlide,           'g'},
        {Effect::pitchTable,           'p'},

        {Effect::volume,               'v'},
        {Effect::volumeIn,             'i'},
        {Effect::volumeOut,            'o'},

        {Effect::arpeggioTable,        'a'},
        {Effect::arpeggio3Notes,       'b'},
        {Effect::arpeggio4Notes,       'c'},

        {Effect::reset,                'r'},

        {Effect::forceInstrumentSpeed, 's'},
        {Effect::forceArpeggioSpeed,   'w'},
        {Effect::forcePitchTableSpeed, 'x'},

        {Effect::fastPitchUp,          'e'},
        {Effect::fastPitchDown,        'f'},
    };

    return defaultEffectToDisplayedChar;
}


}   // namespace arkostracker


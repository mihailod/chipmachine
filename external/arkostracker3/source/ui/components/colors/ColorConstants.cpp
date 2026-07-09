#include "ColorConstants.h"

namespace arkostracker 
{

const juce::uint32 ColorConstants::defaultColorAsArgb(0xffe0e0e0);
const int ColorConstants::colorPackCount = 6;

const std::vector<juce::uint32> ColorConstants::colors = {                    // NOLINT(fuchsia-statically-constructed-objects,cert-err58-cpp)
        // White.
        0xff808080,
        0xff909090,
        0xffa0a0a0,
        0xffc0c0c0,
        defaultColorAsArgb,
        0xffffffff,
        // Blue.
        0xff1077aa,
        0xff3088bb,
        0xff5099cc,
        0xff70aadd,
        0xff90bbee,
        0xffb0ccff,
        // Green.
        0xff10aa20,
        0xff30bb30,
        0xff50cc40,
        0xff70dd50,
        0xff90ee70,
        0xffb0ff90,
        // Red.
        0xffaa1010,
        0xffbb3030,
        0xffcc5050,
        0xffdd7070,
        0xffee9090,
        0xffffb0b0,
        // Yellow.
        0xffaaaa20,
        0xffbbbb40,
        0xffcccc60,
        0xffdddd80,
        0xffeeeea0,
        0xffffffc0,
        // Pink.
        0xffaa20aa,
        0xffbb40bb,
        0xffcc60cc,
        0xffdd80dd,
        0xffeea0ee,
        0xffffc0ff,
};

}   // namespace arkostracker

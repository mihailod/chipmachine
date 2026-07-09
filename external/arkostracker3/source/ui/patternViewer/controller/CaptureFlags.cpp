#include "CaptureFlags.h"

namespace arkostracker
{

CaptureFlags::CaptureFlags(const bool isNoteCaptured, const bool isInstrumentCaptured, const bool isEffect1Captured, const bool isEffect2Captured, const bool isEffect3Captured,
                           const bool isEffect4Captured) noexcept :
        noteCaptured(isNoteCaptured),
        instrumentCaptured(isInstrumentCaptured),
        effect1Captured(isEffect1Captured),
        effect2Captured(isEffect2Captured),
        effect3Captured(isEffect3Captured),
        effect4Captured(isEffect4Captured)
{
}

bool CaptureFlags::isNoteCaptured() const noexcept
{
    return noteCaptured;
}

bool CaptureFlags::isInstrumentCaptured() const noexcept
{
    return instrumentCaptured;
}

bool CaptureFlags::isEffect1Captured() const noexcept
{
    return effect1Captured;
}

bool CaptureFlags::isEffect2Captured() const noexcept
{
    return effect2Captured;
}

bool CaptureFlags::isEffect3Captured() const noexcept
{
    return effect3Captured;
}

bool CaptureFlags::isEffect4Captured() const noexcept
{
    return effect4Captured;
}

bool CaptureFlags::isAllCaptured() const noexcept
{
    return (noteCaptured && instrumentCaptured && effect1Captured && effect2Captured && effect3Captured && effect4Captured);
}

CaptureFlags CaptureFlags::withEffectShift(const int shift) const noexcept
{
    // Not the most elegant code...
    switch (shift) {
        case 0:
            return *this;
        case 1:
            return { noteCaptured, instrumentCaptured, false, effect1Captured, effect2Captured, effect3Captured };
        case 2:
            return { noteCaptured, instrumentCaptured, false, false, effect1Captured, effect2Captured };
        case 3:
            return { noteCaptured, instrumentCaptured, false, false, false, effect1Captured };
        case -1:
            return { noteCaptured, instrumentCaptured, effect2Captured, effect3Captured, effect4Captured, false };
        case -2:
            return { noteCaptured, instrumentCaptured, effect3Captured, effect4Captured, false, false };
        case -3:
            return { noteCaptured, instrumentCaptured, effect4Captured, false, false, false };
        default:
            // Any other shift will clear the effects.
            return { noteCaptured, instrumentCaptured, false, false, false, false };
    }
}

CaptureFlags CaptureFlags::withNoteCaptured(bool captured) const noexcept
{
    return { captured, instrumentCaptured, effect1Captured, effect2Captured, effect3Captured, effect4Captured };
}

CaptureFlags CaptureFlags::withNoteAndInstrumentCaptured(bool pNoteCaptured, bool pNewInstrumentCaptured) const noexcept
{
    return { pNoteCaptured, pNewInstrumentCaptured, effect1Captured, effect2Captured, effect3Captured, effect4Captured };
}

}   // namespace arkostracker

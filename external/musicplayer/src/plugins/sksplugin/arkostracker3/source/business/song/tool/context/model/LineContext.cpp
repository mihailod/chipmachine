#include "LineContext.h"

#include "../../../../../utils/PsgValues.h"

namespace arkostracker
{

LineContext::LineContext() noexcept :
        pitchId(),
        arpeggioId(),
        inlineArpeggio(),
        volume(PsgValues::maximumVolumeNoHard)
{
}

LineContext::LineContext(OptionalId pPitchId, OptionalId pArpeggioId, const int pVolume) noexcept :
           pitchId(std::move(pPitchId)),
           arpeggioId(std::move(pArpeggioId)),
           inlineArpeggio(),
           volume(static_cast<uint16_t>(pVolume))
{
}

LineContext LineContext::withArpeggioId(const OptionalId& newArpeggioId) const noexcept
{
    auto copy = *this;
    copy.arpeggioId = newArpeggioId;
    return copy;
}

LineContext LineContext::withPitchId(const OptionalId& newPitchId) const noexcept
{
    auto copy = *this;
    copy.pitchId = newPitchId;
    return copy;
}

LineContext LineContext::withArpeggioInline(const OptionalValue<Expression>& newExpression) const noexcept
{
    auto copy = *this;
    copy.inlineArpeggio = newExpression;
    return copy;
}

LineContext LineContext::withVolume(const int newVolume, const int decimalPart) const noexcept
{
    auto copy = *this;
    copy.volume = FpFloat(static_cast<uint16_t>(newVolume), decimalPart);
    return copy;
}

void LineContext::setPitchId(const Id& newPitchId) noexcept
{
    pitchId = newPitchId;
}

OptionalId LineContext::getPitchId() const noexcept
{
    return pitchId;
}

OptionalId LineContext::getArpeggioId() const noexcept
{
    return arpeggioId;
}

OptionalValue<Expression> LineContext::getArpeggioInline() const noexcept
{
    return inlineArpeggio;
}

FpFloat LineContext::getVolume() const noexcept
{
    return volume;
}

void LineContext::setArpeggioIdAndResetInline(const Id& newArpeggioId) noexcept
{
    arpeggioId = newArpeggioId;
    inlineArpeggio = { };
}

void LineContext::setInlineArpeggioAndResetArpeggioTable(const Expression& newInlineArpeggio) noexcept
{
    inlineArpeggio = newInlineArpeggio;
    arpeggioId = { };
}

void LineContext::setVolume(const FpFloat& newVolume) noexcept
{
    jassert(newVolume.getIntegerPart() <= PsgValues::maximumVolumeNoHard);
    volume = newVolume;
}

bool LineContext::operator==(const LineContext& other) const noexcept
{
    return pitchId == other.pitchId
        && arpeggioId == other.arpeggioId
        && inlineArpeggio == other.inlineArpeggio
        && volume == other.volume;
}

bool LineContext::operator!=(const LineContext& other) const noexcept
{
    return !(*this == other);
}

bool LineContext::isFull() const noexcept
{
    return pitchId.isPresent()
        && (arpeggioId.isPresent() || getArpeggioInline().isPresent());
}

}   // namespace arkostracker

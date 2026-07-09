#include "BarDataCreator.h"

#include <BinaryData.h>

#include "../../../../song/cells/CellConstants.h"
#include "../../../../song/instrument/psg/PsgInstrumentCell.h"
#include "../../../../utils/NumberUtil.h"
#include "../../../../utils/PsgValues.h"
#include "ValueInterpreter.h"
#include "ViewedValueLimits.h"

namespace arkostracker
{

const juce::Image BarDataCreator::emptyImage = juce::Image();    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

BarAndCaptionData BarDataCreator::createLinkBarData(const PsgInstrumentCell& cell, const bool withinLoop, const bool outOfBounds,
                                                    const bool generated, const bool hovered, const bool selected, const bool withCursor) noexcept
{
    const auto link = cell.getLink();

    const auto [linkMinimumValue, linkMaximumValue] = getSoundTypeMinMax();
    constexpr auto linkBackgroundColorId = LookAndFeelConstants::Colors::barSoundTypeBackground;
    LookAndFeelConstants::Colors linkColorId;    // NOLINT(*-init-variables)
    juce::Image image;
    switch (link) {
        default:
            jassertfalse;           // Shouldn't happen!
        case PsgInstrumentCellLink::noSoftNoHard:
            linkColorId = LookAndFeelConstants::Colors::barSoundTypeNoSoftNoHard;
            image = juce::ImageFileFormat::loadFrom(BinaryData::LinkNoSoftNoHard_png, BinaryData::LinkNoSoftNoHard_pngSize);
            break;
        case PsgInstrumentCellLink::softOnly:
            linkColorId = LookAndFeelConstants::Colors::barSoundTypeSoft;
            image = juce::ImageFileFormat::loadFrom(BinaryData::LinkSoftOnly_png, BinaryData::LinkSoftOnly_pngSize);
            break;
        case PsgInstrumentCellLink::hardOnly:
            linkColorId = LookAndFeelConstants::Colors::barSoundTypeHard;
            image = juce::ImageFileFormat::loadFrom(BinaryData::LinkHardOnly_png, BinaryData::LinkHardOnly_pngSize);
            break;
        case PsgInstrumentCellLink::softToHard:
            linkColorId = LookAndFeelConstants::Colors::barSoundTypeSoftToHard;
            image = juce::ImageFileFormat::loadFrom(BinaryData::LinkSoftToHard_png, BinaryData::LinkSoftToHard_pngSize);
            break;
        case PsgInstrumentCellLink::hardToSoft:
            linkColorId = LookAndFeelConstants::Colors::barSoundTypeHardToSoft;
            image = juce::ImageFileFormat::loadFrom(BinaryData::LinkHardToSoft_png, BinaryData::LinkHardToSoft_pngSize);
            break;
        case PsgInstrumentCellLink::softAndHard:
            linkColorId = LookAndFeelConstants::Colors::barSoundTypeSoftAndHard;
            image = juce::ImageFileFormat::loadFrom(BinaryData::LinkSoftAndHard_png, BinaryData::LinkSoftAndHard_pngSize);
            break;
    }

    const auto linkValue = ValueInterpreter::getLinkToBarValue(link);
    constexpr auto ignored = false;     // The Link cannot be ignored.
    constexpr auto retrig = false;

    const auto barData = BarData(true, linkMinimumValue, linkMaximumValue,
                                 linkValue, linkBackgroundColorId, linkColorId, withCursor, selected,
                                 hovered, withinLoop, outOfBounds, generated, ignored, retrig);
    const auto captionData = BarCaptionDisplayedData(image, juce::String(), ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData};
}

BarAndCaptionData BarDataCreator::createEnvelopeBarData(const PsgInstrumentCell& cell, const BarAreaSize areaSize, const bool withinLoop,
                                                        const bool outOfBounds, const bool generated,
                                                        const bool hovered, const bool selected, const bool withCursor) noexcept
{
    const auto isHardware = cell.isHardware();
    const auto retrig = isHardware && cell.isRetrig();          // Only shows Retrig for hardware.
    const auto value = ValueInterpreter::determineEnvelopeUiValueFromCell(cell);

    const auto [minimumVisibleValue, maximumVisibleValue] = getEnvelopeMinMax(areaSize, isHardware);

    const auto hardwareEnvelope = cell.getHardwareEnvelope();
    constexpr auto ignored = false;                     // Envelope cannot be ignored.
    constexpr auto backgroundColorId = LookAndFeelConstants::Colors::barEnvelopeBackground;
    const auto colorId = isHardware ? LookAndFeelConstants::Colors::barEnvelopeHard : LookAndFeelConstants::Colors::barEnvelopeSoft;

    // Caption.
    auto mainText = isHardware ? juce::String() : NumberUtil::toHexDigit(value);
    juce::Image image;
    if (isHardware) {
        // Links the envelope to an image.
        static const std::unordered_map<int, std::pair<const char*, const int>> envelopeToImage = {
                { 8, { BinaryData::HardwareEnvelope8_png, BinaryData::HardwareEnvelope8_pngSize } },
                { 9, { BinaryData::HardwareEnvelope9_png, BinaryData::HardwareEnvelope9_pngSize } },
                { 0xa, { BinaryData::HardwareEnvelopeA_png, BinaryData::HardwareEnvelopeA_pngSize } },
                { 0xb, { BinaryData::HardwareEnvelopeB_png, BinaryData::HardwareEnvelopeB_pngSize } },
                { 0xc, { BinaryData::HardwareEnvelopeC_png, BinaryData::HardwareEnvelopeC_pngSize } },
                { 0xd, { BinaryData::HardwareEnvelopeD_png, BinaryData::HardwareEnvelopeD_pngSize } },
                { 0xe, { BinaryData::HardwareEnvelopeE_png, BinaryData::HardwareEnvelopeE_pngSize } },
                { 0xf, { BinaryData::HardwareEnvelopeF_png, BinaryData::HardwareEnvelopeF_pngSize } }
        };
        const auto iterator = envelopeToImage.find(hardwareEnvelope);
        if (iterator == envelopeToImage.cend()) {
            jassertfalse;       // Should never happen!
        } else {
            const auto imageData = iterator->second;
            image = juce::ImageFileFormat::loadFrom(imageData.first, static_cast<size_t>(imageData.second));
        }
        // Secondary text.
        // Ratio is only shown if useful: soft to hard and hard to soft.
        const auto isRatioUseful = cell.isRatioUseful();
        mainText = isRatioUseful ? (juce::translate("r") + juce::String(cell.getRatio())) : juce::String();
    }

    auto barData = BarData(false, minimumVisibleValue, maximumVisibleValue, value,
                           backgroundColorId, colorId, withCursor,
                           selected, hovered, withinLoop, outOfBounds, generated, ignored, retrig);
    auto captionData = BarCaptionDisplayedData(image, mainText, ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData };
}

BarAndCaptionData BarDataCreator::createNoiseBarData(const PsgInstrumentCell& cell, const bool withinLoop, const bool outOfBounds, const bool generated, const bool hovered,
                                                     const bool selected, const bool withCursor) noexcept
{
    constexpr auto backgroundColorId = LookAndFeelConstants::Colors::barNoiseBackground;
    constexpr auto colorId = LookAndFeelConstants::Colors::barNoise;

    const auto [minimumValue, maximumValue] = getNoiseMinMax();

    constexpr auto ignored = false;         // Noise is never ignored.
    constexpr auto retrig = false;
    const auto value = cell.getNoise();

    const auto barData = BarData(false, minimumValue, maximumValue,
                                 value, backgroundColorId, colorId, withCursor, selected, hovered, withinLoop, outOfBounds, generated, ignored, retrig);

    const auto mainText = (value == 0) ? juce::String() : NumberUtil::toUnsignedHex(value);
    const auto captionData = BarCaptionDisplayedData(emptyImage, mainText, ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData };
}


// ----------------------------------------------------

BarAndCaptionData BarDataCreator::createPrimaryPeriodBarData(const PsgInstrumentCell& cell, const BarAreaSize areaSize, const bool withinLoop, const bool outOfBounds,
                                                             const bool generated, const bool hovered, const bool selected, const bool withCursor) noexcept
{
    return createPeriodBarData(cell, true, areaSize, withinLoop, outOfBounds, generated, hovered, selected, withCursor);
}

BarAndCaptionData BarDataCreator::createPrimaryArpeggioNoteInOctaveBarData(const PsgInstrumentCell& cell, const bool withinLoop, const bool outOfBounds, const bool generated, const bool hovered,
                                                                           const bool selected, const bool withCursor) noexcept
{
    return createArpeggioNoteInOctaveBarData(cell, true, withinLoop, outOfBounds, generated, hovered, selected, withCursor);
}

BarAndCaptionData BarDataCreator::createPrimaryArpeggioOctaveBarData(const PsgInstrumentCell& cell, const BarAreaSize areaSize, const bool withinLoop, const bool outOfBounds, const bool generated,
                                                                     const bool hovered, const bool selected, const bool withCursor) noexcept
{
    return createArpeggioOctaveBarData(cell, true, areaSize, withinLoop, outOfBounds, generated, hovered, selected, withCursor);
}

BarAndCaptionData BarDataCreator::createPrimaryPitchBarData(const PsgInstrumentCell& cell, const BarAreaSize areaSize, const bool withinLoop, const bool outOfBounds, const bool generated, const bool hovered,
                                                            const bool selected, const bool withCursor) noexcept
{
    return createPitchBarData(cell, true, areaSize, withinLoop, outOfBounds, generated, hovered, selected, withCursor);
}


// ----------------------------------------------------

BarAndCaptionData BarDataCreator::createSecondaryPeriodBarData(const PsgInstrumentCell& cell, const BarAreaSize areaSize, const bool withinLoop, const bool outOfBounds,
                                                               const bool generated, const bool hovered, const bool selected, const bool withCursor) noexcept
{
    return createPeriodBarData(cell, false, areaSize, withinLoop, outOfBounds, generated, hovered, selected, withCursor);
}

BarAndCaptionData BarDataCreator::createSecondaryArpeggioNoteInOctaveBarData(const PsgInstrumentCell& cell, const bool withinLoop, const bool outOfBounds, const bool generated,
                                                                             const bool hovered, const bool selected, const bool withCursor) noexcept
{
    return createArpeggioNoteInOctaveBarData(cell, false, withinLoop, outOfBounds, generated, hovered, selected, withCursor);
}

BarAndCaptionData BarDataCreator::createSecondaryArpeggioOctaveBarData(const PsgInstrumentCell& cell, const BarAreaSize areaSize, const bool withinLoop, const bool outOfBounds,
                                                                       const bool generated, const bool hovered, const bool selected, const bool withCursor) noexcept
{
    return createArpeggioOctaveBarData(cell, false, areaSize, withinLoop, outOfBounds, generated, hovered, selected, withCursor);
}

BarAndCaptionData BarDataCreator::createSecondaryPitchBarData(const PsgInstrumentCell& cell, const BarAreaSize areaSize, const bool withinLoop, const bool outOfBounds, const bool generated, const bool hovered,
                                                              const bool selected, const bool withCursor) noexcept
{
    return createPitchBarData(cell, false, areaSize, withinLoop, outOfBounds, generated, hovered, selected, withCursor);
}

BarAndCaptionData BarDataCreator::createSidIsActivatedAndRatioBarData(const PsgInstrumentCell& cell, BarAreaSize /*areaSize*/, const bool withinLoop,
    const bool outOfBounds, const bool generated, const bool hovered, const bool selected, const bool withCursor) noexcept
{
    const auto isActivated = cell.isSidActivated();
    const auto ratio = cell.getSidRatio();

    // Used already defined colors.
    constexpr auto backgroundColorId = LookAndFeelConstants::Colors::barSoundTypeBackground;
    constexpr auto colorId = LookAndFeelConstants::Colors::barSoundTypeNoSoftNoHard;

    constexpr auto ignored = false;
    constexpr auto retrig = false;

    const auto value = isActivated ? ratio : -1;

    const auto barData = BarData(true, -1, PsgValues::sidMaximumRatio,
                                 value, backgroundColorId, colorId, withCursor, selected, hovered, withinLoop, outOfBounds, generated, ignored, retrig);

    const auto mainText = !isActivated ? juce::translate("-") : (juce::translate("r") + juce::String(ratio));
    const auto captionData = BarCaptionDisplayedData(emptyImage, mainText, ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData };
}

BarAndCaptionData BarDataCreator::createSidBalancePercentBarData(const PsgInstrumentCell& cell, const bool withinLoop, const bool outOfBounds,
    const bool generated, const bool hovered, const bool selected, const bool withCursor) noexcept
{
    // Used already defined colors.
    constexpr auto backgroundColorId = LookAndFeelConstants::Colors::barPeriodBackground;
    constexpr auto colorId = LookAndFeelConstants::Colors::barPeriod;

    const auto [minimumValue, maximumValue] = getPercentMinMax();

    const auto ignored = !cell.isSidActivated();
    constexpr auto retrig = false;
    const auto value = cell.getSidBalancePercent();

    const auto barData = BarData(false, minimumValue, maximumValue,
                                 value, backgroundColorId, colorId, withCursor, selected, hovered, withinLoop, outOfBounds, generated, ignored, retrig);

    const auto mainText = (value == PsgValues::defaultSidBalancePercent) ? juce::String() : (juce::String(value) + "%");
    const auto captionData = BarCaptionDisplayedData(emptyImage, mainText, ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData };
}

BarAndCaptionData BarDataCreator::createSidLowShelfVolumeBarData(const PsgInstrumentCell& cell, const bool withinLoop, const bool outOfBounds, const bool generated,
    const bool hovered, const bool selected, const bool withCursor) noexcept
{
    // Used already defined colors.
    constexpr auto backgroundColorId = LookAndFeelConstants::Colors::barEnvelopeBackground;
    constexpr auto colorId = LookAndFeelConstants::Colors::barEnvelopeSoft;

    constexpr auto minimumVolume = PsgValues::minimumVolume;
    constexpr auto maximumVolume = PsgValues::maximumVolumeNoHard;

    const auto ignored = !cell.isSidActivated();
    constexpr auto retrig = false;
    const auto value = cell.getSidLowShelfVolume();

    const auto barData = BarData(false, minimumVolume, maximumVolume,
                                 value, backgroundColorId, colorId, withCursor, selected, hovered, withinLoop, outOfBounds, generated, ignored, retrig);

    const auto mainText = (value == PsgValues::defaultSidLowShelfVolume) ? juce::String() : NumberUtil::toUnsignedHex(value);
    const auto captionData = BarCaptionDisplayedData(emptyImage, mainText, ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData };
}

BarAndCaptionData BarDataCreator::createSidArpeggioBarData(const PsgInstrumentCell& cell, bool withinLoop, bool outOfBounds, bool generated, bool hovered, bool selected,
    bool withCursor) noexcept
{
    // Used already defined colors.
    constexpr auto backgroundColorId = LookAndFeelConstants::Colors::barArpeggioNoteBackground;
    constexpr auto colorId = LookAndFeelConstants::Colors::barArpeggioNote;

    constexpr auto minimumValue = PsgValues::sidMinimumArpeggio;
    constexpr auto maximumValue = PsgValues::sidMaximumArpeggio;

    const auto ignored = !cell.isSidActivated();
    constexpr auto retrig = false;
    const auto value = cell.getSidArpeggio();

    const auto barData = BarData(false, minimumValue, maximumValue,
                                 value, backgroundColorId, colorId, withCursor, selected, hovered, withinLoop, outOfBounds, generated, ignored, retrig);

    const auto mainText = (value == PsgValues::defaultSidArpeggio) ? juce::String() : NumberUtil::toUnsignedHex(value);
    const auto captionData = BarCaptionDisplayedData(emptyImage, mainText, ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData };
}

BarAndCaptionData BarDataCreator::createSidPitchBarData(const PsgInstrumentCell& cell, bool withinLoop, bool outOfBounds, bool generated, bool hovered, bool selected,
    bool withCursor) noexcept
{
    // Used already defined colors.
    constexpr auto backgroundColorId = LookAndFeelConstants::Colors::barPitchBackground;
    constexpr auto colorId = LookAndFeelConstants::Colors::barPitch;

    constexpr auto minimumValue = PsgValues::sidMinimumPitch;
    constexpr auto maximumValue = PsgValues::sidMaximumPitch;

    const auto ignored = !cell.isSidActivated();
    constexpr auto retrig = false;
    const auto value = cell.getSidPitch();

    const auto barData = BarData(false, minimumValue, maximumValue,
                                 value, backgroundColorId, colorId, withCursor, selected, hovered, withinLoop, outOfBounds, generated, ignored, retrig);

    const auto mainText = (value == PsgValues::defaultSidPitch) ? juce::String()
        : NumberUtil::signedHexToStringWithPrefix(value, juce::String(), true);      // Signed hex.
    const auto captionData = BarCaptionDisplayedData(emptyImage, mainText, ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData };
}


// ----------------------------------------------------

BarAndCaptionData BarDataCreator::createPeriodBarData(const PsgInstrumentCell& cell, const bool primary, const BarAreaSize areaSize, const bool withinLoop, const bool outOfBounds,
                                                      const bool generated, const bool hovered, const bool selected, const bool withCursor) noexcept
{
    // Reduces to mostly used values when not the size is not "all".
    const auto [minimumValue, maximumValue] = getPeriodMinMax(areaSize);

    constexpr auto backgroundColorId = LookAndFeelConstants::Colors::barPeriodBackground;
    constexpr auto colorId = LookAndFeelConstants::Colors::barPeriod;
    const auto link = cell.getLink();
    constexpr auto retrig = false;
    bool ignored;    // NOLINT(*-init-variables)
    if (primary) {
        ignored = (link == PsgInstrumentCellLink::noSoftNoHard);
    } else {
        // If secondary, period present only Soft And Hard.
        ignored = (link != PsgInstrumentCellLink::softAndHard);
    }

    const auto value = (primary ? cell.getPrimaryPeriod(): cell.getSecondaryPeriod());

    const auto barData = BarData(false, minimumValue, maximumValue,
                                 value, backgroundColorId, colorId, withCursor, selected, hovered, withinLoop, outOfBounds, generated, ignored, retrig);

    const auto mainText = (value == 0) ? juce::String() : NumberUtil::toUnsignedHex(value);
    const auto captionData = BarCaptionDisplayedData(emptyImage, mainText, ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData };
}

BarAndCaptionData BarDataCreator::createArpeggioNoteInOctaveBarData(const PsgInstrumentCell& cell, const bool primary, const bool withinLoop, const bool outOfBounds, const bool generated, const bool hovered,
                                                                    const bool selected, const bool withCursor) noexcept
{
    const auto ignored = checkIfArpeggioOrPitchIgnored(cell, primary, true);

    // Shows the note in the octave.
    const auto noteInOctave = (primary ? cell.getPrimaryArpeggioNoteInOctave() : cell.getSecondaryArpeggioNoteInOctave());
    const auto [minimumValue, maximumValue] = getArpeggioNoteInOctaveMinMax();

    constexpr auto backgroundColorId = LookAndFeelConstants::Colors::barArpeggioNoteBackground;
    constexpr auto colorId = LookAndFeelConstants::Colors::barArpeggioNote;
    constexpr auto retrig = false;

    const auto barData = BarData(false, minimumValue, maximumValue,
                                 noteInOctave, backgroundColorId, colorId, withCursor, selected, hovered, withinLoop, outOfBounds, generated, ignored, retrig);

    const auto mainText = (noteInOctave == 0) ? juce::String() : NumberUtil::toHexDigit(noteInOctave);
    const auto captionData = BarCaptionDisplayedData(emptyImage, mainText, ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData };
}

BarAndCaptionData BarDataCreator::createArpeggioOctaveBarData(const PsgInstrumentCell& cell, const bool primary, const BarAreaSize areaSize, const bool withinLoop, const bool outOfBounds,
                                                              const bool generated, const bool hovered, const bool selected, const bool withCursor) noexcept
{
    const auto ignored = checkIfArpeggioOrPitchIgnored(cell, primary, true);

    // Shows the octave only.
    const auto octave = primary ? cell.getPrimaryArpeggioOctave() : cell.getSecondaryArpeggioOctave();
    const auto [minimumValue, maximumValue] = getArpeggioOctaveMinMax(areaSize);

    constexpr auto backgroundColorId = LookAndFeelConstants::Colors::barArpeggioOctaveBackground;
    constexpr auto colorId = LookAndFeelConstants::Colors::barArpeggioOctave;
    constexpr auto retrig = false;

    const auto barData = BarData(false, minimumValue, maximumValue,
                                 octave, backgroundColorId, colorId, withCursor, selected, hovered, withinLoop, outOfBounds, generated, ignored, retrig);

    const auto mainText = (octave == 0) ? juce::String() : NumberUtil::signedHexToStringWithPrefix(octave, juce::String(), false);      // Signed hex.
    const auto captionData = BarCaptionDisplayedData(emptyImage, mainText, ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData };
}

bool BarDataCreator::checkIfArpeggioOrPitchIgnored(const PsgInstrumentCell& cell, const bool primary, const bool arpeggio) noexcept
{
    if (primary) {
        // If a primary period is forced, the primary arpeggio/pitch is ignored. Idem if the link doesn't have envelope.
        return ((cell.getLink() == PsgInstrumentCellLink::noSoftNoHard) || (cell.getPrimaryPeriod() != 0));
    }

    // Secondary.
    // Special case for secondary pitch. If x To x, it is enabled.
    if (!arpeggio &&
        (((cell.getLink() == PsgInstrumentCellLink::softToHard) || (cell.getLink() == PsgInstrumentCellLink::hardToSoft)))) {
        return false;
    }

    // The secondary arpeggio/pitch is used only if SoftAndHard, and the period must not be auto.
    return ((cell.getLink() != PsgInstrumentCellLink::softAndHard) || (cell.getSecondaryPeriod() != 0));
}

BarAndCaptionData BarDataCreator::createPitchBarData(const PsgInstrumentCell& cell, const bool primary, const BarAreaSize areaSize, const bool withinLoop, const bool outOfBounds,
                                                     const bool generated, const bool hovered, const bool selected, const bool withCursor) noexcept
{
    const auto ignored = checkIfArpeggioOrPitchIgnored(cell, primary, false);

    // This is the full pitch.
    const auto pitch = primary ? cell.getPrimaryPitch() : cell.getSecondaryPitch();
    const auto [minimumValue, maximumValue] = getPitchMinMax(areaSize);

    constexpr auto backgroundColorId = LookAndFeelConstants::Colors::barPitchBackground;
    constexpr auto colorId = LookAndFeelConstants::Colors::barPitch;
    constexpr auto retrig = false;

    const auto barData = BarData(false, minimumValue, maximumValue,
                                 pitch, backgroundColorId, colorId, withCursor, selected, hovered, withinLoop, outOfBounds, generated, ignored, retrig);

    const auto mainText = (pitch == 0) ? juce::String() : NumberUtil::signedHexToStringWithPrefix(pitch, juce::String(), true);      // Signed hex.
    const auto captionData = BarCaptionDisplayedData(emptyImage, mainText, ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData };
}


// -------------------------------------------------------

std::pair<int, int> BarDataCreator::getSoundTypeMinMax() noexcept
{
    return { 0, static_cast<int>(PsgInstrumentCellLink::last) };
}

BarAreaSize BarDataCreator::getSizeForSoundType(const PsgInstrumentCellLink /*link*/) noexcept
{
    return BarAreaSize::first;
}

std::pair<int, int> BarDataCreator::getEnvelopeMinMax(const BarAreaSize areaSize, const bool isHardware) noexcept
{
    // The envelope can be out of bounds.
    // WARNING: no map for these values. Must be in synced with the ones from the method below.
    auto minimumVisibleValue = static_cast<int>(PsgValues::minimumVolume);
    auto maximumVisibleValue = static_cast<int>(PsgValues::maximumVolumeNoHard);
    if (isHardware) {
        if (areaSize == BarAreaSize::extended) {
            minimumVisibleValue = ViewedValueLimits::hardwareEnvelopeBarMinimumAllValue;
            maximumVisibleValue = ViewedValueLimits::hardwareEnvelopeBarMaximumAllValue;
        } else {
            // Reduces only to mostly used values.
            minimumVisibleValue = ViewedValueLimits::hardwareEnvelopeBarMinimumMostUsefulValue;
            maximumVisibleValue = ViewedValueLimits::hardwareEnvelopeBarMaximumMostUsefulValue;
        }
    }

    return { minimumVisibleValue, maximumVisibleValue };
}

BarAreaSize BarDataCreator::getSizeForEnvelope(const bool isHardware, const int hardwareEnvelope, const bool forceShrinkFull)
{
    if (!isHardware) {
        return forceShrinkFull ? BarAreaSize::small : BarAreaSize::normal;
    }

    // WARNING: no map for these values. Must be in synced with the ones from the method above.
    // Peculiar values are all shown.
    if (!ValueInterpreter::isMostUsefulHardwareEnvelope(hardwareEnvelope)) {
        return BarAreaSize::extended;
    }

    // Mostly used envelopes.
    return forceShrinkFull ? BarAreaSize::small : BarAreaSize::normal;
}

std::pair<int, int> BarDataCreator::getNoiseMinMax() noexcept
{
    return { PsgValues::minimumNoise, PsgValues::maximumNoise };
}

BarAreaSize BarDataCreator::getSizeForNoise(const int /*noise*/, const bool forceShrinkFull) noexcept
{
    return forceShrinkFull ? BarAreaSize::small : BarAreaSize::normal;
}

// =============================================================

std::pair<int, int> BarDataCreator::getPercentMinMax() noexcept
{
    return { PsgValues::minimumPercent, PsgValues::maximumPercent };
}

// =============================================================

std::pair<int, int> BarDataCreator::getArpeggioNoteInOctaveMinMax() noexcept
{
    return { CellConstants::firstNoteInOctave, CellConstants::lastNoteInOctave };        // All the notes are always shown.
}

BarAreaSize BarDataCreator::getMinimumSizeForArpeggioNoteInOctave(const bool forceShrinkFull) noexcept
{
    return forceShrinkFull ? BarAreaSize::small : BarAreaSize::normal;
}


// =============================================================

std::pair<int, int> BarDataCreator::getArpeggioOctaveMinMax(const BarAreaSize areaSize) noexcept
{
    return getSizeToMinMaxValueForArpeggioOctave().find(areaSize)->second;
}

const std::map<BarAreaSize, std::pair<int, int>>& BarDataCreator::getSizeToMinMaxValueForArpeggioOctave() noexcept
{
    static const std::map<BarAreaSize, std::pair<int, int>> map = {
        { BarAreaSize::small, { ViewedValueLimits::arpeggioOctaveBarMinimumWhenSmall, ViewedValueLimits::arpeggioOctaveBarMaximumWhenSmall } },
        { BarAreaSize::normal, { PsgValues::minimumArpeggioOctave, PsgValues::maximumArpeggioOctave } },
        { BarAreaSize::extended, { PsgValues::minimumArpeggioOctave, PsgValues::maximumArpeggioOctave } },
        { BarAreaSize::allUnusual, { PsgValues::minimumArpeggioOctave, PsgValues::maximumArpeggioOctave } },
    };
    jassert(map.size() == (static_cast<int>(BarAreaSize::last) + 1 - static_cast<int>(BarAreaSize::first)));

    return map;
}

BarAreaSize BarDataCreator::getMinimumSizeForArpeggioOctave(const int octave) noexcept
{
    const auto& sizeToMinMaxValue = getSizeToMinMaxValueForArpeggioOctave();
    return determineMinimumSize(sizeToMinMaxValue, octave);
}


// =============================================================

std::pair<int, int> BarDataCreator::getPeriodMinMax(const BarAreaSize areaSize) noexcept
{
    return getSizeToMinMaxValueForPeriod().find(areaSize)->second;
}

const std::map<BarAreaSize, std::pair<int, int>>& BarDataCreator::getSizeToMinMaxValueForPeriod() noexcept
{
    static const std::map<BarAreaSize, std::pair<int, int>> map = {
        { BarAreaSize::small, { PsgValues::minimumPeriod, ViewedValueLimits::periodMaximumWhenSmall } },
        { BarAreaSize::normal, { PsgValues::minimumPeriod, ViewedValueLimits::periodMaximumWhenNormal } },
        { BarAreaSize::extended, { PsgValues::minimumPeriod, ViewedValueLimits::periodMaximumWhenExtended } },
        { BarAreaSize::allUnusual, { PsgValues::minimumPeriod, PsgValues::maximumSoftwarePeriod } },
    };
    jassert(map.size() == (static_cast<int>(BarAreaSize::last) + 1 - static_cast<int>(BarAreaSize::first)));

    return map;
}

BarAreaSize BarDataCreator::getMinimumSizeForPeriod(const int period) noexcept
{
    const auto& sizeToMinMaxValue = getSizeToMinMaxValueForPeriod();
    return determineMinimumSize(sizeToMinMaxValue, period);
}


// =============================================================

std::pair<int, int> BarDataCreator::getPitchMinMax(const BarAreaSize areaSize) noexcept
{
    return getSizeToMinMaxValueForPitch().find(areaSize)->second;
}

const std::map<BarAreaSize, std::pair<int, int>>& BarDataCreator::getSizeToMinMaxValueForPitch() noexcept
{
    static const std::map<BarAreaSize, std::pair<int, int>> map = {
        { BarAreaSize::small, { ViewedValueLimits::pitchBarMinimumWhenSmall, ViewedValueLimits::pitchBarMaximumWhenSmall } },
        { BarAreaSize::normal, { ViewedValueLimits::pitchBarMinimumWhenNormal, ViewedValueLimits::pitchBarMaximumWhenNormal } },
        { BarAreaSize::extended, { ViewedValueLimits::pitchBarMinimumWhenExtended, ViewedValueLimits::pitchBarMaximumWhenExtended } },
        { BarAreaSize::allUnusual, { PsgValues::minimumPitch, PsgValues::maximumPitch } },
    };
    jassert(map.size() == (static_cast<int>(BarAreaSize::last) + 1 - static_cast<int>(BarAreaSize::first)));

    return map;
}

BarAreaSize BarDataCreator::getMinimumSizeForPitch(const int pitch) noexcept
{
    const auto& sizeToMinMaxValue = getSizeToMinMaxValueForPitch();
    return determineMinimumSize(sizeToMinMaxValue, pitch);
}

BarAreaSize BarDataCreator::determineMinimumSize(const std::map<BarAreaSize, std::pair<int, int>>& sizeToMinMaxValue, const int value) noexcept
{
    for (const auto& [size, minMax] : sizeToMinMaxValue) {
        if ((minMax.first <= value) && (minMax.second >= value)) {
            return size;
        }
    }

    jassertfalse;           // Shouldn't happen.
    return BarAreaSize::normal;
}

BarAreaSize BarDataCreator::getMinimumSizeForSidIsActivatedAndRatio() noexcept
{
    return BarAreaSize::small;
}

BarAreaSize BarDataCreator::getMinimumSizeForSidBalancePercent() noexcept
{
    return BarAreaSize::small;
}

BarAreaSize BarDataCreator::getMinimumSizeForSidLowShelfVolume(const int /*volume*/) noexcept
{
    return BarAreaSize::small;
}

BarAreaSize BarDataCreator::getMinimumSizeForSidArpeggio(const bool forceShrinkFull) noexcept
{
    return forceShrinkFull ? BarAreaSize::small : BarAreaSize::normal;
}

BarAreaSize BarDataCreator::getMinimumSizeForSidPitch() noexcept
{
    return BarAreaSize::small;
}

}   // namespace arkostracker

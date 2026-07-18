#include "ArpeggioBarCreator.h"

#include "../../../song/ExpressionConstants.h"
#include "../../../song/cells/CellConstants.h"
#include "../../../utils/NoteUtil.h"
#include "../../../utils/NumberUtil.h"

namespace arkostracker 
{

const juce::Image ArpeggioBarCreator::emptyImage = juce::Image();   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

BarAndCaptionData ArpeggioBarCreator::createArpeggioNoteInOctaveBarData(const int storedValue, const bool withinLoop, const bool outOfBounds, const bool hovered,
                                                                        const bool selected, const bool withCursor) noexcept
{
    constexpr auto minimumValue = CellConstants::firstNoteInOctave;
    constexpr auto maximumValue = CellConstants::lastNoteInOctave;
    constexpr auto backgroundColorId = LookAndFeelConstants::Colors::barArpeggioNoteBackground;
    constexpr auto colorId = LookAndFeelConstants::Colors::barArpeggioNote;
    constexpr auto retrig = false;
    constexpr auto generated = false;
    const auto ignored = outOfBounds;

    // Extracts the note.
    const auto noteInOctave = NoteUtil::getNoteInOctave(storedValue);

    const auto barData = BarData(false, minimumValue, maximumValue,
                                 noteInOctave, backgroundColorId, colorId, withCursor, selected, hovered, withinLoop, outOfBounds, generated,
                                 ignored, retrig);

    const auto mainText = (noteInOctave == 0) ? juce::String() : NumberUtil::toHexDigit(noteInOctave);
    const auto captionData = BarCaptionDisplayedData(emptyImage, mainText, ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData };
}

BarAndCaptionData ArpeggioBarCreator::createArpeggioOctaveBarData(const BarAreaSize barAreaSize, const int storedValue, const bool withinLoop, const bool outOfBounds,
                                                                  const bool hovered, const bool selected, const bool withCursor) noexcept
{
    // Don't show all the octaves according to the size.
    int minimumValue;    // NOLINT(*-init-variables)
    int maximumValue;    // NOLINT(*-init-variables)
    switch (barAreaSize) {
        case BarAreaSize::small:
            maximumValue = 4;
            minimumValue = -maximumValue;
            break;
        default: [[fallthrough]];
        case BarAreaSize::normal: [[fallthrough]];
        case BarAreaSize::extended: [[fallthrough]];
        case BarAreaSize::allUnusual:
            minimumValue = ExpressionConstants::minimumArpeggioOctave;
            maximumValue = ExpressionConstants::maximumArpeggioOctave;
            break;
    }

    constexpr auto backgroundColorId = LookAndFeelConstants::Colors::barArpeggioOctaveBackground;
    constexpr auto colorId = LookAndFeelConstants::Colors::barArpeggioOctave;
    constexpr auto retrig = false;
    constexpr auto generated = false;
    const auto ignored = outOfBounds;

    // Extracts the octave.
    const auto octave = NoteUtil::getOctaveFromNote(storedValue);

    const auto barData = BarData(false, minimumValue, maximumValue,
                                 octave, backgroundColorId, colorId, withCursor, selected, hovered, withinLoop, outOfBounds, generated, ignored, retrig);

    const auto mainText = (octave == 0) ? juce::String() : NumberUtil::signedHexToStringWithPrefix(octave, juce::String(), false);      // Signed hex.
    const auto captionData = BarCaptionDisplayedData(emptyImage, mainText, ignored, outOfBounds, withinLoop, generated);

    return { barData, captionData };
}

}   // namespace arkostracker

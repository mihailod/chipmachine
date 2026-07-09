#include "CellView.h"

#include <algorithm>

#include "../../../../../song/cells/CellConstants.h"
#include "../../../../../utils/NoteUtil.h"
#include "../../../../../utils/NumberUtil.h"

namespace arkostracker 
{

const juce::String CellView::oneDash = juce::String("-");                           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CellView::twoDashes = juce::String("--");                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CellView::threeDashes = juce::String("---");                     // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CellView::fourDashes = juce::String("----");                     // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CellView::rst = juce::String("RST");                             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CellView::twoSpaces = juce::String("  ");                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CellView::sameInstrument = juce::String("==");                   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)


// DisplayedEffect.
// ===================================

CellView::DisplayedEffect::DisplayedEffect(const juce::juce_wchar pEffect, const int pValue, const int pValueDigitCount, const juce::Colour pColor) :
        effect(pEffect),
        value(pValue),
        valueDigitCount(pValueDigitCount),
        color(pColor)
{
}

juce::juce_wchar CellView::DisplayedEffect::getEffect() const noexcept
{
    return effect;
}
int CellView::DisplayedEffect::getValue() const noexcept
{
    return value;
}
int CellView::DisplayedEffect::getValueDigitCount() const noexcept
{
    return valueDigitCount;
}
const juce::Colour& CellView::DisplayedEffect::getColor() const noexcept
{
    return color;
}

bool CellView::DisplayedEffect::operator==(const CellView::DisplayedEffect& rhs) const        // NOLINT(fuchsia-overloaded-operator)
{
    return effect == rhs.effect &&
           value == rhs.value &&
           valueDigitCount == rhs.valueDigitCount &&
           color == rhs.color;
}

bool CellView::DisplayedEffect::operator!=(const CellView::DisplayedEffect& rhs) const        // NOLINT(fuchsia-overloaded-operator)
{
    return !(rhs == *this);
}




// DisplayedData.
// ===================================

CellView::DisplayedData::DisplayedData() noexcept :
        showSomething(false),
        note(),
        instrument(),
        indexToEffects(),
        cursorOnLine(false),
        cursorPosition(CellCursorRank::notPresent),
        startSelection(CellCursorRank::notPresent),
        endSelection(CellCursorRank::notPresent),
        noteColor(),
        instrumentColor(),
        minimized(false),
        primaryHighlight(),
        playedLine(false)
{
}

CellView::DisplayedData::DisplayedData(const OptionalValue<Note>& pNote, const OptionalInt& pInstrument,
                                       const juce::Colour pNoteColor, const juce::Colour pInstrumentColor,
                                       std::unordered_map<int, DisplayedEffect> pIndexToEffects,
                                       const bool pIsCursorOnLine, const CellCursorRank pCursorPosition,
                                       const CellCursorRank pStartSelection, const CellCursorRank pEndSelection,
                                       const bool pMinimized, const OptionalValue<bool> pPrimaryHighlight,
                                       const bool pPlayedLine) noexcept :
        showSomething(true),
        note(pNote),
        instrument(pInstrument),
        indexToEffects(std::move(pIndexToEffects)),
        cursorOnLine(pIsCursorOnLine),
        cursorPosition(pCursorPosition),
        startSelection(pStartSelection),
        endSelection(pEndSelection),
        noteColor(pNoteColor),
        instrumentColor(pInstrumentColor),
        minimized(pMinimized),
        primaryHighlight(pPrimaryHighlight),
        playedLine(pPlayedLine)
{
}

bool CellView::DisplayedData::isShowSomething() const noexcept
{
    return showSomething;
}
const OptionalValue<Note>& CellView::DisplayedData::getNote() const noexcept
{
    return note;
}
const OptionalInt& CellView::DisplayedData::getInstrument() const noexcept
{
    return instrument;
}

bool CellView::DisplayedData::isCursorOnLine() const noexcept
{
    return cursorOnLine;
}
CellCursorRank CellView::DisplayedData::getCursorPosition() const noexcept
{
    return cursorPosition;
}
const std::unordered_map<int, CellView::DisplayedEffect>& CellView::DisplayedData::getEffects() const noexcept
{
    return indexToEffects;
}

CellCursorRank CellView::DisplayedData::getStartSelection() const noexcept
{
    return startSelection;
}

CellCursorRank CellView::DisplayedData::getEndSelection() const noexcept
{
    return endSelection;
}

bool CellView::DisplayedData::areEffectsCollapsed() const noexcept
{
    return minimized;
}

const juce::Colour& CellView::DisplayedData::getNoteColor() const noexcept
{
    return noteColor;
}

const juce::Colour& CellView::DisplayedData::getInstrumentColor() const noexcept
{
    return instrumentColor;
}

const OptionalValue<bool>& CellView::DisplayedData::getPrimaryHighlight() const noexcept
{
    return primaryHighlight;
}

bool CellView::DisplayedData::isPlayedLine() const noexcept
{
    return playedLine;
}

bool CellView::DisplayedData::operator==(const CellView::DisplayedData& rhs) const        // NOLINT(fuchsia-overloaded-operator)
{
    if (showSomething != rhs.showSomething) {
        return false;
    }
    if (!showSomething) {       // If both must not show anything, no need to compare anything else!
        return true;
    }

    return note == rhs.note &&
           instrument == rhs.instrument &&
           indexToEffects == rhs.indexToEffects &&
           cursorOnLine == rhs.cursorOnLine &&
           cursorPosition == rhs.cursorPosition &&
           startSelection == rhs.startSelection &&
           endSelection == rhs.endSelection &&
           noteColor == rhs.noteColor &&
           instrumentColor == rhs.instrumentColor &&
           minimized == rhs.minimized &&
           primaryHighlight == rhs.primaryHighlight &&
           playedLine == rhs.playedLine;
}

bool CellView::DisplayedData::operator!=(const CellView::DisplayedData& rhs) const        // NOLINT(fuchsia-overloaded-operator)
{
    return !(rhs == *this);
}





// CellView.
// ===================================

CellView::CellView(CellView::Listener& pListener, const int pViewRank, const juce::Font& pFont, const juce::Font& pDoubleFont,
                   CellView::DisplayedData pDisplayedData, const CellColors& pCellColors) noexcept :
        AbstractCellView(pViewRank, pFont, pDoubleFont, pCellColors),
        listener(pListener),
        displayedData(std::move(pDisplayedData)),
        letterIndexToCursorRankMinimized(),
        letterIndexToCursorRankMaximized(),
        cursorRankToLetterIndexMinimized(),
        cursorRankToLetterIndexMaximized()
{
    // Fills the index map once and for all.
    buildCursorRankAndLetterIndexMaps(true, letterIndexToCursorRankMinimized, cursorRankToLetterIndexMinimized);
    buildCursorRankAndLetterIndexMaps(false, letterIndexToCursorRankMaximized, cursorRankToLetterIndexMaximized);
}

void CellView::updateDisplayedData(const CellView::DisplayedData& newDisplayedData) noexcept
{
    if (newDisplayedData == displayedData) {
        return;
    }

    displayedData = newDisplayedData;

    repaint();
}

void CellView::setUseDoubleFontAndRepaint(const bool newUseDoubleFont) noexcept
{
    if (useDoubleFont != newUseDoubleFont) {
        useDoubleFont = newUseDoubleFont;

        repaint();
    }
}

int CellView::getCellRankFromX(const int x) const noexcept
{
    const auto rank = getRankFromX(x);
    return static_cast<int>(rank);
}

int CellView::calculateWidth(const int minimizedEffectCount, const juce::Font& pFont) noexcept
{
    constexpr auto effectCount = CellConstants::effectCount;
    jassert((minimizedEffectCount >= 0) && (minimizedEffectCount <= effectCount));

    const auto fullEffectCount = effectCount - minimizedEffectCount;
    const auto widthChar = calculateCharWidth(pFont);

    return widthChar * 4       // Note + space.
            + widthChar * 2    // Instrument (no space, accounted in effects).
            + fullEffectCount * widthChar * 4          // Effect (1) + value (3).
            + minimizedEffectCount * widthChar * 1     // Effect (1).
            + (effectCount * widthChar)                // Space before each effect.
            + 2 * borderWidth;                         // The empty space at the beginning and end.
}


// Component method implementations.
// =====================================

void CellView::paint(juce::Graphics& g)                       // NOLINT(readability-function-cognitive-complexity)
{
    const auto cursorRank = displayedData.getCursorPosition();
    const auto isCursorOnLine = displayedData.isCursorOnLine();     // Cursor line?

    // Fills the background.
    const auto showSomething = displayedData.isShowSomething();
    const auto currentBackgroundColor = determineBackgroundColor(displayedData.isShowSomething(),
                                                                   displayedData.getPrimaryHighlight(), isCursorOnLine,
                                                                   displayedData.isPlayedLine());
    g.fillAll(currentBackgroundColor);

    // Stops if nothing to show.
    if (!showSomething) {
        return;
    }

    g.setFont(useDoubleFont ? doubleFont : font);

    const auto selectionStart = displayedData.getStartSelection();
    const auto selectionEnd = displayedData.getEndSelection();

    const auto height = getHeight();
    const auto yBaseLine = getBaseLine();

    auto charIndex = 0;

    // Displays the note.
    const auto& optionalInstrument = displayedData.getInstrument();
    const auto possibleNote = displayedData.getNote();
    const auto isNotePresent = possibleNote.isPresent();
    const auto isRst = (optionalInstrument.isPresent() && (optionalInstrument.getValue() == CellConstants::rstInstrument) && possibleNote.isPresent());

    // Note or RST?
    const auto noteText = isRst ? rst :
                              possibleNote.isPresent()
                                  ? NoteUtil::getStringFromNote(possibleNote.getValue())      // Note.
                                  : threeDashes;
    constexpr auto noteLengthInDigits = 3;
    jassert(noteText.length() == noteLengthInDigits); (void)noteLengthInDigits;

    const auto noteColor = displayedData.getNoteColor();

    const auto isCursorOnNote = (cursorRank == CellCursorRank::cursorOnNote);
    const auto& isPrimaryHighlight = displayedData.getPrimaryHighlight();
    displayChar(g, charIndex++, yBaseLine, height, noteText.substring(0, 1), noteColor, isCursorOnNote, isCursorOnLine, isPrimaryHighlight);
    displayChar(g, charIndex++, yBaseLine, height, noteText.substring(1, 2), noteColor, isCursorOnNote, isCursorOnLine, isPrimaryHighlight);
    displayChar(g, charIndex++, yBaseLine, height, noteText.substring(2, 3), noteColor, isCursorOnNote, isCursorOnLine, isPrimaryHighlight);

    // Space between the note and the instrument.
    charIndex++;

    // Displays the instrument. Only if a note is present, and if no RST.
    const auto instrumentText = (isRst || !isNotePresent) ? twoSpaces :      // If RST or no note, don't show the instrument.
                                    optionalInstrument.isPresent()
                                        ? NumberUtil::toHexByte(optionalInstrument.getValue())
                                        : sameInstrument;           // Instrument not present, but note, so Legato.
    const auto instrumentColor = displayedData.getInstrumentColor();

    displayChar(g, charIndex++, yBaseLine, height, instrumentText.substring(0, 1), instrumentColor,
                (cursorRank == CellCursorRank::cursorOnInstrumentDigit2), isCursorOnLine, isPrimaryHighlight);
    displayChar(g, charIndex++, yBaseLine, height, instrumentText.substring(1, 2), instrumentColor,
                (cursorRank == CellCursorRank::cursorOnInstrumentDigit1), isCursorOnLine, isPrimaryHighlight);

    // Displays the effects.
    for (auto effectIndex = 0; effectIndex < CellConstants::effectCount; ++effectIndex) {
        // Space between what was before, and the next effect.
        charIndex++;

        charIndex = displayEffect(g, charIndex, yBaseLine, height, effectIndex);
    }

    const auto collapsedEffects = displayedData.areEffectsCollapsed();
    const auto& cursorRankToLetterIndex = getCursorRankToLetterIndex(collapsedEffects);

    // Draws the selection, if any.
    if ((selectionStart != CellCursorRank::notPresent) && (selectionEnd != CellCursorRank::notPresent)) {
        jassert(selectionStart <= selectionEnd);

        // If minimized, the start/end rank might be invisible. They must be mapped into displayed ranks.
        // For example, effect digits are not seen anymore when minimized, so must be mapped to the related effect number, which is visible.
        const auto& mappedSelectionStart = collapsedEffects ? mapRankToMinimizedRank(selectionStart) : selectionStart;
        const auto& mappedSelectionEnd = collapsedEffects ? mapRankToMinimizedRank(selectionEnd) : selectionEnd;

        const auto startIterator = cursorRankToLetterIndex.find(mappedSelectionStart);
        const auto endIterator = cursorRankToLetterIndex.find(mappedSelectionEnd);
        if ((startIterator != cursorRankToLetterIndex.cend()) && (endIterator != cursorRankToLetterIndex.cend())) {
            constexpr auto y = 0;
            const auto width1Char = getWidth1Char();
            const auto digitStart = startIterator->second;
            const auto digitEnd = endIterator->second;
            const auto selectionStartX = digitStart * width1Char;
            // All selection are only one-char wide, except the note, which is three.
            const auto selectionWidthInChars = (mappedSelectionEnd == CellCursorRank::cursorOnNote) ? 3 : 1;
            const auto selectionWidth = (digitEnd + selectionWidthInChars - digitStart) * width1Char;

            g.setColour(cellColors->selectionColor.withAlpha(selectionAlpha));
            g.fillRect(selectionStartX + borderWidth, y, selectionWidth, height);
        }
    }
}

CellCursorRank CellView::mapRankToMinimizedRank(const CellCursorRank rank) noexcept
{
    switch (rank) {
        case CellCursorRank::cursorOnEffect1Digit1: [[fallthrough]];
        case CellCursorRank::cursorOnEffect1Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect1Digit3:
            return CellCursorRank::cursorOnEffect1Number;
        case CellCursorRank::cursorOnEffect2Digit1: [[fallthrough]];
        case CellCursorRank::cursorOnEffect2Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect2Digit3:
            return CellCursorRank::cursorOnEffect2Number;
        case CellCursorRank::cursorOnEffect3Digit1: [[fallthrough]];
        case CellCursorRank::cursorOnEffect3Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect3Digit3:
            return CellCursorRank::cursorOnEffect3Number;
        case CellCursorRank::cursorOnEffect4Digit1: [[fallthrough]];
        case CellCursorRank::cursorOnEffect4Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect4Digit3:
            return CellCursorRank::cursorOnEffect4Number;
        case CellCursorRank::first: [[fallthrough]];
        case CellCursorRank::cursorOnInstrumentDigit2: [[fallthrough]];
        case CellCursorRank::cursorOnInstrumentDigit1: [[fallthrough]];
        case CellCursorRank::cursorOnEffect1Number: [[fallthrough]];
        case CellCursorRank::cursorOnEffect2Number: [[fallthrough]];
        case CellCursorRank::cursorOnEffect3Number: [[fallthrough]];
        case CellCursorRank::cursorOnEffect4Number: [[fallthrough]];
        case CellCursorRank::count:
        default:
            return rank;
    }
}

int CellView::displayEffect(juce::Graphics& g, const int initialLetterIndex, const int baselineY, const int height, const int effectIndex) const noexcept
{
    const auto isMinimized = displayedData.areEffectsCollapsed();

    // Is there an effect?
    const auto effectMap = displayedData.getEffects();
    const auto iterator = effectMap.find(effectIndex);
    juce::String text;
    juce::Colour textColour;
    if (iterator == effectMap.cend()) {
        // No effect.
        text = isMinimized ? oneDash : fourDashes;
        textColour = cellColors->noEffectColor;
    } else {
        // An effect is present.
        const auto& displayedEffect = iterator->second;
        textColour = displayedEffect.getColor();

        text += displayedEffect.getEffect();

        if (!isMinimized) {
            const auto digitCount = displayedEffect.getValueDigitCount();
            const auto rawValue = static_cast<unsigned int>(displayedEffect.getValue());
            switch (digitCount) {
                default:
                    jassertfalse;       // Unhandled digit count.
                case 0:
                    text += threeDashes;
                    break;
                case 1:
                    text += NumberUtil::toHexDigit(static_cast<int>(rawValue >> 8U));
                    text += twoDashes;
                    break;
                case 2:
                    text += NumberUtil::toHexByte(static_cast<int>(rawValue >> 4U));
                    text += oneDash;
                    break;
                case 3:
                    text += NumberUtil::toHexThreeDigits(static_cast<int>(rawValue));
                    break;
            }
        }
    }

    // Displays the letter, with maybe the cursor on it.
    auto letterIndex = initialLetterIndex;
    for (auto index = 0, size = text.length(); index < size; ++index) {
        const auto cursorOnLetter = isCursorOnLetter(letterIndex, displayedData.getCursorPosition());
        const auto cursorOnLine = displayedData.isCursorOnLine();
        const auto& isPrimaryHighlight = displayedData.getPrimaryHighlight();

        displayChar(g, letterIndex, baselineY, height, text.substring(index, index + 1), textColour, cursorOnLetter, cursorOnLine,
                    isPrimaryHighlight);

        ++letterIndex;
    }

    return letterIndex;
}

void CellView::buildCursorRankAndLetterIndexMaps(const bool isMinimized, std::unordered_map<int, CellCursorRank>& letterIndexToCursorRank,
                                                 std::unordered_map<CellCursorRank, int>& cursorRankToLetterIndex) noexcept
{
    // Fills the two maps at the same time.
    letterIndexToCursorRank.clear();
    cursorRankToLetterIndex.clear();

    // Note and instruments are always present, but there is space between.
    // ALL THE SPACES are considered "valid", because it eases the clicks. The next letter is considered the target.
    auto letterIndex = 0;
    auto charIndex = 0;
    letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnNote));
    cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnNote, charIndex));
    letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnNote));
    letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnNote));
    charIndex += 3;

    // Space between note and instrument.
    ++charIndex;
    letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnInstrumentDigit2));

    // Instrument.
    letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnInstrumentDigit2));
    cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnInstrumentDigit2, charIndex++));

    letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnInstrumentDigit1));
    cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnInstrumentDigit1, charIndex++));

    // Space between instrument and effect 1.
    ++charIndex;
    letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect1Number));

    // Effects.
    auto effectIndex = 0;
    letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect1Number));
    cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect1Number, charIndex++));
    if (!isMinimized) {
        letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect1Digit3));
        cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect1Digit3, charIndex++));

        letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect1Digit2));
        cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect1Digit2, charIndex++));

        letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect1Digit1));
        cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect1Digit1, charIndex++));
    }
    // Space between effect 1 and 2.
    ++charIndex;
    letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect2Number));

    // Effect 2. Sorry for the copy/paste, but I prefer this to clumsy arithmetic on enums...
    ++effectIndex;
    letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect2Number));
    cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect2Number, charIndex++));
    if (!isMinimized) {
        letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect2Digit3));
        cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect2Digit3, charIndex++));

        letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect2Digit2));
        cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect2Digit2, charIndex++));

        letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect2Digit1));
        cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect2Digit1, charIndex++));
    }
    // Space between effect 2 and 3.
    ++charIndex;
    letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect3Number));

    // Effect 3.
    ++effectIndex;
    letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect3Number));
    cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect3Number, charIndex++));
    if (!isMinimized) {
        letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect3Digit3));
        cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect3Digit3, charIndex++));

        letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect3Digit2));
        cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect3Digit2, charIndex++));

        letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect3Digit1));
        cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect3Digit1, charIndex++));
    }
    // Space between effect 3 and 4.
    ++charIndex;
    letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect4Number));

    // Effect 4.
    ++effectIndex;
    letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect4Number));
    cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect4Number, charIndex++));
    if (!isMinimized) {
        letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect4Digit3));
        cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect4Digit3, charIndex++));

        letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect4Digit2));
        cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect4Digit2, charIndex++));

        letterIndexToCursorRank.insert(std::make_pair(letterIndex++, CellCursorRank::cursorOnEffect4Digit1));
        cursorRankToLetterIndex.insert(std::make_pair(CellCursorRank::cursorOnEffect4Digit1, charIndex));
    }

    jassert(effectIndex == (CellConstants::effectCount - 1));            // Missing an effect?
}

const std::unordered_map<int, CellCursorRank>& CellView::getLetterIndexToCursorRank(const bool isMinimized) const noexcept
{
    return isMinimized ? letterIndexToCursorRankMinimized : letterIndexToCursorRankMaximized;
}

const std::unordered_map<CellCursorRank, int>& CellView::getCursorRankToLetterIndex(const bool isMinimized) const noexcept
{
    return isMinimized ? cursorRankToLetterIndexMinimized : cursorRankToLetterIndexMaximized;
}

bool CellView::isCursorOnLetter(const int letterIndex, const CellCursorRank cursorRank) const noexcept
{
    const auto& letterIndexToCursorRank = getLetterIndexToCursorRank(displayedData.areEffectsCollapsed());

    jassert(!letterIndexToCursorRank.empty());       // Forgot to initialize it?

    // Sanity check.
    if (cursorRank == CellCursorRank::notPresent) {
        return false;
    }

    const auto iterator = letterIndexToCursorRank.find(letterIndex);
    if (iterator == letterIndexToCursorRank.cend()) {
        return false;           // The letter must be a space (or out of bounds).
    }

    return (iterator->second == cursorRank);
}

CellCursorRank CellView::getRankFromX(int x) const noexcept
{
    // Will select the first if the X is too much "to the left".
    x = std::max(x, 0);

    const auto& letterIndexToCursorRank = getLetterIndexToCursorRank(displayedData.areEffectsCollapsed());

    const auto letterIndex = getLetterIndexFromX(x);

    const auto iterator = letterIndexToCursorRank.find(letterIndex);
    if (iterator == letterIndexToCursorRank.cend()) {
        return CellCursorRank::last;                // Out of bounds.
    }

    return iterator->second;
}


// Component method implementations (suite).
// ============================================

void CellView::mouseDown(const juce::MouseEvent& event)
{
    // What rank clicked? Maybe none.
    const auto clickedX = event.getPosition().getX();
    const auto rank = getRankFromX(clickedX);
    if (rank == CellCursorRank::notPresent) {
        return;
    }

    const auto leftButton = event.mods.isLeftButtonDown();
    const auto shift = event.mods.isShiftDown();
    listener.onUserClickedOnRank(viewRank, rank, leftButton, shift);
}

void CellView::mouseUp(const juce::MouseEvent& event)
{
    const auto leftButton = event.mods.isLeftButtonDown();
    listener.onUserClickIsUp(leftButton);
}

void CellView::mouseDrag(const juce::MouseEvent& event)
{
    listener.onUserDraggedCursor(event);
}

}   // namespace arkostracker

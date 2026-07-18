#include "PsgInstrumentEditorControllerSpecificImpl.h"

#include <algorithm>

#include "../../../../business/song/validation/CheckLoopStartEnd.h"
#include "../../../../controllers/MainController.h"
#include "../../../../song/instrument/psg/PsgPartConstants.h"
#include "../../../../utils/NumberUtil.h"
#include "../../../../utils/PsgValues.h"
#include "../../../../utils/SetUtil.h"
#include "../../../editorWithBars/controller/EditorWithBarsController.h"
#include "../../view/psg/PsgInstrumentEditorViewImpl.h"
#include "PsgInstrumentEditorControllerLogic.h"

namespace arkostracker 
{

PsgInstrumentEditorControllerSpecificImpl::PsgInstrumentEditorControllerSpecificImpl(MainController& pMainController, EditorWithBarsController& pEditorController) noexcept :
        mainController(pMainController),
        songController(pMainController.getSongController()),
        editorController(pEditorController),
        quickEditController(pMainController.getSongController()),
        contextualMenu()
{
    // Note that the SELECTED instrument observer is done by the top controller.
    songController.getInstrumentObservers().addObserver(this);
}

PsgInstrumentEditorControllerSpecificImpl::~PsgInstrumentEditorControllerSpecificImpl()
{
    songController.getInstrumentObservers().removeObserver(this);
}


// InstrumentChangeObserver method implementations.
// ===================================================

void PsgInstrumentEditorControllerSpecificImpl::onInstrumentChanged(const Id& itemId, unsigned int whatChanged)
{
    editorController.onItemMetadataChanged(itemId, whatChanged);
}

void PsgInstrumentEditorControllerSpecificImpl::onPsgInstrumentCellChanged(const Id& itemId, const int cellIndex, const bool mustRefreshAllAfterToo)
{
    editorController.onItemCellChanged(itemId, cellIndex, mustRefreshAllAfterToo);
}

void PsgInstrumentEditorControllerSpecificImpl::onInstrumentsInvalidated()
{
    // Nothing to do.
}


// EditorWithBarsControllerSpecific method implementations.
// ===========================================================

OptionalId PsgInstrumentEditorControllerSpecificImpl::getCurrentItemId() const noexcept
{
    return mainController.getSelectedInstrumentId();
}

std::unique_ptr<EditorWithBarsView> PsgInstrumentEditorControllerSpecificImpl::createView(EditorWithBarsController& editorControllerToUse, int xZoomRate,
                                                                                          const OptionalId& /*selectedItemId*/) noexcept
{
    return std::make_unique<PsgInstrumentEditorViewImpl>(editorControllerToUse, xZoomRate);
}

bool PsgInstrumentEditorControllerSpecificImpl::shouldViewInstanceRecreated(const OptionalId& /*currentItemId*/, const Id& /*newItemId*/) const noexcept
{
    return false;           // No need to recreate anything, this Controller only shows one view.
}

EditorWithBarsView::DisplayTopHeaderData PsgInstrumentEditorControllerSpecificImpl::getDisplayedTopHeaderData(const Id& itemId) const noexcept
{
    Loop loop;
    auto retrig = false;     // Filled in case the instrument doesn't exist.
    auto speed = 0;
    songController.performOnConstInstrument(itemId, [&](const Instrument& instrument) {
        const auto& psgPart = instrument.getConstPsgPart();
        loop = psgPart.getMainLoop();
        retrig = psgPart.isInstrumentRetrig();
        speed = psgPart.getSpeed();
    });
    auto shift = 0;     // Not used for Instruments.

    return { loop.getStartIndex(), loop.getEndIndex(), loop.isLooping(), retrig, speed, shift };
}

int PsgInstrumentEditorControllerSpecificImpl::getItemLength(const Id& itemId) const noexcept
{
    auto length = 0;
    songController.performOnConstInstrument(itemId, [&](const Instrument& instrument) {
        const auto& psgPart = instrument.getConstPsgPart();
        length = psgPart.getLength();
    });

    return length;
}

std::unordered_map<AreaType, BarAndCaptionData> PsgInstrumentEditorControllerSpecificImpl::createBarsData(const Id& itemId, const int cellIndex) const noexcept
{
    std::unordered_map<AreaType, BarAndCaptionData> areaTypeToData;

    // Warning, the cell index may be out of bounds, as more than the actual data can be displayed!
    std::unique_ptr<SpreadPsgInstrumentCell> spreadCell;
    auto length = 0;
    auto withinLoop = false;
    auto outOfBounds = true;
    // As an optimization, gathers all the cells we need here, because we may require several because of the auto-spread.
    songController.performOnConstInstrument(itemId, [&](const Instrument& instrument) {
        const auto& psgPart = instrument.getConstPsgPart();
        length = psgPart.getLength();            // The index CAN be > length, can still be displayed.
        outOfBounds = (cellIndex >= length);

        // If out of bounds, shows an empty cell.
        if (outOfBounds) {
            spreadCell = std::make_unique<SpreadPsgInstrumentCell>(SpreadPsgInstrumentCell::getEmptyCell());
        } else {
            spreadCell = std::make_unique<SpreadPsgInstrumentCell>(psgPart.buildSpreadPsgInstrumentCell(cellIndex));
        }

        const auto& loop = psgPart.getMainLoopRef();
        withinLoop = (cellIndex <= loop.getEndIndex());           // Shows the same even if before start.
    });
    const auto& generatedSections = spreadCell->getGeneratedSectionsRef();
    const auto& cell = spreadCell->getCellWithAutoSpreadAppliedRef();

    // Hovered? This represents the whole row.
    const auto hovered = (editorController.getHoveredBarIndex() == cellIndex);
    // Cursor?
    const auto cursorLocation = editorController.getBarEditorCursorLocation();
    const auto cursorLocationAreaType = cursorLocation.getAreaType();
    const auto cursorXOk = (cellIndex == cursorLocation.getX());

    // For now, nothing can be selected "individually".
    // TO IMPROVE, probably use a Set<PsgSection> with only the selected ones...
    constexpr auto linkBarSelected = false;
    constexpr auto envelopeBarSelected = false;
    constexpr auto noiseBarSelected = false;
    constexpr auto primaryPeriodBarSelected = false;
    constexpr auto primaryArpeggioNoteInOctaveBarSelected = false;
    constexpr auto primaryArpeggioOctaveBarSelected = false;
    constexpr auto primaryPitchBarSelected = false;
    constexpr auto secondaryPeriodBarSelected = false;
    constexpr auto secondaryArpeggioNoteInOctaveBarSelected = false;
    constexpr auto secondaryArpeggioOctaveBarSelected = false;
    constexpr auto secondaryPitchBarSelected = false;
    constexpr auto sidIsActivatedAndRatioSelected = false;
    constexpr auto sidBalancePercentSelected = false;
    constexpr auto sidLowShelfVolumeSelected = false;
    constexpr auto sidArpeggioSelected = false;
    constexpr auto sidPitchSelected = false;

    // Type.
    auto linkBarData = BarDataCreator::createLinkBarData(cell, withinLoop, outOfBounds, SetUtil::contains(generatedSections, PsgSection::soundType),
                                                         hovered, linkBarSelected, cursorXOk && (cursorLocationAreaType == AreaType::soundType));
    areaTypeToData.insert(std::make_pair(AreaType::soundType, linkBarData));

    // Envelope.
    auto envelopeBarData = BarDataCreator::createEnvelopeBarData(cell, getAreaSize(AreaType::envelope), withinLoop, outOfBounds,
                                                                 SetUtil::contains(generatedSections, PsgSection::envelope), hovered, envelopeBarSelected,
                                                                 cursorXOk && (cursorLocationAreaType == AreaType::envelope));
    areaTypeToData.insert(std::make_pair(AreaType::envelope, envelopeBarData));

    // Noise.
    auto noiseBarData = BarDataCreator::createNoiseBarData(cell, withinLoop, outOfBounds, SetUtil::contains(generatedSections, PsgSection::noise),
                                                           hovered, noiseBarSelected, cursorXOk && (cursorLocationAreaType == AreaType::noise));
    areaTypeToData.insert(std::make_pair(AreaType::noise, noiseBarData));

    // -----------------------
    // Primary period.
    auto primaryPeriodBarData = BarDataCreator::createPrimaryPeriodBarData(cell, getAreaSize(AreaType::primaryPeriod), withinLoop, outOfBounds,
                                                                           SetUtil::contains(generatedSections, PsgSection::primaryPeriod), hovered,
                                                                           primaryPeriodBarSelected, cursorXOk && (cursorLocationAreaType == AreaType::primaryPeriod));
    areaTypeToData.insert(std::make_pair(AreaType::primaryPeriod, primaryPeriodBarData));

    // Primary arpeggio note in octave.
    auto primaryArpeggioNoteBarData = BarDataCreator::createPrimaryArpeggioNoteInOctaveBarData(cell, withinLoop, outOfBounds,
                                                                                               SetUtil::contains(generatedSections, PsgSection::primaryArpeggioNoteInOctave),
                                                                                               hovered, primaryArpeggioNoteInOctaveBarSelected,
                                                                                               cursorXOk && (cursorLocationAreaType == AreaType::primaryArpeggioNoteInOctave));
    areaTypeToData.insert(std::make_pair(AreaType::primaryArpeggioNoteInOctave, primaryArpeggioNoteBarData));

    // Primary arpeggio octave.
    auto primaryArpeggioOctaveBarData = BarDataCreator::createPrimaryArpeggioOctaveBarData(cell, getAreaSize(AreaType::primaryArpeggioOctave), withinLoop,
                                                                                           outOfBounds, SetUtil::contains(generatedSections, PsgSection::primaryArpeggioOctave),
                                                                                           hovered, primaryArpeggioOctaveBarSelected,
                                                                                           cursorXOk && (cursorLocationAreaType == AreaType::primaryArpeggioOctave));
    areaTypeToData.insert(std::make_pair(AreaType::primaryArpeggioOctave, primaryArpeggioOctaveBarData));

    // Primary pitch.
    auto primaryArpeggioPitchBarData = BarDataCreator::createPrimaryPitchBarData(cell, getAreaSize(AreaType::primaryPitch), withinLoop,
                                                                                 outOfBounds, SetUtil::contains(generatedSections, PsgSection::primaryPitch),
                                                                                 hovered, primaryPitchBarSelected,
                                                                                 cursorXOk && (cursorLocationAreaType == AreaType::primaryPitch));
    areaTypeToData.insert(std::make_pair(AreaType::primaryPitch, primaryArpeggioPitchBarData));

    // -----------------------
    // Secondary period.
    auto secondaryPeriodBarData = BarDataCreator::createSecondaryPeriodBarData(cell, getAreaSize(AreaType::secondaryPeriod), withinLoop, outOfBounds,
                                                                               SetUtil::contains(generatedSections, PsgSection::secondaryPeriod),
                                                                               hovered, secondaryPeriodBarSelected,
                                                                               cursorXOk && (cursorLocationAreaType == AreaType::secondaryPeriod));
    areaTypeToData.insert(std::make_pair(AreaType::secondaryPeriod, secondaryPeriodBarData));

    // Secondary arpeggio note in octave.
    auto secondaryArpeggioNoteBarData = BarDataCreator::createSecondaryArpeggioNoteInOctaveBarData(cell, withinLoop, outOfBounds,
                                                                                                   SetUtil::contains(generatedSections, PsgSection::secondaryArpeggioNoteInOctave),
                                                                                                   hovered, secondaryArpeggioNoteInOctaveBarSelected,
                                                                                                   cursorXOk && (cursorLocationAreaType == AreaType::secondaryArpeggioNoteInOctave));
    areaTypeToData.insert(std::make_pair(AreaType::secondaryArpeggioNoteInOctave, secondaryArpeggioNoteBarData));

    // Secondary arpeggio octave.
    auto secondaryArpeggioOctaveBarData = BarDataCreator::createSecondaryArpeggioOctaveBarData(cell, getAreaSize(AreaType::secondaryArpeggioOctave), withinLoop,
                                                                                               outOfBounds, SetUtil::contains(generatedSections, PsgSection::secondaryArpeggioOctave),
                                                                                               hovered, secondaryArpeggioOctaveBarSelected,
                                                                                               cursorXOk && (cursorLocationAreaType == AreaType::secondaryArpeggioOctave));
    areaTypeToData.insert(std::make_pair(AreaType::secondaryArpeggioOctave, secondaryArpeggioOctaveBarData));

    // Secondary pitch.
    auto secondaryArpeggioPitchBarData = BarDataCreator::createSecondaryPitchBarData(cell, getAreaSize(AreaType::secondaryPitch), withinLoop,
                                                                                     outOfBounds, SetUtil::contains(generatedSections, PsgSection::secondaryPitch),
                                                                                     hovered, secondaryPitchBarSelected,
                                                                                     cursorXOk && (cursorLocationAreaType == AreaType::secondaryPitch));
    areaTypeToData.insert(std::make_pair(AreaType::secondaryPitch, secondaryArpeggioPitchBarData));

    // -----------------------
    // Sid.
    auto sidIsActivatedAndRatioBarData = BarDataCreator::createSidIsActivatedAndRatioBarData(cell, getAreaSize(AreaType::sidIsActivatedAndRatio), withinLoop,
                                                                                     outOfBounds,
                                                                                     SetUtil::contains(generatedSections, PsgSection::sidIsActivated),
                                                                                     hovered, sidIsActivatedAndRatioSelected,
                                                                                     cursorXOk && (cursorLocationAreaType == AreaType::sidIsActivatedAndRatio));
    areaTypeToData.insert(std::make_pair(AreaType::sidIsActivatedAndRatio, sidIsActivatedAndRatioBarData));

    auto sidBalancePercentBarData = BarDataCreator::createSidBalancePercentBarData(cell, withinLoop, outOfBounds,
                                                       SetUtil::contains(generatedSections, PsgSection::sidBalancePercent),
                                                       hovered, sidBalancePercentSelected,
                                                       cursorXOk && (cursorLocationAreaType == AreaType::sidBalancePercent));
    areaTypeToData.insert(std::make_pair(AreaType::sidBalancePercent, sidBalancePercentBarData));

    auto sidLowShelfVolumeBarData = BarDataCreator::createSidLowShelfVolumeBarData(cell, withinLoop, outOfBounds,
                                                       SetUtil::contains(generatedSections, PsgSection::sidLowShelfVolume),
                                                       hovered,
                                                       sidLowShelfVolumeSelected, cursorXOk && (cursorLocationAreaType == AreaType::sidLowShelfVolume));
    areaTypeToData.insert(std::make_pair(AreaType::sidLowShelfVolume, sidLowShelfVolumeBarData));

    auto sidArpeggioBarData = BarDataCreator::createSidArpeggioBarData(cell, withinLoop, outOfBounds,
                                                       SetUtil::contains(generatedSections, PsgSection::sidArpeggio),
                                                       hovered,
                                                       sidArpeggioSelected, cursorXOk && (cursorLocationAreaType == AreaType::sidArpeggio));
    areaTypeToData.insert(std::make_pair(AreaType::sidArpeggio, sidArpeggioBarData));

    auto sidPitchBarData = BarDataCreator::createSidPitchBarData(cell, withinLoop, outOfBounds,
                                                       SetUtil::contains(generatedSections, PsgSection::sidPitch),
                                                       hovered,
                                                       sidPitchSelected, cursorXOk && (cursorLocationAreaType == AreaType::sidPitch));
    areaTypeToData.insert(std::make_pair(AreaType::sidPitch, sidPitchBarData));

    jassert(areaTypeToData.size() == static_cast<size_t>(AreaType::last) + 1U);              // Missing area?

    return areaTypeToData;
}

void PsgInstrumentEditorControllerSpecificImpl::checkValueAndChangeItem(const Id& itemId, const AreaType areaType, const int barIndex, const int value) noexcept
{
    // Generates a new Cell, if it is worth being modified.
    const auto newCell = PsgInstrumentEditorControllerLogic::checkValueAndChangeInstrumentCell(areaType, barIndex, value, songController, itemId);
    if (newCell == nullptr) {
        return;
    }

    // Modifies the instrument. The target bar may not exist! The action may create some.
    songController.setPsgInstrumentCell(itemId, barIndex, *newCell);
}

std::unordered_set<AreaType> PsgInstrumentEditorControllerSpecificImpl::getAreaTypes() const noexcept
{
    static_assert(static_cast<int>(BarAreaSize::small) == 0, "Small must be 0.");

    return {
        AreaType::soundType, AreaType::envelope, AreaType::noise, AreaType::primaryPeriod, AreaType::primaryArpeggioNoteInOctave,
        AreaType::primaryArpeggioOctave, AreaType::primaryPitch, AreaType::secondaryPeriod, AreaType::secondaryArpeggioNoteInOctave,
        AreaType::secondaryArpeggioOctave, AreaType::secondaryPitch,
        AreaType::sidIsActivatedAndRatio, AreaType::sidBalancePercent, AreaType::sidLowShelfVolume, AreaType::sidArpeggio, AreaType::sidPitch,
    };
}

std::unordered_map<AreaType, BarAreaSize> PsgInstrumentEditorControllerSpecificImpl::getAreaTypesToInitialSize() const noexcept
{
    return {
        { AreaType::soundType, BarAreaSize::small },
        { AreaType::envelope, BarAreaSize::normal },
        { AreaType::noise, BarAreaSize::normal },
        { AreaType::primaryPeriod, BarAreaSize::small },
        { AreaType::primaryArpeggioNoteInOctave, BarAreaSize::normal },
        { AreaType::primaryArpeggioOctave, BarAreaSize::normal },
        { AreaType::primaryPitch, BarAreaSize::small },
        { AreaType::secondaryPeriod, BarAreaSize::small },
        { AreaType::secondaryArpeggioNoteInOctave, BarAreaSize::small },
        { AreaType::secondaryArpeggioOctave, BarAreaSize::small },
        { AreaType::secondaryPitch, BarAreaSize::small },
        { AreaType::sidIsActivatedAndRatio, BarAreaSize::small },
        { AreaType::sidBalancePercent, BarAreaSize::small },
        { AreaType::sidLowShelfVolume, BarAreaSize::small },
        { AreaType::sidArpeggio, BarAreaSize::small },
        { AreaType::sidPitch, BarAreaSize::small },
    };
}

std::unordered_map<AreaType, BarAreaSize> PsgInstrumentEditorControllerSpecificImpl::getAreaTypesToMaximumSize() const noexcept
{
    return {
            { AreaType::soundType, BarAreaSize::normal },
            { AreaType::envelope, BarAreaSize::extended },
            { AreaType::noise, BarAreaSize::normal },
            { AreaType::primaryPeriod, BarAreaSize::allUnusual },
            { AreaType::primaryArpeggioNoteInOctave, BarAreaSize::normal },
            { AreaType::primaryArpeggioOctave, BarAreaSize::normal },
            { AreaType::primaryPitch, BarAreaSize::allUnusual },
            { AreaType::secondaryPeriod, BarAreaSize::allUnusual },
            { AreaType::secondaryArpeggioNoteInOctave, BarAreaSize::normal },
            { AreaType::secondaryArpeggioOctave, BarAreaSize::normal },
            { AreaType::secondaryPitch, BarAreaSize::allUnusual },
            { AreaType::sidIsActivatedAndRatio, BarAreaSize::normal },
            { AreaType::sidBalancePercent, BarAreaSize::normal },
            { AreaType::sidLowShelfVolume, BarAreaSize::normal },
            { AreaType::sidArpeggio, BarAreaSize::normal },
            { AreaType::sidPitch, BarAreaSize::normal },
        };
}

bool PsgInstrumentEditorControllerSpecificImpl::isAutoSpreadAllowed() const noexcept
{
    return true;
}

Loop PsgInstrumentEditorControllerSpecificImpl::getAutoSpreadLoop(const Id& itemId, const AreaType areaType) const noexcept
{
    Loop loop;
    songController.performOnConstInstrument(itemId, [&](const Instrument& instrument) {
        const auto& psgPart = instrument.getConstPsgPart();

        loop = psgPart.getAutoSpreadLoop(PsgInstrumentEditorControllerLogic::convertAreaType(areaType));
    });

    return loop;
}

std::pair<int, Loop> PsgInstrumentEditorControllerSpecificImpl::getItemLengthAndLoop(const Id& itemId) const noexcept
{
    auto length = 0;
    Loop loop;
    songController.performOnConstInstrument(itemId, [&](const Instrument& instrument) {
        const auto& psgPart = instrument.getConstPsgPart();
        length = psgPart.getLength();
        loop = psgPart.getMainLoop();
    });

    return { length, loop };
}

void PsgInstrumentEditorControllerSpecificImpl::setItemMetadata(const Id& itemId, const OptionalInt newStart, const OptionalInt newEnd, const OptionalInt newSpeed, OptionalInt /*newShift*/,
                                                                const OptionalBool newIsLoop, const OptionalBool newIsRetrig,
                                                                const std::unordered_map<PsgSection, Loop> modifiedSectionToAutoSpreadLoop) noexcept
{
    songController.setPsgInstrumentMetadata(itemId, newStart, newEnd, newIsLoop, newIsRetrig, newSpeed, modifiedSectionToAutoSpreadLoop);
}

bool PsgInstrumentEditorControllerSpecificImpl::duplicateColumn(const Id& itemId, const int cellIndex) noexcept
{
    return songController.duplicateInstrumentCell(itemId, cellIndex);
}

bool PsgInstrumentEditorControllerSpecificImpl::duplicateValue(const Id& itemId, const int sourceCellIndex, const AreaType areaType) noexcept
{
    // Gets the current cell.
    const auto targetCellIndex = sourceCellIndex + 1;
    PsgInstrumentCell sourceCell;
    PsgInstrumentCell targetCell;
    songController.performOnConstInstrument(itemId, [&](const Instrument& instrument) {
        const auto& psgPart = instrument.getConstPsgPart();
        sourceCell = psgPart.getCellRefConst(sourceCellIndex);
        // If the target cell is out of bounds, reuses the source cell (more user-friendly, to avoid empty sound type for example).
        if (targetCellIndex < psgPart.getLength()) {
            targetCell = psgPart.getCellRefConst(targetCellIndex);
        } else {
            targetCell = sourceCell;
        }
    });

    // Changes the cell.
    switch (areaType) {
        case AreaType::soundType:
            targetCell = targetCell.withLink(sourceCell.getLink());
            break;
        case AreaType::envelope:
            targetCell = targetCell
                    .withEnvelope(sourceCell.getHardwareEnvelope())
                    .withVolume(sourceCell.getVolume())
                    .withRatio(sourceCell.getRatio());
            break;
        case AreaType::noise:
            targetCell = targetCell.withNoise(sourceCell.getNoise());
            break;
        case AreaType::primaryPeriod:
            targetCell = targetCell.withPrimaryPeriod(sourceCell.getPrimaryPeriod());
            break;
        case AreaType::primaryArpeggioNoteInOctave:
            targetCell = targetCell.withPrimaryArpeggioNoteInOctave(sourceCell.getPrimaryArpeggioNoteInOctave());
            break;
        case AreaType::primaryArpeggioOctave:
            targetCell = targetCell.withPrimaryArpeggioOctave(sourceCell.getPrimaryArpeggioOctave());
            break;
        case AreaType::primaryPitch:
            targetCell = targetCell.withPrimaryPitch(sourceCell.getPrimaryPitch());
            break;
        case AreaType::secondaryPeriod:
            targetCell = targetCell.withSecondaryPeriod(sourceCell.getSecondaryPeriod());
            break;
        case AreaType::secondaryArpeggioNoteInOctave:
            targetCell = targetCell.withSecondaryArpeggioNoteInOctave(sourceCell.getSecondaryArpeggioNoteInOctave());
            break;
        case AreaType::secondaryArpeggioOctave:
            targetCell = targetCell.withSecondaryArpeggioOctave(sourceCell.getSecondaryArpeggioOctave());
            break;
        case AreaType::secondaryPitch:
            targetCell = targetCell.withSecondaryPitch(sourceCell.getSecondaryPitch());
            break;
        case AreaType::sidIsActivatedAndRatio:
            targetCell = targetCell
                    .withSidActivated(sourceCell.isSidActivated())
                    .withSidRatio(sourceCell.getSidRatio());
            break;
        case AreaType::sidBalancePercent:
            targetCell = targetCell.withSidBalancePercent(sourceCell.getSidBalancePercent());
            break;
        case AreaType::sidLowShelfVolume:
            targetCell = targetCell.withSidLowShelfVolume(sourceCell.getSidLowShelfVolume());
            break;
        case AreaType::sidArpeggio:
            targetCell = targetCell.withSidArpeggio(sourceCell.getSidArpeggio());
            break;
        case AreaType::sidPitch:
            targetCell = targetCell.withSidPitch(sourceCell.getSidPitch());
            break;
    }

    songController.setPsgInstrumentCell(itemId, targetCellIndex, targetCell);
    return true;
}

bool PsgInstrumentEditorControllerSpecificImpl::deleteColumn(const Id& itemId, const int cellIndex) noexcept
{
    return songController.deleteInstrumentCell(itemId, cellIndex);
}

void PsgInstrumentEditorControllerSpecificImpl::toggleRetrig(const Id& itemId, const int cellIndex) noexcept
{
    songController.toggleRetrig(itemId, cellIndex);
}

void PsgInstrumentEditorControllerSpecificImpl::generateIncreasingVolume(const Id& itemId, const int cellIndex) noexcept
{
    songController.generateIncreasingVolume(itemId, cellIndex);
}

void PsgInstrumentEditorControllerSpecificImpl::generateDecreasingVolume(const Id& itemId, const int cellIndex) noexcept
{
    songController.generateDecreasingVolume(itemId, cellIndex);
}

bool PsgInstrumentEditorControllerSpecificImpl::isItemMetadataChangeFlagsInteresting(const unsigned int whatChanged) noexcept
{
    return NumberUtil::isBitPresent(whatChanged, static_cast<unsigned int>(other));
}

void PsgInstrumentEditorControllerSpecificImpl::onUserWantsToToggleLoop(const Id& itemId, const OptionalValue<AreaType> areaType) noexcept
{
    // Main loop or auto-spread loop?
    if (areaType.isPresent()) {
        onUserWantsToToggleAutoSpread(itemId, areaType.getValue());
    } else {
        // Gets the current loop state in order to invert it.
        auto currentLoop = false;
        songController.performOnConstInstrument(itemId, [&](const Instrument& instrument) {
            const auto& psgPart = instrument.getConstPsgPart();
            currentLoop = psgPart.getMainLoopRef().isLooping();
        });
        const auto newLoopState = !currentLoop;

        songController.setPsgInstrumentMetadata(itemId, OptionalInt(), OptionalInt(), newLoopState, OptionalBool(),
                                                OptionalInt(), { });
    }
}

void PsgInstrumentEditorControllerSpecificImpl::onUserWantsToIncreaseSpeed(const Id& itemId, const int step) noexcept
{
    // Gets the current speed.
    auto speed = 0;
    songController.performOnConstInstrument(itemId, [&](const Instrument& instrument) {
        const auto& psgPart = instrument.getConstPsgPart();
        speed = psgPart.getSpeed();
    });

    // Sets it.
    speed = NumberUtil::correctNumber(speed + step, PsgPartConstants::minimumSpeed, PsgPartConstants::maximumSpeed);
    songController.setPsgInstrumentMetadata(itemId, OptionalInt(), OptionalInt(), OptionalBool(), OptionalBool(),
                                                speed, { });
}

void PsgInstrumentEditorControllerSpecificImpl::onUserWantsToToggleInstrumentRetrig(const Id& itemId) noexcept
{
    // Gets the current retrig state in order to invert it.
    auto currentRetrig = false;
    songController.performOnConstInstrument(itemId, [&](const Instrument& instrument) {
        const auto& psgPart = instrument.getConstPsgPart();
        currentRetrig = psgPart.isInstrumentRetrig();
    });
    const auto newRetrigState = !currentRetrig;

    songController.setPsgInstrumentMetadata(itemId, OptionalInt(), OptionalInt(), OptionalBool(), newRetrigState, OptionalInt(), { });
}

void PsgInstrumentEditorControllerSpecificImpl::onUserWantsToSetAutoSpreadStartAndEnd(const Id& itemId, const AreaType areaType, const OptionalInt originalNewStart,
                                                                                      const OptionalInt originalNewEnd) noexcept
{
    if (originalNewStart.isAbsent() && originalNewEnd.isAbsent()) {
        jassertfalse;           // No change? Abnormal.
        return;
    }

    const auto psgSection = PsgInstrumentEditorControllerLogic::convertAreaType(areaType);

    // Builds the map to modified sections.
    std::unordered_map<PsgSection, Loop> modifiedSectionToAutoSpreadLoop;

    // Gets the original spread.
    Loop originalLoop;
    auto length = 0;
    songController.performOnConstInstrument(itemId, [&](const Instrument& instrument) {
        const auto& psgPart = instrument.getConstPsgPart();
        length = psgPart.getLength();
        originalLoop = psgPart.getAutoSpreadLoop(psgSection);
    });

    // Corrects the values. Fills the non-given start/end with the original loop.
    const auto newStart = originalNewStart.isPresent() ? originalNewStart.getValue() : originalLoop.getStartIndex();
    const auto newEnd = originalNewEnd.isPresent() ? originalNewEnd.getValue() : originalLoop.getEndIndex();
    const auto newStartAndEnd = CheckLoopStartEnd::checkLoopStartEnd(newStart, newEnd, length, originalNewStart, originalNewEnd);

    const auto newLoop = Loop(newStartAndEnd.first, newStartAndEnd.second, originalLoop.isLooping());

    // No need to do anything if the loop is the same.
    if (newLoop == originalLoop) {
        return;
    }

    modifiedSectionToAutoSpreadLoop[psgSection] = newLoop;

    songController.setPsgInstrumentMetadata(itemId, OptionalInt(), OptionalInt(), OptionalBool(),
                                            OptionalBool(), OptionalInt(), modifiedSectionToAutoSpreadLoop);
}

int PsgInstrumentEditorControllerSpecificImpl::getItemSpeed(const Id& itemId) noexcept
{
    auto speed = 0;
    songController.performOnConstInstrument(itemId, [&](const Instrument& instrument) {
        const auto& psgPart = instrument.getConstPsgPart();
        speed = psgPart.getSpeed();
    });

    return speed;
}

int PsgInstrumentEditorControllerSpecificImpl::correctItemSpeed(const int speed) const noexcept
{
    return NumberUtil::correctNumber(speed, PsgValues::minimumSpeed, PsgValues::maximumSpeed);
}

int PsgInstrumentEditorControllerSpecificImpl::getItemShift(const Id& /*itemId*/) noexcept
{
    jassertfalse;
    return 0;       // No shift for Instruments, shouldn't be called.
}

int PsgInstrumentEditorControllerSpecificImpl::correctItemShift(const int shift) const noexcept
{
    jassertfalse;   // No shift for Instruments, shouldn't be called.
    return shift;
}

const std::unordered_map<AreaType, std::vector<int>>& PsgInstrumentEditorControllerSpecificImpl::getSpecificAreaTypeToSlideSpeed() const noexcept
{
    static const std::unordered_map<AreaType, std::vector<int>> areaTypeToSlideSpeeds = {
            { AreaType::primaryPitch, { 1, 0x8, 0x10 }},
            { AreaType::secondaryPitch, { 1, 0x8, 0x10 }},
    };

    return areaTypeToSlideSpeeds;
}

bool PsgInstrumentEditorControllerSpecificImpl::canHideRow(const Id& /*itemId*/, const AreaType areaType) noexcept
{
    // Envelope cannot be hidden, not relevant.
    return (areaType != AreaType::envelope);
}

bool PsgInstrumentEditorControllerSpecificImpl::doesContainOnlyDefaultData(const Id& itemId, const AreaType areaType) noexcept
{
    if (!canHideRow(itemId, areaType)) {
        return false;
    }

    // TO IMPROVE? Would be nice if the BarCreator static method would be called, ignored rows could be shrunk.

    auto containOnlyDefaultData = true;
    songController.performOnConstInstrument(itemId, [&](const Instrument& instrument) {
        const auto& psgPart = instrument.getConstPsgPart();
        const auto endIndex = psgPart.getMainLoopRef().getEndIndex();       // Keeps only the used part.
        for (auto cellIndex = 0; cellIndex <= endIndex; ++cellIndex) {
            const auto& baseCell = psgPart.buildSpreadPsgInstrumentCell(cellIndex);
            const auto& cell = baseCell.getCellWithAutoSpreadAppliedRef();      // Gets the cell with auto spread applied, else not relevant.
            switch (areaType) {
                case AreaType::envelope:
                    jassertfalse;       // Shouldn't happen, we tested it before.
                    containOnlyDefaultData = false;
                    break;
                case AreaType::soundType:
                    containOnlyDefaultData = (cell.getLink() == EditorWithBarsController::getDefaultSoundTypeValue());
                    break;
                case AreaType::noise:
                    containOnlyDefaultData = (cell.getNoise() == EditorWithBarsController::getDefaultNoiseValue());
                    break;
                case AreaType::primaryPeriod:
                    containOnlyDefaultData = (cell.getPrimaryPeriod() == EditorWithBarsController::getDefaultPeriodValue());
                    break;
                case AreaType::primaryArpeggioNoteInOctave:
                    containOnlyDefaultData = (cell.getPrimaryArpeggioNoteInOctave() == EditorWithBarsController::getDefaultArpeggioNoteInOctaveValue());
                    break;
                case AreaType::primaryArpeggioOctave:
                    containOnlyDefaultData = (cell.getPrimaryArpeggioOctave() == EditorWithBarsController::getDefaultArpeggioOctaveValue());
                    break;
                case AreaType::primaryPitch:
                    containOnlyDefaultData = (cell.getPrimaryPitch() == EditorWithBarsController::getDefaultPitchValue());
                    break;
                case AreaType::secondaryPeriod:
                    containOnlyDefaultData = (cell.getSecondaryPeriod() == EditorWithBarsController::getDefaultPeriodValue());
                    break;
                case AreaType::secondaryArpeggioNoteInOctave:
                    containOnlyDefaultData = (cell.getSecondaryArpeggioNoteInOctave() == EditorWithBarsController::getDefaultArpeggioNoteInOctaveValue());
                    break;
                case AreaType::secondaryArpeggioOctave:
                    containOnlyDefaultData = (cell.getSecondaryArpeggioOctave() == EditorWithBarsController::getDefaultArpeggioOctaveValue());
                    break;
                case AreaType::secondaryPitch:
                    containOnlyDefaultData = (cell.getSecondaryPitch() == EditorWithBarsController::getDefaultPitchValue());
                    break;
                case AreaType::sidIsActivatedAndRatio:
                    containOnlyDefaultData = (cell.isSidActivated() == EditorWithBarsController::getDefaultSidIsActivated());
                    break;
                case AreaType::sidBalancePercent:
                    containOnlyDefaultData = (cell.getSidBalancePercent() == EditorWithBarsController::getDefaultSidBalancePercent());
                    break;
                case AreaType::sidLowShelfVolume:
                    containOnlyDefaultData = (cell.getSidLowShelfVolume() == EditorWithBarsController::getDefaultSidLowShelfVolume());
                    break;
                case AreaType::sidArpeggio:
                    containOnlyDefaultData = (cell.getSidArpeggio() == EditorWithBarsController::getDefaultSidArpeggio());
                    break;
                case AreaType::sidPitch:
                    containOnlyDefaultData = (cell.getSidPitch() == EditorWithBarsController::getDefaultSidPitch());
                    break;
            }
            if (!containOnlyDefaultData) {
                break;      // Stops as soon as we found non default data.
            }
        }
    });

    return containOnlyDefaultData;
}

BarAreaSize PsgInstrumentEditorControllerSpecificImpl::getMinimumSizeForData(const Id& itemId, const AreaType areaType, const bool forceShrinkFull) noexcept
{
    auto newAreaSize = BarAreaSize::small;

    songController.performOnConstInstrument(itemId, [&](const Instrument& instrument) {
        const auto& psgPart = instrument.getConstPsgPart();
        const auto endIndex = psgPart.getMainLoopRef().getEndIndex();

        // Browses each cell, but no need to continue if the max size has been reached.
        for (auto cellIndex = 0; (cellIndex <= endIndex) && (newAreaSize != BarAreaSize::last); ++cellIndex) {
            const auto& baseCell = psgPart.buildSpreadPsgInstrumentCell(cellIndex);
            const auto& cell = baseCell.getCellWithAutoSpreadAppliedRef();      // Gets the cell with auto spread applied, else not relevant.
            auto foundAreaSize = BarAreaSize::small;
            switch (areaType) {
                case AreaType::soundType:
                    foundAreaSize = BarDataCreator::getSizeForSoundType(cell.getLink());
                    break;

                case AreaType::envelope:
                    foundAreaSize = BarDataCreator::getSizeForEnvelope(cell.isHardware(), cell.getHardwareEnvelope(), forceShrinkFull);
                    break;

                case AreaType::noise:
                    foundAreaSize = BarDataCreator::getSizeForNoise(cell.getNoise(), forceShrinkFull);
                    break;

                case AreaType::primaryPeriod:
                    foundAreaSize = BarDataCreator::getMinimumSizeForPeriod(cell.getPrimaryPeriod());
                    break;
                case AreaType::secondaryPeriod:
                    foundAreaSize = BarDataCreator::getMinimumSizeForPeriod(cell.getSecondaryPeriod());
                    break;

                case AreaType::primaryArpeggioNoteInOctave: [[fallthrough]];
                case AreaType::secondaryArpeggioNoteInOctave:
                    foundAreaSize = BarDataCreator::getMinimumSizeForArpeggioNoteInOctave(forceShrinkFull);
                    break;

                case AreaType::primaryArpeggioOctave:
                    foundAreaSize = BarDataCreator::getMinimumSizeForArpeggioOctave(cell.getPrimaryArpeggioOctave());
                    break;
                case AreaType::secondaryArpeggioOctave:
                    foundAreaSize = BarDataCreator::getMinimumSizeForArpeggioOctave(cell.getSecondaryArpeggioOctave());
                    break;

                case AreaType::primaryPitch:
                    foundAreaSize = BarDataCreator::getMinimumSizeForPitch(cell.getPrimaryPitch());
                    break;
                case AreaType::secondaryPitch:
                    foundAreaSize = BarDataCreator::getMinimumSizeForPitch(cell.getSecondaryPitch());
                    break;

                case AreaType::sidIsActivatedAndRatio:
                    foundAreaSize = BarDataCreator::getMinimumSizeForSidIsActivatedAndRatio();
                    break;
                case AreaType::sidBalancePercent:
                    foundAreaSize = BarDataCreator::getMinimumSizeForSidBalancePercent();
                    break;
                case AreaType::sidLowShelfVolume:
                    foundAreaSize = BarDataCreator::getMinimumSizeForSidLowShelfVolume(cell.getSidLowShelfVolume());
                    break;
                case AreaType::sidArpeggio:
                    foundAreaSize = BarDataCreator::getMinimumSizeForSidArpeggio(forceShrinkFull);
                    break;
                case AreaType::sidPitch:
                    foundAreaSize = BarDataCreator::getMinimumSizeForSidPitch();
                    break;
            }

            newAreaSize = std::max(foundAreaSize, newAreaSize);        // Keeps the found size if larger than the previous one.
        }
    });

    return newAreaSize;
}


// ============================================================

BarAreaSize PsgInstrumentEditorControllerSpecificImpl::getAreaSize(const AreaType areaType) const noexcept
{
    return editorController.getAreaSize(areaType);
}

void PsgInstrumentEditorControllerSpecificImpl::onUserWantsToToggleAutoSpread(const Id& itemId, const AreaType areaType) const noexcept
{
    // Converts the AreaType to a PSG Section.
    const auto psgSection = PsgInstrumentEditorControllerLogic::convertAreaType(areaType);

    // Builds the map to modified sections.
    std::unordered_map<PsgSection, Loop> modifiedSectionToAutoSpreadLoop;

    // Gets the original spread state, and inverts it.
    Loop originalLoop;
    songController.performOnConstInstrument(itemId, [&](const Instrument& instrument) {
        const auto& psgPart = instrument.getConstPsgPart();
        originalLoop = psgPart.getAutoSpreadLoop(psgSection);
    });

    // This is the new loop.
    const auto newLoop = originalLoop.withLooping(!originalLoop.isLooping());
    modifiedSectionToAutoSpreadLoop[psgSection] = newLoop;

    songController.setPsgInstrumentMetadata(itemId, OptionalInt(), OptionalInt(), OptionalBool(),
                                            OptionalBool(), OptionalInt(), modifiedSectionToAutoSpreadLoop);
}

void PsgInstrumentEditorControllerSpecificImpl::openContextualMenu(const Id& /*itemId*/, AreaType /*areaType*/, int /*cellIndex*/,
                                                                   const std::function<void()> onMenuItemClicked) noexcept
{
    auto *parentComponent = editorController.getComponentWithTargetCommands();

    contextualMenu = std::make_unique<PsgInstrumentEditorContextualMenu>(mainController.getCommandManager());
    contextualMenu->openContextualMenu(parentComponent, onMenuItemClicked);
}

void PsgInstrumentEditorControllerSpecificImpl::onUserWantsToEditValue(const Id& itemId, const AreaType areaType, const int barIndex) noexcept
{
    quickEditController.edit(itemId, areaType, barIndex);
}

void PsgInstrumentEditorControllerSpecificImpl::onUserWantsToResetValue(const Id& itemId, const AreaType areaType, const int barIndex) noexcept
{
    // Generates a new Cell with a boilerplate value. It is only to get a cell.
    const auto baseCell = PsgInstrumentEditorControllerLogic::checkValueAndChangeInstrumentCell(areaType, barIndex, 0, songController, itemId);
    if (baseCell == nullptr) {
        return;
    }
    auto newCell = *baseCell;

    // Fills the new cell with the data to reset.
    switch (areaType) {
        case AreaType::soundType:
            newCell = newCell.withLink(EditorWithBarsController::getDefaultSoundTypeValue());
            break;
        case AreaType::envelope:
            if (newCell.isHardware()) {
                newCell = newCell
                        .withEnvelope(EditorWithBarsController::getDefaultHardwareEnvelopeValue())
                        .withRatio(EditorWithBarsController::getDefaultHardwareRatioValue());
            } else {
                newCell = newCell.withVolume(EditorWithBarsController::getDefaultVolumeValue());
            }
            break;
        case AreaType::noise:
            newCell = newCell.withNoise(EditorWithBarsController::getDefaultNoiseValue());
            break;
        case AreaType::primaryPeriod:
            newCell = newCell.withPrimaryPeriod(EditorWithBarsController::getDefaultPeriodValue());
            break;
        case AreaType::primaryArpeggioNoteInOctave:
            newCell = newCell.withPrimaryArpeggioNoteInOctave(EditorWithBarsController::getDefaultArpeggioNoteInOctaveValue());
            break;
        case AreaType::primaryArpeggioOctave:
            newCell = newCell.withPrimaryArpeggioOctave(EditorWithBarsController::getDefaultArpeggioOctaveValue());
            break;
        case AreaType::primaryPitch:
            newCell = newCell.withPrimaryPitch(EditorWithBarsController::getDefaultPitchValue());
            break;
        case AreaType::secondaryPeriod:
            newCell = newCell.withSecondaryPeriod(EditorWithBarsController::getDefaultPeriodValue());
            break;
        case AreaType::secondaryArpeggioNoteInOctave:
            newCell = newCell.withSecondaryArpeggioNoteInOctave(EditorWithBarsController::getDefaultArpeggioNoteInOctaveValue());
            break;
        case AreaType::secondaryArpeggioOctave:
            newCell = newCell.withSecondaryArpeggioOctave(EditorWithBarsController::getDefaultArpeggioOctaveValue());
            break;
        case AreaType::secondaryPitch:
            newCell = newCell.withSecondaryPitch(EditorWithBarsController::getDefaultPitchValue());
            break;
        case AreaType::sidIsActivatedAndRatio:
            newCell = newCell.withSidActivated(EditorWithBarsController::getDefaultSidIsActivated());
            break;
        case AreaType::sidBalancePercent:
            newCell = newCell.withSidBalancePercent(EditorWithBarsController::getDefaultSidBalancePercent());
            break;
        case AreaType::sidLowShelfVolume:
            newCell = newCell.withSidLowShelfVolume(EditorWithBarsController::getDefaultSidLowShelfVolume());
            break;
        case AreaType::sidArpeggio:
            newCell = newCell.withSidArpeggio(EditorWithBarsController::getDefaultSidArpeggio());
            break;
        case AreaType::sidPitch:
            newCell = newCell.withSidPitch(EditorWithBarsController::getDefaultSidPitch());
            break;
    }

    songController.setPsgInstrumentCell(itemId, barIndex, newCell);
}

}   // namespace arkostracker

#include "ToolboxControllerImpl.h"

#include "../../../../business/CompoundAction.h"
#include "../../../../controllers/MainController.h"
#include "../../../../controllers/SongController.h"
#include "../../../../song/cells/CellConstants.h"
#include "../../../../utils/CollectionUtil.h"
#include "../../../../utils/NumberUtil.h"
#include "../../../../utils/SetUtil.h"

namespace arkostracker
{

ToolboxControllerImpl::ToolboxControllerImpl(Controller& pController, SongController& pSongController) noexcept :
        controller(pController),
        songController(pSongController),
        toolType(ToolType::transpose),
        selectionTarget(Target::selection),
        selectedInstrumentIndexes(),
        transposeRate(1),
        currentSubsongChannelCount(PsgValues::channelCountPerPsg),
        firstSelectedChannelIndex(0),
        secondSelectedChannelIndex(1),
        toolboxView()
{
    songController.getMainController().observers().getSelectedInstrumentIndexesObservers().addObserver(this);
}

ToolboxControllerImpl::~ToolboxControllerImpl()
{
    songController.getMainController().observers().getSelectedInstrumentIndexesObservers().removeObserver(this);
}


// SelectedInstrumentIndexesObserver method implementations
// ==========================================================

void ToolboxControllerImpl::onSelectedInstrumentsChanged(const std::vector<int>& newSelectedInstrumentIndexes)
{
    selectedInstrumentIndexes = newSelectedInstrumentIndexes;

    if (toolboxView != nullptr) {       // The view can be hidden when an instrument is selected.
        buildAndSendDisplayedData();
    }
}


// ToolboxController method implementations
// ============================================

void ToolboxControllerImpl::show(juce::Component& parent, const int toolBoxButtonX, const int toolBoxButtonY) noexcept
{
    // Slight hack. On app startup, the indexes are generated too late.
    selectedInstrumentIndexes = songController.getMainController().getSelectedInstrumentIndexes();

    toolboxView = std::make_unique<Toolbox>(*this);
    locate(toolBoxButtonX, toolBoxButtonY);
    parent.addAndMakeVisible(*toolboxView);

    buildAndSendDisplayedData(true);
}

void ToolboxControllerImpl::hide() noexcept
{
    toolboxView.reset();
}

void ToolboxControllerImpl::locate(const int toolBoxButtonX, const int toolBoxButtonY) noexcept
{
    if (toolboxView == nullptr) {
        return;
    }

    constexpr auto toolBoxWidth = Toolbox::desiredWidth;
    const auto toolboxX = toolBoxButtonX - toolBoxWidth;
    const auto toolboxY = toolBoxButtonY;
    toolboxView->setBounds(toolboxX, toolboxY, toolBoxWidth, Toolbox::desiredHeight);
}

ToolboxController::DisplayedData ToolboxControllerImpl::buildDisplayedData() noexcept
{
    juce::String buttonText;

    const auto selectedInstrumentIndexesWithoutRst = buildInstrumentIndexesWithoutRst(selectedInstrumentIndexes);

    const auto rstPresentInInstrumentIndexes = isRstPresentInInstrumentIndexes();
    const auto isRstTolerated = (toolType == ToolType::deleteNote);
    const auto isProblemBecauseOfRst = !isRstTolerated && rstPresentInInstrumentIndexes;

    const auto* indexesToUse = isRstTolerated ? &selectedInstrumentIndexes : &selectedInstrumentIndexesWithoutRst;

    auto isButtonEnabled = !isProblemBecauseOfRst && !indexesToUse->empty();

    const auto instrumentCount = indexesToUse->size();

    // Top text.
    juce::String topText;
    if (isProblemBecauseOfRst) {
        topText = juce::translate("RST not accepted.");
    } else if (instrumentCount <= 0) {
        topText = juce::translate("No instrument selected.");
    } else if (instrumentCount == 1) {
        topText = juce::translate("1 instrument selected.");
    } else {
        topText = juce::String(instrumentCount) + juce::translate(" instruments selected.");
    }

    switch (toolType) {
        case ToolType::transpose:
            buttonText = juce::translate("Transpose");
            break;
        case ToolType::deleteNote:
            buttonText = juce::translate("Delete notes");
            break;
        case ToolType::remapInstrument: {
            if (instrumentCount < 2) {
                if (!isProblemBecauseOfRst) {   // Don't override the already set text.
                    topText = juce::translate("Select 2 or more instrs.");
                }
                isButtonEnabled = false;
                buttonText = juce::translate("Map");
            } else {
                buttonText = translate("Map to " + juce::String(*indexesToUse->crbegin()));   // Shows the last item.
            }
            break;
        }
        case ToolType::swapInstrument: {
            if (instrumentCount != 2) {
                if (!isProblemBecauseOfRst) {       // Don't override the already set text.
                    topText = juce::translate("Select only 2 instruments.");
                }
                isButtonEnabled = false;
                buttonText = juce::translate("Swap");
            } else {
                buttonText = translate("Swap " + juce::String(indexesToUse->at(0)) + " and " +
                    juce::String(indexesToUse->at(1)));   // Shows the two items.
            }
            break;
        }
        case ToolType::swapCells: {
            isButtonEnabled = (firstSelectedChannelIndex != secondSelectedChannelIndex);
            topText = isButtonEnabled ? juce::String() : juce::translate("Select two different channels.");
            buttonText = juce::translate("Swap");
            break;
        }
        default:
            jassertfalse;           // Must never happen!
            break;
    }

    return { toolType, topText, buttonText, isButtonEnabled, selectionTarget, transposeRate,
        currentSubsongChannelCount, firstSelectedChannelIndex, secondSelectedChannelIndex };
}

void ToolboxControllerImpl::onUserSelectedTarget(const Target target) noexcept
{
    selectionTarget = target;
    buildAndSendDisplayedData();
}

void ToolboxControllerImpl::onUserWantsToChangeFirstChannelToSwap(const int channelIndex) noexcept
{
    firstSelectedChannelIndex = std::min(channelIndex, currentSubsongChannelCount - 1);
    buildAndSendDisplayedData();
}

void ToolboxControllerImpl::onUserWantsToChangeSecondChannelToSwap(const int channelIndex) noexcept
{
    secondSelectedChannelIndex = std::min(channelIndex, currentSubsongChannelCount - 1);
    buildAndSendDisplayedData();
}

void ToolboxControllerImpl::onUserChangedToolType(const ToolType newToolType) noexcept
{
    toolType = newToolType;
    buildAndSendDisplayedData();
}

void ToolboxControllerImpl::onUserWantsToChangeTranspositionRate(const int desiredTranspositionRate) noexcept
{
    const auto correctedTransposeRate = NumberUtil::correctNumber(desiredTranspositionRate, -CellConstants::noteCountInOctave, CellConstants::noteCountInOctave);
    if (transposeRate != correctedTransposeRate) {
        transposeRate = correctedTransposeRate;
        buildAndSendDisplayedData();
    }
}

void ToolboxControllerImpl::onSongMetadataChanged(const Id& subsongId) noexcept
{
    if (toolboxView == nullptr) {
        return;
    }

    rebuildChannelCountAndInterface(subsongId);
}

void ToolboxControllerImpl::onSubsongMetadataChanged(const Id& subsongId) noexcept
{
    if (toolboxView == nullptr) {
        return;
    }

    rebuildChannelCountAndInterface(subsongId);
}


// ============================================

void ToolboxControllerImpl::rebuildChannelCountAndInterface(const Id& subsongId) noexcept
{
    // Gets and corrects the channel count and target channels (for Swap Cells).
    auto channelCount = PsgValues::channelCountPerPsg;
    songController.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        channelCount = subsong.getChannelCount();
    });
    currentSubsongChannelCount = channelCount;
    const auto lastChannelIndex = channelCount - 1;
    jassert(lastChannelIndex > 0);
    firstSelectedChannelIndex = NumberUtil::correctNumber(firstSelectedChannelIndex, 0, lastChannelIndex);
    secondSelectedChannelIndex = NumberUtil::correctNumber(secondSelectedChannelIndex, 0, lastChannelIndex);

    // Builds the interface.
    buildAndSendDisplayedData();
}


std::vector<int> ToolboxControllerImpl::buildInstrumentIndexesWithoutRst(const std::vector<int>& inputIndexes) noexcept
{
    std::vector<int> outputIndexes;
    outputIndexes.reserve(inputIndexes.size());
    // Removes the possible RST.
    std::copy_if(inputIndexes.cbegin(), inputIndexes.cend(), std::back_inserter(outputIndexes),
        [] (const int i) {
            return (i != CellConstants::rstInstrument);
        } );


    return outputIndexes;
}

bool ToolboxControllerImpl::isRstPresentInInstrumentIndexes() const noexcept
{
    return std::find(selectedInstrumentIndexes.cbegin(), selectedInstrumentIndexes.cend(), CellConstants::rstInstrument) !=
        selectedInstrumentIndexes.cend();
}

void ToolboxControllerImpl::buildAndSendDisplayedData(const bool forceViewCreation) noexcept
{
    if (toolboxView == nullptr) {           // May be nullptr when changing song because of the SubsongMetadata observer.
        return;
    }

    const auto displayedData = buildDisplayedData();
    toolboxView->updateUi(displayedData, forceViewCreation);
}

void ToolboxControllerImpl::perform() noexcept
{
    // What selected data?
    std::vector<SelectedData> selectedDataPerSubsong;

    switch (selectionTarget) {
        case Target::selection: {
            // Only the current selection in this Subsong.
            const auto currentlySelectedData = controller.getSelectedData();
            selectedDataPerSubsong.push_back(currentlySelectedData);
            break;
        }
        case Target::wholePattern: {
            const auto patternSelectedData = controller.getPatternSelection();
            selectedDataPerSubsong.push_back(patternSelectedData);
            break;
        }
        case Target::wholeSubsong: {
            const auto patternSelectedData = controller.getCurrentSubsongSelection();
            selectedDataPerSubsong.push_back(patternSelectedData);
            break;
        }
        case Target::wholeSong: {
            const auto patternsSelectedData = controller.getAllSubsongsSelection();
            CollectionUtil::append(patternsSelectedData, selectedDataPerSubsong);
            break;
        }
    }

    // Builds an Action for each Subsong.
    auto actions = std::make_unique<CompoundAction>();
    juce::String actionName;    // Set several times below, who cares.

    for (const auto& selectedData : selectedDataPerSubsong) {
        std::unique_ptr<juce::UndoableAction> action;
        switch (toolType) {
            case ToolType::transpose:
                action = buildTransposeAction(selectedData);
                actionName = juce::translate("Transpose");
                break;
            case ToolType::deleteNote:
                action = buildDeleteNoteAction(selectedData);
                actionName = juce::translate("Delete notes");
                break;
            case ToolType::remapInstrument:
                action = buildRemapInstrumentsAction(selectedData);
                actionName = juce::translate("Remap instruments");
                break;
            case ToolType::swapInstrument:
                action = buildSwapInstrumentsAction(selectedData);
                actionName = juce::translate("Swap instruments");
                break;
            case ToolType::swapCells:
                action = buildSwapCellsAction(selectedData);
                actionName = juce::translate("Swap cells");
                break;
        }

        if (action == nullptr) {
            jassertfalse;       // Couldn't create the action.
            return;
        }
        actions->addAction(std::move(action));
    }

    songController.performAction(std::move(actions), actionName);
}

std::unique_ptr<TransposeNote> ToolboxControllerImpl::buildTransposeAction(const SelectedData& selectedData) const noexcept
{
    return std::make_unique<TransposeNote>(songController, transposeRate, selectedData, SetUtil::toUnorderedSet(selectedInstrumentIndexes));
}

std::unique_ptr<DeleteNote> ToolboxControllerImpl::buildDeleteNoteAction(const SelectedData& selectedData) const noexcept
{
    return std::make_unique<DeleteNote>(songController, selectedData, SetUtil::toUnorderedSet(selectedInstrumentIndexes));
}

std::unique_ptr<SwapInstruments> ToolboxControllerImpl::buildSwapInstrumentsAction(const SelectedData& selectedData) const noexcept
{
    if (selectedInstrumentIndexes.size() != 2) {
        jassertfalse;           // Shouldn't have been tested by the UI before.
        return nullptr;
    }

    return std::make_unique<SwapInstruments>(songController, selectedData, selectedInstrumentIndexes.at(0),
        selectedInstrumentIndexes.at(1));
}

std::unique_ptr<RemapInstruments> ToolboxControllerImpl::buildRemapInstrumentsAction(const SelectedData& selectedData) const noexcept
{
    const auto allIndexes = buildInstrumentIndexesWithoutRst(selectedInstrumentIndexes);
    if (allIndexes.size() < 2) {
        jassertfalse;           // Shouldn't have been tested by the UI before.
        return nullptr;
    }

    const auto targetInstrumentIndex = *allIndexes.crbegin();

    // The source indexes are all indexes, minus the last one.
    std::vector<int> sourceInstrumentIndexes;
    std::copy(allIndexes.cbegin(), allIndexes.cend() - 1, std::back_inserter(sourceInstrumentIndexes));

    return std::make_unique<RemapInstruments>(songController, selectedData, SetUtil::toUnorderedSet(sourceInstrumentIndexes), targetInstrumentIndex);
}

std::unique_ptr<SwapCells> ToolboxControllerImpl::buildSwapCellsAction(const SelectedData& selectedData) const noexcept
{
    const auto& endLocationOptional = selectedData.getEndLocation();
    if (endLocationOptional.isAbsent()) {
        jassertfalse;           // Shouldn't happen.
        return nullptr;
    }
    const auto subsongId = selectedData.getStartLocation().getSubsongId();
    const auto topLineIndex = selectedData.getStartLocation().getLine();
    const auto bottomLineIndex = endLocationOptional.getValueRef().getLine();

    const auto positionIndexStart = selectedData.getStartLocation().getPosition();
    const auto positionIndexEnd = endLocationOptional.getValueRef().getPosition();

    return std::make_unique<SwapCells>(songController, subsongId, positionIndexStart, positionIndexEnd, topLineIndex, bottomLineIndex,
        firstSelectedChannelIndex, secondSelectedChannelIndex);
}

} // namespace arkostracker

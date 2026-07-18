#pragma once

#pragma once

#include <vector>

#include "../../../../business/actions/instruments/RemapInstruments.h"
#include "../../../../business/actions/instruments/SwapInstruments.h"
#include "../../../../business/actions/patternViewer/DeleteNote.h"
#include "../../../../business/actions/patternViewer/SwapCells.h"
#include "../../../../business/actions/patternViewer/TransposeNote.h"
#include "../../../../controllers/observers/SelectedInstrumentIndexesObserver.h"
#include "../../view/toolbox/Toolbox.h"
#include "ToolboxController.h"

namespace arkostracker
{

class SongController;
class TransposeNote;
class Transpose;

/**
 * Controller to the Toolbox.
 * Note that the observers of the Subsongs data are not observed here, but notified by the controller,
 * which helps synchronize with the PV.
 */
class ToolboxControllerImpl final : public ToolboxController,
                                    public SelectedInstrumentIndexesObserver
{
public:
    /**
     * Constructor.
     * @param controller the top controller to get high-level information.
     * @param songController the Song Controller.
     */
    explicit ToolboxControllerImpl(Controller& controller, SongController& songController) noexcept;

    /** Destructor. */
    ~ToolboxControllerImpl() override;

    // ToolboxController method implementations
    // ============================================
    void show(juce::Component& parent, int toolBoxButtonX, int toolBoxButtonY) noexcept override;
    void hide() noexcept override;
    void locate(int toolBoxButtonX, int toolBoxButtonY) noexcept override;
    void onUserSelectedTarget(Target target) noexcept override;
    void onUserWantsToChangeFirstChannelToSwap(int channelIndex) noexcept override;
    void onUserWantsToChangeSecondChannelToSwap(int channelIndex) noexcept override;
    void onUserChangedToolType(ToolType toolType) noexcept override;
    void onUserWantsToChangeTranspositionRate(int newTranspositionRate) noexcept override;
    void perform() noexcept override;
    void onSongMetadataChanged(const Id& subsongId) noexcept override;
    void onSubsongMetadataChanged(const Id& subsongId) noexcept override;

    // SelectedInstrumentIndexesObserver method implementations
    // ==========================================================
    void onSelectedInstrumentsChanged(const std::vector<int>& selectedInstrumentIndexes) override;

private:
    /** @return a copy of the input list without RST index. */
    static std::vector<int> buildInstrumentIndexesWithoutRst(const std::vector<int>& inputIndexes) noexcept;
    /** @return true if RST is present in the input list of instruments. */
    bool isRstPresentInInstrumentIndexes() const noexcept;

    /** @return a DisplayedData object from the currently stored data. */
    DisplayedData buildDisplayedData() noexcept;

    /**
     * Builds and sends the displayed data to the UI.
     * @param forceViewCreation true to force the view creation. Useful on view init.
     */
    void buildAndSendDisplayedData(bool forceViewCreation = false) noexcept;

    /**
     * @return a Transposition action for one Subsong.
     * @param selectedData what data is selected in the Subsong.
     */
    std::unique_ptr<TransposeNote> buildTransposeAction(const SelectedData& selectedData) const noexcept;

    /**
     * @return a Delete Note action for one Subsong.
     * @param selectedData what data is selected in the Subsong.
     */
    std::unique_ptr<DeleteNote> buildDeleteNoteAction(const SelectedData& selectedData) const noexcept;

    /**
     * @return a Swap Instruments action for one Subsong.
     * @param selectedData what data is selected in the Subsong.
     */
    std::unique_ptr<SwapInstruments> buildSwapInstrumentsAction(const SelectedData& selectedData) const noexcept;

    /**
     * @return a Remap Instruments action for one Subsong.
     * @param selectedData what data is selected in the Subsong.
     */
    std::unique_ptr<RemapInstruments> buildRemapInstrumentsAction(const SelectedData& selectedData) const noexcept;

    /**
     * @return a Swap Cells action for one Subsong or more.
     * @param selectedData what data is selected in the Subsong.
     */
    std::unique_ptr<SwapCells> buildSwapCellsAction(const SelectedData& selectedData) const noexcept;

    /**
     * Rebuilds the channel count of the Subsong which ID is given, and refreshes the UI.
     * @param subsongId the viewed SubsongId.
     */
    void rebuildChannelCountAndInterface(const Id& subsongId) noexcept;

    Controller& controller;
    SongController& songController;

    ToolType toolType;
    Target selectionTarget;
    std::vector<int> selectedInstrumentIndexes;
    int transposeRate;

    int currentSubsongChannelCount;
    int firstSelectedChannelIndex;
    int secondSelectedChannelIndex;

    std::unique_ptr<Toolbox> toolboxView;
};

} // namespace arkostracker

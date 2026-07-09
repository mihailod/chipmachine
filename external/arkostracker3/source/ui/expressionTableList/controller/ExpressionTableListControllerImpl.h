#pragma once

#include <set>

#include "../../../controllers/observers/ExpressionChangeObserver.h"
#include "../../../controllers/observers/SelectedExpressionIndexObserver.h"
#include "../../containerArranger/PanelType.h"
#include "../../lists/controller/ListControllerImpl.h"
#include "../view/CreateNewExpressionView.h"
#include "LoadExpression.h"
#include "SaveExpression.h"

namespace arkostracker 
{

class MainController;
class SongController;

/** Implementation of the Expression Table Controller. */
class ExpressionTableListControllerImpl : public ListControllerImpl,
                                          public SelectedExpressionIndexObserver,
                                          public ExpressionChangeObserver,
                                          public CreateNewExpressionView::Listener
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param isArpeggio true if the View is about Arpeggio, false if about Pitch.
     */
    ExpressionTableListControllerImpl(MainController& mainController, bool isArpeggio) noexcept;

    /** Destructor. */
    ~ExpressionTableListControllerImpl() override;

    // SelectedExpressionIndexObserver method implementations.
    // =========================================================
    void onSelectedExpressionChanged(bool isArpeggio, const OptionalId& newExpressionId, bool forceSingleSelection) override;

    // ExpressionChangeObserver method implementations.
    // ==================================================
    void onExpressionChanged(const Id& expressionId, unsigned int whatChanged) override;
    void onExpressionCellChanged(const Id& expressionId, int cellIndex, bool mustAlsoRefreshPastIndex) override;
    void onExpressionsInvalidated() override;

    // DataProvider method implementations.
    // ======================================
    int getItemCount() const noexcept override;
    juce::String getItemTitle(int rowNumber) const noexcept override;
    juce::String getNumberPrefix() const noexcept override;
    OptionalInt getSelectedIndex() const noexcept override;

    // CreateNewExpressionView::Listener method implementations.
    // ============================================================
    void onCreateNewExpressionValidated(int index, juce::String name, ExpressionTemplate::Template desiredTemplate) noexcept override;
    void onCreateNewExpressionCancelled() noexcept override;

protected:
    // ListController method implementations.
    // ============================================
    std::unique_ptr<ListView> createListView() noexcept override;
    OptionalId getIdFromIndex(int index) const noexcept override;
    void moveItems(const std::set<int>& indexesToMove, int targetIndex) noexcept override;
    void deleteItems(const std::set<int>& indexesToDelete) noexcept override;
    void renameItem(int itemIndex, const juce::String& newName) noexcept override;
    void enterSelectedItem() noexcept override;
    void promptToAddNewItemAndPerform(int itemIndex) noexcept override;
    void notifyNewSelectedId(Id newSelectedExpressionId) noexcept override;
    void notifySelection(const std::vector<int>& selectedIndexes) noexcept override;
    std::unique_ptr<juce::XmlElement> getItemsToCopy(const std::set<int>& indexesToCopy) const noexcept override;
    std::vector<Id> deserializeXmlItemsAndPaste(const juce::XmlElement& serializedItems, int targetIndex) noexcept override;
    ListCommands& getListCommands() noexcept override;
    void putAdditionalPopUpMenuItems(juce::PopupMenu& popupMenu, juce::ApplicationCommandManager& commandManager) noexcept override;
    bool performSubCommand(const juce::ApplicationCommandTarget::InvocationInfo& info) noexcept override;

private:
    /** Saves the current expression. */
    void saveExpression() noexcept;
    /** Load the current expression after. */
    void loadExpression() noexcept;

    /**
     * Called when the expression was saved.
     * @param success true if success.
     */
    void onSaveExpressionFinished(bool success) noexcept;
    /** Called when the user canceled the expression saving. */
    void onSaveExpressionCanceled() noexcept;

    /**
     * Called when the expression was loaded.
     * @param expression the Expression if success, or absent.
     */
    void onLoadExpressionFinished(std::unique_ptr<Expression> expression) noexcept;
    /** Called when the user canceled the expression loading. */
    void onLoadExpressionCanceled() noexcept;

    SongController& songController;
    bool isArpeggio;                                // True if Arpeggio, false if Pitch.
    ListCommands listCommands;

    std::unique_ptr<CreateNewExpressionView> createNewExpressionView;

    std::unique_ptr<SaveExpression> saveExpressionProcess;
    std::unique_ptr<LoadExpression> loadExpressionProcess;
};

}   // namespace arkostracker

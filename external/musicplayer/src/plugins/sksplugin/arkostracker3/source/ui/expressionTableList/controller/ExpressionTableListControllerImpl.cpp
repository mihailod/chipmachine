#include "ExpressionTableListControllerImpl.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../business/serialization/song/ExpressionSerializer.h"
#include "../../../business/song/validation/CorrectExpression.h"
#include "../../../controllers/MainController.h"
#include "../../components/dialogs/SimpleTextDialog.h"
#include "../../containerArranger/PanelSearcher.h"
#include "../../keyboard/CommandIds.h"
#include "../view/ExpressionTableListViewImpl.h"

namespace arkostracker 
{

ExpressionTableListControllerImpl::ExpressionTableListControllerImpl(MainController& pMainController, const bool pIsArpeggio) noexcept :
        ListControllerImpl(pMainController),
        songController(pMainController.getSongController()),
        isArpeggio(pIsArpeggio),
        listCommands(*this),
        createNewExpressionView(),
        saveExpressionProcess(),
        loadExpressionProcess()
{
    // Registers to be aware of changes.
    songController.getExpressionObservers(isArpeggio).addObserver(this);
    getMainController().observers().getSelectedExpressionIndexObservers(isArpeggio).addObserver(this);
}

ExpressionTableListControllerImpl::~ExpressionTableListControllerImpl()
{
    songController.getExpressionObservers(isArpeggio).removeObserver(this);
    getMainController().observers().getSelectedExpressionIndexObservers(isArpeggio).removeObserver(this);
}


// SelectedArpeggioIndexObserver method implementations.
// =====================================================

void ExpressionTableListControllerImpl::onSelectedExpressionChanged(const bool paramIsArpeggio, const OptionalId& newExpressionId, const bool forceSingleSelection)
{
    // Only the same Expression type interests us. Security only...
    if (isArpeggio != paramIsArpeggio) {
        jassertfalse;
        return;
    }

    if (newExpressionId.isAbsent()) {
        jassertfalse;       // Nothing to show?
        return;
    }

    const auto expressionIndex = songController.getExpressionIndex(isArpeggio, newExpressionId.getValueRef());
    if (expressionIndex.isAbsent()) {
        jassertfalse;       // Instrument does not exist?
        return;
    }

    // Lets the parent do the job.
    onSelectedItemChanged(expressionIndex.getValueRef(), forceSingleSelection);
}


// ExpressionChangeObserver method implementations.
// ==================================================

void ExpressionTableListControllerImpl::onExpressionChanged(const Id& expressionId, const unsigned int /*whatChanged*/)
{
    const auto expressionIndex = songController.getExpressionIndex(isArpeggio, expressionId);
    if (expressionIndex.isAbsent()) {
        jassertfalse;       // Expression does not exist?
        return;
    }
    jassert(expressionIndex.getValueRef() > 0);

    updateItem(expressionIndex.getValueRef());
}

void ExpressionTableListControllerImpl::onExpressionCellChanged(const Id& /*expressionId*/, int /*cellIndex*/, bool /*mustAlsoRefreshPastIndex*/)
{
    // Not interested.
}

void ExpressionTableListControllerImpl::onExpressionsInvalidated()
{
    updateList();
}


// DataProvider method implementations.
// ======================================

int ExpressionTableListControllerImpl::getItemCount() const noexcept
{
    return std::max(songController.getExpressionCount(isArpeggio), 0);
}

juce::String ExpressionTableListControllerImpl::getItemTitle(const int rowNumber) const noexcept
{
    return (rowNumber == 0) ? juce::translate("None (read-only)") : songController.getExpressionNameFromIndex(isArpeggio, rowNumber);
}

juce::String ExpressionTableListControllerImpl::getNumberPrefix() const noexcept
{
    return isArpeggio ? juce::translate("Arp ") : juce::translate("Pitch ");
}

OptionalInt ExpressionTableListControllerImpl::getSelectedIndex() const noexcept
{
    const auto expressionId = getMainController().getSelectedExpressionId(isArpeggio);
    if (expressionId.isAbsent()) {
        return { };
    }

    return songController.getExpressionIndex(isArpeggio, expressionId.getValueRef());
}


// ListControllerImpl method implementations.
// ============================================

std::unique_ptr<ListView> ExpressionTableListControllerImpl::createListView() noexcept
{
    return std::make_unique<ExpressionTableListViewImpl>(*this, *this, *this, isArpeggio);
}

OptionalId ExpressionTableListControllerImpl::getIdFromIndex(const int index) const noexcept
{
    return songController.getExpressionId(isArpeggio, index);
}

void ExpressionTableListControllerImpl::moveItems(const std::set<int>& indexesToMove, const int targetIndex) noexcept
{
    getMainController().getSongController().moveExpressions(isArpeggio, indexesToMove, targetIndex);
}

void ExpressionTableListControllerImpl::deleteItems(const std::set<int>& indexesToDelete) noexcept
{
    songController.deleteExpressions(isArpeggio, indexesToDelete);
}

void ExpressionTableListControllerImpl::renameItem(const int itemIndex, const juce::String& newName) noexcept
{
    const auto expressionId = songController.getExpressionId(isArpeggio, itemIndex);
    if (expressionId.isAbsent()) {
        jassertfalse;
        return;
    }
    songController.setExpressionName(isArpeggio, expressionId.getValueRef(), newName);
}

void ExpressionTableListControllerImpl::enterSelectedItem() noexcept
{
    const auto relatedPanel = getRelatedPanelToOpen();
    getMainController().searchAndOpenPanel(relatedPanel);
}

void ExpressionTableListControllerImpl::promptToAddNewItemAndPerform(int itemIndex) noexcept
{
    jassert(createNewExpressionView == nullptr);

    // Shows prompt.
    createNewExpressionView = std::make_unique<CreateNewExpressionView>(*this, itemIndex, isArpeggio);
}

void ExpressionTableListControllerImpl::notifyNewSelectedId(const Id newSelectedExpressionId) noexcept
{
    getMainController().setSelectedExpressionId(isArpeggio, newSelectedExpressionId, false);
}

void ExpressionTableListControllerImpl::notifySelection(const std::vector<int>& /*selectedIndexes*/) noexcept
{
    // Not interested.
}

std::unique_ptr<juce::XmlElement> ExpressionTableListControllerImpl::getItemsToCopy(const std::set<int>& indexesToCopy) const noexcept
{
    std::vector<Expression> expressions;

    // Reads all the Expressions.
    for (const auto index : indexesToCopy) {
        jassert(index > 0);         // Not touching the 0th!

        const auto expressionId = songController.getExpressionId(isArpeggio, index);
        if (expressionId.isAbsent()) {
            jassertfalse;
            continue;
        }

        const auto expression = songController.getExpression(isArpeggio, expressionId.getValueRef());
        expressions.push_back(expression);
    }

    // Serializes them to XML.
    return ExpressionSerializer::serialize(expressions);
}

std::vector<Id> ExpressionTableListControllerImpl::deserializeXmlItemsAndPaste(const juce::XmlElement& serializedItems, const int targetIndex) noexcept
{
    // Deserializes the XML.
    auto expressions = ExpressionSerializer::deserializeExpressions(serializedItems);
    if (expressions.empty()) {
        return { };
    }

    // Only keeps the right expressions! (Arp if arp!)
    expressions.erase(
            std::remove_if(expressions.begin(), expressions.end(), [&](const std::unique_ptr<Expression>& expression) {
                return expression->isArpeggio() != isArpeggio;
            }),
            expressions.cend()
    );
    if (expressions.empty()) {
        jassertfalse;   // Tried to copy wrong kind of Expression, it seems.
        return { };
    }

    // Corrects the XML. Who knows what a sneaky user modified in the XML!
    CorrectExpression correctExpression;
    std::vector<Id> ids;
    for (auto& expression : expressions) {
        correctExpression.correctExpression(*expression);
        ids.push_back(expression->getId());
    }

    // Performs the paste.
    songController.insertExpressions(isArpeggio, targetIndex, std::move(expressions));

    return ids;
}

ListCommands& ExpressionTableListControllerImpl::getListCommands() noexcept
{
    return listCommands;
}

void ExpressionTableListControllerImpl::putAdditionalPopUpMenuItems(juce::PopupMenu& popupMenu, juce::ApplicationCommandManager& commandManager) noexcept
{
    const auto loadText = isArpeggio ? juce::translate("Load arpeggio after") : juce::translate("Load pitch after");
    const auto saveText = isArpeggio ? juce::translate("Save arpeggio") : juce::translate("Save pitch");

    popupMenu.addSeparator();
    popupMenu.addCommandItem(&commandManager, listLoadItem, loadText);
    popupMenu.addCommandItem(&commandManager, listSaveItem, saveText);
}

bool ExpressionTableListControllerImpl::performSubCommand(const juce::ApplicationCommandTarget::InvocationInfo& info) noexcept
{
    auto success = true;

    switch (info.commandID) {
        case listLoadItem:
            loadExpression();
            break;
        case listSaveItem:
            saveExpression();
            break;
        default:
            success = false;
            jassertfalse;       // Command not managed?
            break;
    }

    return success;
}


// CreateNewExpressionView::Listener method implementations.
// ============================================================

void ExpressionTableListControllerImpl::onCreateNewExpressionValidated(const int index, const juce::String name, const ExpressionTemplate::Template desiredTemplate) noexcept
{
    createNewExpressionView.reset();

    // Builds the Expression.
    auto expression = ExpressionTemplate::getTemplate(desiredTemplate);
    expression->setName(name);

    // Inserts one empty Expression.
    std::vector<std::unique_ptr<Expression>> expressions;
    expressions.push_back(std::move(expression));
    songController.insertExpressions(isArpeggio, index, std::move(expressions));

    // Opens the bar panel.
    getMainController().searchAndOpenPanel(isArpeggio ? PanelType::arpeggioTable : PanelType::pitchTable);
}

void ExpressionTableListControllerImpl::onCreateNewExpressionCancelled() noexcept
{
    createNewExpressionView.reset();
}


// ============================================================

void ExpressionTableListControllerImpl::saveExpression() noexcept
{
    jassert(saveExpressionProcess == nullptr);          // Already instantiated?

    // Index of the expression to save.
    const auto indexOptional = getSelectedIndex();
    if (indexOptional.isAbsent()) {
        jassertfalse;       // Should have been detected before.
        return;
    }
    const auto expressionId = songController.getInstrumentId(indexOptional.getValue());
    if (expressionId.isAbsent()) {
        jassertfalse;
        return;
    }

    // Copies the expression.
    auto localExpression = songController.getExpression(isArpeggio, expressionId.getValueRef());

    // Performs the save.
    const auto compressXml = PreferencesManager::getInstance().areSavedFilesCompressed();
    saveExpressionProcess = std::make_unique<SaveExpression>(isArpeggio, localExpression, compressXml, [&](const bool success) { onSaveExpressionFinished(success); },
                                                             [&]() { onSaveExpressionCanceled(); });
    saveExpressionProcess->save();
}

void ExpressionTableListControllerImpl::onSaveExpressionFinished(bool /*success*/) noexcept
{
    onSaveExpressionCanceled();
}

void ExpressionTableListControllerImpl::onSaveExpressionCanceled() noexcept
{
    saveExpressionProcess.reset();
}


// ============================================================

void ExpressionTableListControllerImpl::loadExpression() noexcept
{
    jassert(loadExpressionProcess == nullptr);          // Already instantiated?

    // No need to check the index, it will be done after loading, and it has been checked before (and we cannot anywhere anyway).
    loadExpressionProcess = std::make_unique<LoadExpression>(isArpeggio,
                                                             [&](std::unique_ptr<Expression> expression) { onLoadExpressionFinished(std::move(expression)); },
                                                             [&]() { onLoadExpressionCanceled(); });
    loadExpressionProcess->load();
}

void ExpressionTableListControllerImpl::onLoadExpressionFinished(std::unique_ptr<Expression> loadedExpression) noexcept
{
    loadExpressionProcess.reset();

    if (loadedExpression == nullptr) {
        onLoadExpressionCanceled();
        return;
    }

    // Where to insert? Should be valid, checked before.
    const auto indexOptional = getSelectedIndex();
    if (indexOptional.isAbsent()) {
        jassertfalse;       // Should have been detected before.
        return;
    }
    const auto index = indexOptional.getValue() + 1;        // Inserts after.

    std::vector<std::unique_ptr<Expression>> expressions;
    expressions.push_back(std::move(loadedExpression));
    songController.insertExpressions(isArpeggio, index, std::move(expressions));
}

void ExpressionTableListControllerImpl::onLoadExpressionCanceled() noexcept
{
    loadExpressionProcess.reset();
}

}   // namespace arkostracker

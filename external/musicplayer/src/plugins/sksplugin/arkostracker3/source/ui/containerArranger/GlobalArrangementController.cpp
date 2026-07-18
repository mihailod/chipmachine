#include "GlobalArrangementController.h"

#include "../../app/preferences/PreferencesManager.h"
#include "../../controllers/MainController.h"
#include "ArrangerControllerImpl.h"
#include "factory/ArrangementFactory.h"

namespace arkostracker 
{

GlobalArrangementController::GlobalArrangementController(MainController& pMainController, juce::Component& pMainView, int pTopOffset) noexcept :
        mainController(pMainController),
        currentArrangementNumber(0),
        topOffset(pTopOffset),
        arrangerController(),
        mainView(pMainView),
        numberToStoredArrangement(),
        preferencesManager(PreferencesManager::getInstance())
{
    // Creates the ArrangerControllerHolder.
    arrangerController = std::make_unique<ArrangerControllerImpl>(pTopOffset, pMainView, pMainController);

    // Populates the Arrangements from the preferences (or creates default ones if not existing).
    retrieveStoredArrangements();
    // Displays a first Arrangement.
    applyLocalArrangement(0);
}

void GlobalArrangementController::retrieveStoredArrangements() noexcept
{
    numberToStoredArrangement.clear();

    for (auto index = 0; index < layoutCount; ++index) {
        auto layoutArrangementXml = preferencesManager.getLayoutArrangement(index);

        auto storedArrangement = createDefaultStoredArrangement(index);

        const auto newIndex = index;        // Only to make a warning shut-up.
        numberToStoredArrangement.insert(std::make_pair(newIndex, std::move(storedArrangement)));
    }
}


// StorageListener method implementations.
// =========================================

int GlobalArrangementController::getCurrentArrangementNumber() const
{
    return currentArrangementNumber;
}

void GlobalArrangementController::onRestoreArrangement(const int arrangementNumber)
{
    // Before restoring, the current local layout is saved. But not if it is the current one
    // (it allows to retrieve the last config if not satisfied with the current one (a mistake is made)).
    if (currentArrangementNumber != arrangementNumber) {
        // Saves the current arrangement in the current slot.
        storeCurrentLayoutLocally(currentArrangementNumber);

        currentArrangementNumber = arrangementNumber;
    }

    applyLocalArrangement(arrangementNumber);

    // Notifies the listeners.
    mainController.observers().getGeneralDataObservers().applyOnObservers([](GeneralDataObserver* observer) {
        observer->onGeneralDataChanged(GeneralDataObserver::What::layoutRestore);
    });
}


// PanelSearcher method implementations.
// ========================================

bool GlobalArrangementController::searchAndOpenPanel(const PanelType panelType) noexcept
{
    return arrangerController->searchAndOpenPanel(panelType, true);
}

bool GlobalArrangementController::isPanelVisible(const PanelType panelType) const noexcept
{
    return arrangerController->isPanelVisible(panelType);
}

// =========================================

const DocumentArrangement& GlobalArrangementController::getLocalArrangement(const int arrangementNumber) const noexcept
{
    const auto iterator = numberToStoredArrangement.find(arrangementNumber);
    jassert(iterator != numberToStoredArrangement.cend());          // Should never happen! We created all the entries with default values if needed.
    jassert(iterator->second != nullptr);                           // Should never happen! We create an entry but with a null object!
    return *iterator->second;
}

void GlobalArrangementController::applyArrangement(const DocumentArrangement& documentArrangement) noexcept
{
    // Creates the new Controller.
    arrangerController = std::make_unique<ArrangerControllerImpl>(topOffset, mainView, mainController);

    auto lastFocusOrder = -1;

    // Adds Line Containers.
    const auto& lineContainerArrangements = documentArrangement.getLineContainerArrangements();
    size_t lineContainerIndex = 0U;
    const auto lineContainerCount = lineContainerArrangements.size();
    for (const auto& lineContainerArrangement : lineContainerArrangements) {
        // For each Line Container, creates as many Containers as needed.
        const auto desiredHeight = lineContainerArrangement.getDesiredHeight();
        const auto height = desiredHeight.isPresent() ? desiredHeight.getValue() : Dimension::minimumValue;
        auto lineContainer = std::make_unique<LineContainer>(desiredHeight);     // Empty LineContainer.

        // Populates the Line Container with all its Containers, each with their tabs.
        const auto& containerArrangements = lineContainerArrangement.getContainerArrangements();
        size_t containerArrangementIndex = 0U;
        const auto containerArrangementCount = containerArrangements.size();
        for (const auto& containerArrangement : containerArrangements) {
            const auto focusOrder = containerArrangement.getFocusOrder();
            // Increases the last focus order if needed.
            if (focusOrder.isPresent()) {
                lastFocusOrder = std::max(lastFocusOrder, focusOrder.getValue());
            }

            const auto desiredWidth = containerArrangement.getDesiredWidth();
            const auto width = desiredWidth.getDesiredSize().isPresent() ? desiredWidth.getDesiredSize().getValue() : Dimension::minimumValue;
            auto container = std::make_unique<Container>(desiredWidth, *arrangerController, containerArrangement.isHiddenHeader(),
                                                         containerArrangement.getPanelTypes(),
                                                         containerArrangement.getSidePanelTypes(),
                                                         focusOrder);
            container->setSize(width, height);
            // Cannot last vertical handle (the first vertical handle is not handled here).
            const auto isLast = (containerArrangementIndex == (containerArrangementCount - 1U));
            lineContainer->addContainer(std::move(container), isLast);

            ++containerArrangementIndex;
        }

        lineContainer->setSize(lineContainer->getWidth(), height);
        const auto isLast = (lineContainerIndex == (lineContainerCount - 1U));
        arrangerController->addLineContainer(std::move(lineContainer), isLast);

        ++lineContainerIndex;
    }

    if (lastFocusOrder >= 0) {
        arrangerController->setLastFocusOrder(lastFocusOrder);
    }

    // Selects the panels from the stored data, making them all appear.
    for (const auto panelType : documentArrangement.getSelectedPanels()) {
        arrangerController->searchAndOpenPanel(panelType, false);
    }

    arrangerController->updateAllLayouts();

    // Focuses on a panel. // FIXME on app opening, nothing is focused anyway.
    if (mainView.isShowing() || mainView.isOnDesktop()) {     // To avoid a JUCE assertion on start of the application, the Main View is not ready yet.
        searchAndOpenPanel(documentArrangement.getFocusedPanel());
    }
}

std::unique_ptr<DocumentArrangement> GlobalArrangementController::createDefaultStoredArrangement(const int arrangementNumber) noexcept
{
    switch (arrangementNumber) {
        case 0:
            [[fallthrough]];
        default:
            return ArrangementFactory::createStoredArrangementMain();
        case 1:
            return ArrangementFactory::createStoredArrangementFocusOnPatternViewer();
        case 2:
            return ArrangementFactory::createStoredArrangementFocusOnExpressions();
        case 3:
            return ArrangementFactory::createStoredArrangementFocusOnListening();
    }
}

void GlobalArrangementController::storeArrangementLocally(const int number, const DocumentArrangement& arrangement) noexcept
{
    auto arrangementCopy = std::make_unique<DocumentArrangement>(arrangement);
    numberToStoredArrangement.at(number) = std::move(arrangementCopy);
}

DocumentArrangement GlobalArrangementController::buildArrangementFromCurrentLayout() const noexcept
{
    // One Document per Controller.
    DocumentArrangement documentArrangement;

    std::set<PanelType> selectedPanels;

    // LineContainers for each Document.
    const auto lineContainerSearchableViews = arrangerController->getSearchableViews();
    for (const auto* lineContainerSearchableView : lineContainerSearchableViews) {
        const auto lineContainerHeight = lineContainerSearchableView->getArrangerViewHeight();
        // If the line container is not locked, do not store its height, so that it is dynamic.
        const auto height = lineContainerHeight.isLocked() ? lineContainerHeight.getDesiredSize() : OptionalInt();

        LineContainerArrangement lineContainerArrangement(height);

        // Containers inside each LineContainer.
        for (const auto* containerSearchableView : lineContainerSearchableView->getSearchableViews()) {
            const auto currentDimension = containerSearchableView->getArrangerViewWidth();
            const auto currentWidth = containerSearchableView->getCurrentSize(true);
            // Stores the current width, so that we retrieve the same width when going back.
            const auto dimension = Dimension(currentWidth, currentDimension.getMinimum(), currentDimension.getMaximum());

            const auto isHiddenHeader = containerSearchableView->isHiddenHeader();
            const auto panelTypes = containerSearchableView->getPanelTypes();
            const auto sidePanelTypes = containerSearchableView->getSidePanelTypes();
            const auto focusOrder = containerSearchableView->getFocusOrder();

            selectedPanels.insert(containerSearchableView->getSelectedPanelType());     // Stores the selected panels to get them when going back.

            const ContainerArrangement containerArrangement(dimension, isHiddenHeader, panelTypes, sidePanelTypes, focusOrder);
            lineContainerArrangement.addContainerArrangement(containerArrangement);
        }

        documentArrangement.addLineContainerArrangement(lineContainerArrangement);

        // Stores the latest open panel type, to have it focused if going back to this panel.
        documentArrangement.setSelectedPanels(arrangerController->getLatestOpenedPanelType(), selectedPanels);
    }

    return documentArrangement;
}

void GlobalArrangementController::storeCurrentLayoutLocally(const int arrangementNumber) noexcept
{
    // Converts the current layout into an Arrangement.
    const auto arrangement = buildArrangementFromCurrentLayout();
    storeArrangementLocally(arrangementNumber, arrangement);
}

void GlobalArrangementController::applyLocalArrangement(const int arrangementNumber) noexcept
{
    const auto arrangement = getLocalArrangement(arrangementNumber);
    applyArrangement(arrangement);
}

}   // namespace arkostracker

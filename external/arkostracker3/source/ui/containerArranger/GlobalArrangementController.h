#pragma once

#include <memory>
#include <unordered_map>

#include "ArrangerController.h"
#include "PanelSearcher.h"
#include "StorageListener.h"
#include "storage/DocumentArrangement.h"

namespace arkostracker 
{

class MainController;
class PreferencesManager;

/**
 * This is the global controller of all that is "arrangement".
 * There is only one instance of it, it never dies.
 *
 * The layouts are all first deserialized from the Preferences, but then only these local layouts are
 * modified.
 * On exit the application, they are stored in the Preferences.
 */
class GlobalArrangementController final : public StorageListener,
                                          public PanelSearcher
{
public:
    /** How many layouts there are. */
    static constexpr auto layoutCount = 4;

    /**
     * Constructor.
     * @param mainController the Main Controller, used for the newly created Panels.
     * @param mainView the View of the main document. It is NOT owned by this Controller.
     * @param topOffset offset from the top.
     */
    GlobalArrangementController(MainController& mainController, juce::Component& mainView, int topOffset) noexcept;

    // StorageListener method implementations.
    // ==========================================
    int getCurrentArrangementNumber() const override;
    void onRestoreArrangement(int arrangementNumber) override;

    // PanelSearcher method implementations.
    // ========================================
    bool searchAndOpenPanel(PanelType panelType) noexcept override;
    bool isPanelVisible(PanelType panelType) const noexcept override;

private:
    /**
     * Gets the arrangement from this class, not from the Preferences It must exist.
     * It is not applied.
     * @param arrangementNumber the arrangement number.
     * @return the arrangement.
     */
    const DocumentArrangement& getLocalArrangement(int arrangementNumber) const noexcept;

    /**
     * Applies the given Arrangement. All the current Controllers will be deleted.
     * @param arrangement the arrangement to apply.
     */
    void applyArrangement(const DocumentArrangement& arrangement) noexcept;

    /** Retrieves the stored arrangements from the Preferences and populates the local data. */
    void retrieveStoredArrangements() noexcept;

    /**
     * Generates a LayoutArrangement from the current layout.
     * @return the arrangement.
     */
    DocumentArrangement buildArrangementFromCurrentLayout() const noexcept;

    /**
     * Stores the given StoredArrangement locally.
     * @param number the number of the arrangement.
     * @param arrangement the arrangement to store.
     */
    void storeArrangementLocally(int number, const DocumentArrangement& arrangement) noexcept;

    /**
     * Stores the current layout into an Arrangement. The storage is local.
     * @param arrangementNumber where to store the arrangement.
     */
    void storeCurrentLayoutLocally(int arrangementNumber) noexcept;

    /**
     * Applies the arrangement from its number, stored locally.
     * @param arrangementNumber the arrangement to apply.
     */
    void applyLocalArrangement(int arrangementNumber) noexcept;

    /**
     * @return a default Stored Arrangement according to the given arrangement number.
     * The arrangement may be the same between several (or all) number.
     */
    static std::unique_ptr<DocumentArrangement> createDefaultStoredArrangement(int arrangementNumber) noexcept;

    MainController& mainController;                                                             // The controller to reach the listeners.
    int currentArrangementNumber;                                                               // The current shown Arrangement number.
    int topOffset;                                                                              // The offset from the top.
    std::unique_ptr<ArrangerController> arrangerController;                                     // Controls the current document.
    juce::Component& mainView;                                                                  // The main window.

    std::unordered_map<int, std::unique_ptr<DocumentArrangement>> numberToStoredArrangement;    // Stores the Arrangements.
    PreferencesManager& preferencesManager;                                                     // The Preferences to store/retrieve app values.
};

}   // namespace arkostracker

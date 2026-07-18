#pragma once

#include <memory>

#include "../containerArranger/Panel.h"
#include "../utils/ParentViewLifeCycleAware.h"

namespace arkostracker 
{

class LinkerController;
class MainController;

/** A panel to show the Linker. */
class LinkerPanel final : public Panel
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener to get the panel events.
     */
    LinkerPanel(MainController& mainController, Panel::Listener& listener) noexcept;

    /** Destructor. */
    ~LinkerPanel() override;

    // Panel method implementations.
    // ================================
    PanelType getType() const noexcept override;
    void getKeyboardFocus() noexcept override;

    // Component method implementations.
    // ===================================
    void resized() override;

private:
    LinkerController& linkerController;
    bool hasBeenSized;                      // True once the parent has a size.
};

}   // namespace arkostracker


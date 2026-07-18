#pragma once

#include "../containerArranger/Panel.h"

namespace arkostracker 
{

class TestAreaController;
class MainController;

/** A panel to show the Test Area. */
class TestAreaPanel final : public Panel
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener to get the panel events.
     */
    TestAreaPanel(MainController& mainController, Listener& listener) noexcept;

    /** Destructor. */
    ~TestAreaPanel() noexcept override;

    // Panel method implementations.
    // ================================
    PanelType getType() const noexcept override;
    void getKeyboardFocus() noexcept override;

    // Component method implementations.
    // ================================
    void resized() override;

private:
    TestAreaController& controller;
};

}   // namespace arkostracker

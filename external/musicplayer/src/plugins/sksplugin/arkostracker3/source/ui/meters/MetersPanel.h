#pragma once

#include <memory>

#include "MetersListener.h"
#include "../../utils/WithParent.h"
#include "../components/ThemedColoredImage.h"
#include "../containerArranger/Panel.h"

#include "controller/MetersController.h"

namespace arkostracker 
{

class MainController;

/** A panel to show the VU-Meters. */
class MetersPanel final : public Panel,
                          public MetersListener
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener to get the panel events.
     */
    explicit MetersPanel(MainController& mainController, Listener& listener) noexcept;

    // Panel method implementations.
    // =======================================
    PanelType getType() const noexcept override;
    void getKeyboardFocus() noexcept override;

    // Component method implementations.
    // =======================================
    void resized() override;

    // MetersListener method implementations.
    // =======================================
    void onMeterSwitchPerformed() noexcept override;

private:
    class CustomMouseListener final : public MouseListener,
                                      public WithParent<MetersPanel>
    {
    public:
        explicit CustomMouseListener(MetersPanel& parent) : WithParent(parent) {}
        void mouseEnter(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;
    };
    std::unique_ptr<MetersController> metersController;

    CustomMouseListener customMouseListener;
    ThemedColoredImage switchButton;
};

}   // namespace arkostracker

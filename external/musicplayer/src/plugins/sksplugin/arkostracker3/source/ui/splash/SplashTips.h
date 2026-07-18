#pragma once

#include <functional>

#include "../components/BlankComponent.h"
#include "../components/dialogs/CustomResizableWindow.h"
#include "../utils/zippedImages/ZippedImages.h"
#include "SplashDialog.h"
#include "tips/TipsController.h"

namespace arkostracker 
{

class MainController;

/** A splash which shows random tips. It manages itself the preferences (if the user don't want to see this anymore for example). */
class SplashTips : public SplashDialog,
                   public CustomResizableWindow
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param callback the callback when done.
     */
    explicit SplashTips(const MainController& mainController, std::function<void()> callback) noexcept;

    // SplashDialog method implementations.
    // ==========================================================
    void show() noexcept override;

    // Component method implementations.
    // ==========================================================
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    static constexpr auto margins = 15;
    static constexpr auto videoFrameDurationMs = 100;

    /** Saves the "show on startup" flag and calls the callback. */
    void exitSplash() const noexcept;

    /** Shows the next tip. */
    void showNextTip() noexcept;

    /** Displays the given tip. */
    void displayTip(const Tip& tip) noexcept;

    std::function<void()> callback;

    TipsController tipsController;

    juce::Component contentComponent;                   // Where all the views are added.
    juce::ImageComponent logoImage;
    juce::Label tipOfTheDayLabel;
    BlankComponent separatorTop;
    BlankComponent separatorBottom;
    juce::Label text;
    std::unique_ptr<juce::ImageComponent> tipImage;
    std::unique_ptr<ZippedImages> tipZippedImages;
    juce::ToggleButton showSplashToggleButton;
    juce::Label versionLabel;
    juce::TextButton okButton;
    juce::TextButton nextTipButton;
};

}   // namespace arkostracker

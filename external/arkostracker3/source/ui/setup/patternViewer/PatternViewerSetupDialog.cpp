#include "PatternViewerSetupDialog.h"

#include <unordered_set>

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../business/patternViewer/DisplayedEffects.h"
#include "../../../controllers/MainController.h"
#include "../../../utils/NumberUtil.h"

namespace arkostracker 
{

const int PatternViewerSetupDialog::minimumFontSize = 10;
const int PatternViewerSetupDialog::maximumFontSize = 30;
const int PatternViewerSetupDialog::minimumMouseWheelSteps = 1;
const int PatternViewerSetupDialog::maximumMouseWheelSteps = 16;

const juce::String PatternViewerSetupDialog::propertyEffect = "effect";             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

PatternViewerSetupDialog::PatternViewerSetupDialog(MainController& pMainController, std::function<void()> pOkCallback,
                                                   std::function<void()> pCancelCallback) noexcept :
        ModalDialog(juce::translate("Pattern viewer"), 310, 570,
                    [&] { onOkButtonClicked(); },
                    std::move(pCancelCallback),
                    true, true),
        mainController(pMainController),
        okCallback(std::move(pOkCallback)),
        toggleShowHexadecimal(juce::translate("Show hexadecimal line numbers")),
        toggleZoomMiddleLine(juce::translate("Zoom the middle line")),
        textSizeSlider(*this, juce::translate("Text size"), 2, 2.0, 4.0,
                       SliderIncDec::Filter::integer, 200),
        mouseWheelStepsSlider(*this, juce::translate("Mouse wheel speed"), 2, 1.0, 2.0,
                       SliderIncDec::Filter::integer, 200),
        group(juce::translate("Effect names")),
        effectNameLabels(),
        effectComboBoxes(),
        originalLabelsColor(juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::Label::ColourIds::textColourId)),
        resetToDefaultButton(juce::translate("Reset to defaults")),
        preferences(PreferencesManager::getInstance()),
        changed(false)
{
    const auto bounds = getUsableModalDialogBounds();
    const auto x = bounds.getX();
    auto y = bounds.getY();
    const auto usableWidth = bounds.getWidth();
    const auto itemHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;
    const auto itemHeightToPass = itemHeight + margins;

    const auto labelsWidth = textSizeSlider.getLabelWidth();     // Since it was calculated earlier, use the same value.
    constexpr auto groupHeight = 300;
    constexpr auto resetToDefaultButtonWidth = 150;
    const auto slidersWidth = labelsWidth + 90;

    toggleShowHexadecimal.setBounds(x, y, usableWidth, itemHeight);
    toggleShowHexadecimal.onClick = [this] { onToggledChanged(); };
    y += itemHeight;
    toggleZoomMiddleLine.setBounds(toggleShowHexadecimal.getX(), y, usableWidth, itemHeight);
    toggleZoomMiddleLine.onClick = [this] { onToggledChanged(); };

    y += itemHeightToPass;
    textSizeSlider.setBounds(x, y, slidersWidth, itemHeight);
    y += itemHeightToPass;
    mouseWheelStepsSlider.setBounds(x, y, slidersWidth, itemHeight);
    y += itemHeightToPass + margins;
    group.setBounds(x, y, usableWidth, groupHeight);
    resetToDefaultButton.setBounds(x + usableWidth - resetToDefaultButtonWidth, group.getBottom() + margins, resetToDefaultButtonWidth, itemHeight);
    resetToDefaultButton.onClick = [&] { onResetToDefaultClicked(); };

    addComponentToModalDialog(toggleShowHexadecimal);
    addComponentToModalDialog(toggleZoomMiddleLine);
    addComponentToModalDialog(textSizeSlider);
    addComponentToModalDialog(mouseWheelStepsSlider);
    addComponentToModalDialog(group);
    addComponentToModalDialog(resetToDefaultButton);

    // Applies the data from the Preferences.
    const auto isHexadecimal = preferences.isPatternViewerLineNumberInHexadecimal();
    const auto isZoomMiddleLine = preferences.getPatternViewerMustZoomMiddleLine();
    toggleShowHexadecimal.setToggleState(isHexadecimal, juce::NotificationType::dontSendNotification);
    toggleZoomMiddleLine.setToggleState(isZoomMiddleLine, juce::NotificationType::dontSendNotification);
    textSizeSlider.setShownValue(static_cast<double>(preferences.getPatternViewerFontHeight()));
    mouseWheelStepsSlider.setShownValue(static_cast<double>(preferences.getPatternViewerMouseWheelSteps()));
    displayEffects(preferences.getEffectToChar());
}


// SliderIncDec::Listener method implementations.
// =================================================
void PatternViewerSetupDialog::onWantToChangeSliderValue(SliderIncDec& slider, const double value)
{
    if (&slider == &textSizeSlider) {
        onUserWantsNewFontSize(static_cast<int>(value));
    } else if (&slider == &mouseWheelStepsSlider) {
        onUserWantsNewMouseWheelSteps(static_cast<int>(value));
    } else {
        jassertfalse;           // Slider not managed?
    }
}


// =================================================

void PatternViewerSetupDialog::onToggledChanged() noexcept
{
    changed = true;
}

void PatternViewerSetupDialog::onUserWantsNewFontSize(const int desiredFontSize) noexcept
{
    changed = true;

    // Corrects the font size and updates the UI.
    const auto fontSize = static_cast<unsigned int>(NumberUtil::correctNumber(desiredFontSize, minimumFontSize, maximumFontSize)) & 0b11111110U;
    textSizeSlider.setShownValue(static_cast<const double>(fontSize));
}

void PatternViewerSetupDialog::onUserWantsNewMouseWheelSteps(const int desiredSteps) noexcept
{
    changed = true;

    // Corrects the font size and updates the UI.
    const auto steps = NumberUtil::correctNumber(desiredSteps, minimumMouseWheelSteps, maximumMouseWheelSteps);
    mouseWheelStepsSlider.setShownValue(static_cast<const double>(steps));
}

void PatternViewerSetupDialog::notifyChange() const noexcept
{
    mainController.observers().getPatternViewerMetadataObservers().applyOnObservers(
            [] (PatternViewerMetadataObserver* observer) { observer->onPatternViewerMetadataChanged(); }
    );
}

void PatternViewerSetupDialog::displayEffects(const std::unordered_map<Effect, juce::juce_wchar>& effectToChar) noexcept
{
    group.removeAllGroupChildren();     // Security.

    // Shows in this order.
    static const auto effects = std::vector {
        Effect::reset,
        Effect::pitchUp,
        Effect::pitchDown,
        Effect::fastPitchUp,
        Effect::fastPitchDown,
        Effect::pitchGlide,
        Effect::pitchTable,
        Effect::volume,
        Effect::volumeIn,
        Effect::volumeOut,
        Effect::arpeggioTable,
        Effect::arpeggio3Notes,
        Effect::arpeggio4Notes,
        Effect::forceInstrumentSpeed,
        Effect::forceArpeggioSpeed,
        Effect::forcePitchTableSpeed,
    };
    jassert(static_cast<int>(effects.size()) == static_cast<int>(Effect::lastEffect2_0) - 1);       // Forgot an effect?

    const auto bounds = group.getGroupInnerArea();
    const auto width = bounds.getWidth();

    auto y = 0;
    constexpr auto margin = 10;
    constexpr auto labelsHeight = 25;
    constexpr auto comboBoxesWidth = 60;
    constexpr auto comboBoxesHeight = labelsHeight;
    const auto comboBoxesX = width - comboBoxesWidth - (margin * 4);
    constexpr auto itemsHeight = labelsHeight + (margin / 2);

    for (const auto lEffect : effects) {
        constexpr auto labelsX = 0;
        constexpr auto labelsWidth = 160;
        const auto effectName = EffectUtil::toDisplayedString(lEffect);
        jassert(effectName.isNotEmpty());

        auto label = std::make_unique<juce::Label>(juce::String(), effectName);
        label->setBounds(labelsX, y, labelsWidth, labelsHeight);
        group.addComponentToGroup(*label);

        // Builds the ComboBox.
        auto comboBox = std::make_unique<juce::ComboBox>(juce::String());
        comboBox->setBounds(comboBoxesX, y, comboBoxesWidth, comboBoxesHeight);
        group.addComponentToGroup(*comboBox);
        fillComboBox(*comboBox, *label, lEffect, effectToChar);
        comboBox->onChange = [&] { onComboBoxChanged(); };

        effectNameLabels.push_back(std::move(label));
        effectComboBoxes.push_back(std::move(comboBox));

        y += itemsHeight;
    }
}

void PatternViewerSetupDialog::fillComboBox(juce::ComboBox& comboBox, juce::Label& label, Effect effect, const std::unordered_map<Effect, juce::juce_wchar>& effectToChar) noexcept
{
    // Fills with letters.
    static const std::vector<juce::juce_wchar> chars = {
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
            'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
            'y', 'z',
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    };

    // Finds the related char.
    juce::juce_wchar relatedChar;    // NOLINT(*-init-variables)
    const auto iterator = effectToChar.find(effect);
    if (iterator != effectToChar.cend()) {
        relatedChar = iterator->second;
    } else {
        relatedChar = chars.at(0U);              // Takes the first one...
        jassertfalse;                           // No effect linked??
    }

    // Fills the ComboBox.
    for (const auto c : chars) {
        comboBox.addItem(juce::String::charToString(c), static_cast<int>(c));     // The ID is the char itself! Handy. Cast needed for ARM.
    }
    // Selects the right char. Cast needed for ARM.
    comboBox.setSelectedId(static_cast<int>(relatedChar), juce::NotificationType::dontSendNotification);

    // Uses a property to link the ComboBox to its effect, as well as the labels (for coloring).
    comboBox.getProperties().set(propertyEffect, static_cast<int>(effect));
    label.getProperties().set(propertyEffect, static_cast<int>(effect));
}

void PatternViewerSetupDialog::onOkButtonClicked() const noexcept
{
    // Saves the data if needed, and notifies.
    if (changed) {
        const auto showHexadecimal = toggleShowHexadecimal.getToggleState();
        preferences.setPatternViewerLineNumberInHexadecimal(showHexadecimal);

        const auto zoomMiddleLine = toggleZoomMiddleLine.getToggleState();
        preferences.setPatternViewerMustZoomMiddleLine(zoomMiddleLine);

        const auto fontSize = textSizeSlider.getValue();
        preferences.setPatternViewerFontHeight(static_cast<const int>(fontSize));

        const auto steps = mouseWheelStepsSlider.getValue();
        preferences.setPatternViewerMouseWheelSteps(static_cast<const int>(steps));

        // Builds the map of the effect to char.
        std::unordered_map<Effect, juce::juce_wchar> effectToChar;
        for (const auto& comboBox : effectComboBoxes) {
            const auto* var = comboBox->getProperties().getVarPointer(propertyEffect);
            const auto lEffect = static_cast<const Effect>(var->operator int());

            const auto c = comboBox->getSelectedId();       // The ID is the char.
            effectToChar.insert(std::make_pair(lEffect, static_cast<const juce::juce_wchar>(c)));
        }
        preferences.setEffectToChar(effectToChar);

        notifyChange();
    }

    okCallback();
}

void PatternViewerSetupDialog::onComboBoxChanged() noexcept
{
    changed = true;

    checkErrorsAndUpdateColors();
}

void PatternViewerSetupDialog::checkErrorsAndUpdateColors() noexcept
{
    auto effectsInError = std::unordered_set<Effect>();
    auto browsedInts = std::unordered_set<int>();

    // Checks that there are no duplicate.
    for (const auto& comboBox : effectComboBoxes) {
        const auto* var = comboBox->getProperties().getVarPointer(propertyEffect);
        const auto lEffect = static_cast<const Effect>(var->operator int());

        const auto selectedInt = comboBox->getSelectedId();
        // Does this int (=char) already known? If yes, error.
        if (browsedInts.find(selectedInt) == browsedInts.cend()) {
            browsedInts.insert(selectedInt);
        } else {
            // Error.
            effectsInError.insert(lEffect);
        }
    }

    const auto isError = !effectsInError.empty();

    // Now updates the colors of all the labels.
    for (const auto& label : effectNameLabels) {
        const auto* var = label->getProperties().getVarPointer(propertyEffect);
        const auto lEffect = static_cast<const Effect>(var->operator int());

        const auto isLabelInError = (effectsInError.find(lEffect) != effectsInError.cend());
        const auto color = isLabelInError ? juce::Colours::red : originalLabelsColor;
        label->setColour(juce::Label::ColourIds::textColourId, color);
    }

    // If error, disables the OK button.
    setOkButtonEnable(!isError);
}

void PatternViewerSetupDialog::onResetToDefaultClicked() noexcept
{
    changed = true;

    // Put the data back from default.
    const auto& defaultEffectToChar = DisplayedEffects::getDefaultEffectToDisplayedChar();
    for (const auto& comboBox : effectComboBoxes) {
        const auto* var = comboBox->getProperties().getVarPointer(propertyEffect);
        const auto lEffect = static_cast<const Effect>(var->operator int());

        const auto iterator = defaultEffectToChar.find(lEffect);
        if (iterator == defaultEffectToChar.cend()) {
            jassertfalse;       // Should never happen!
        } else {
            const auto c = iterator->second;
            comboBox->setSelectedId(static_cast<int>(c), juce::NotificationType::dontSendNotification);
        }
    }

    // Updates the UI.
    checkErrorsAndUpdateColors();
}

}   // namespace arkostracker

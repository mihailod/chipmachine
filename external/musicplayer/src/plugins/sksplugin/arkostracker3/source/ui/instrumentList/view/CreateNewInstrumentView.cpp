#include "CreateNewInstrumentView.h"

#include "../../../utils/FileExtensions.h"
#include "../../components/FileChooserCustom.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

CreateNewInstrumentView::CreateNewInstrumentView(Listener& pListener, const juce::Colour& pInitialColor) noexcept :
        listener(pListener),
        modalDialog(std::make_unique<ModalDialog>(juce::translate("Create new instrument"),
                                                  400, 260,
                                                  [&] { onDialogValidated(); },
                                                  [&] { onDialogCancelled(); },
                                                  true, true)),
        colorChooser(),

        nameLabel(juce::String(), juce::translate("Name")),
        nameTextEditor(),

        colorLabel(juce::String(), juce::translate("Color")),
        colorView(*this, pInitialColor),

        typeLabel(juce::String(), juce::translate("Instrument type")),
        typeComboBox(),

        psgInstrumentTemplateLabel(juce::String(), juce::translate("Template")),
        psgInstrumentTemplateComboBox(),

        sampleFilePathChooser(juce::translate("Path to WAV (drag'n'drop possible)"), juce::translate("Open a WAV file"), FileExtensions::wavExtensionWithWildcard)
{
    // Locates the UI items.
    const auto bounds = modalDialog->getUsableModalDialogBounds();
    const auto margins = LookAndFeelConstants::margins / 2;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    const auto width = bounds.getWidth();
    const auto x = bounds.getX();
    const auto top = bounds.getY() + margins;       // Added a small margins, else seems too close.

    // The colors on the top-right.
    constexpr auto colorLabelWidth = 45;
    const auto colorLabelX = width - colorLabelWidth + 10;  // Why add this offset? Doesn't look good if I don't.
    constexpr auto colorViewWidth = 20;
    constexpr auto colorViewHeight = colorViewWidth;
    colorLabel.setBounds(colorLabelX, top, colorLabelWidth, labelsHeight);
    colorLabel.setJustificationType(juce::Justification::centred);
    // The ColorView X is in the middle of the color label.
    colorView.setBounds(colorLabelX + ((colorLabelWidth - colorViewWidth) / 2),
                        colorLabel.getBottom() + ((labelsHeight - colorViewHeight) / 2), colorViewWidth, colorViewHeight);

    // The name is on the left of the Color Views.
    nameLabel.setBounds(x, top, colorLabel.getX() - static_cast<int>(static_cast<double>(margins) * 1.2), labelsHeight);
    nameTextEditor.setBounds(x, nameLabel.getBottom(), nameLabel.getWidth(), labelsHeight);
    nameTextEditor.setText(juce::translate("New"), false);
    nameTextEditor.onEscapeKey = [&] { onDialogCancelled(); };
    nameTextEditor.onReturnKey = [&] { onDialogValidated(); };

    // Populates the Type ComboBox. This is static, so no need to do complicated stuff.
    typeLabel.setBounds(x, nameTextEditor.getBottom() + margins, width, labelsHeight);
    typeComboBox.setBounds(x, typeLabel.getBottom(), width, labelsHeight);
    typeComboBox.addItem(juce::translate("PSG"), idDropdownInstrumentPsg);
    typeComboBox.addItem(juce::translate("Sample"), idDropdownInstrumentSample);
    typeComboBox.onChange = [&] { onInstrumentTypeChanged(); };

    // The PSG/sample specific components overlap, but they will switch according to the selected instrument type.
    psgInstrumentTemplateLabel.setBounds(x, typeComboBox.getBottom() + (margins * 2), width, labelsHeight);
    psgInstrumentTemplateComboBox.setBounds(x, psgInstrumentTemplateLabel.getBottom(), width, labelsHeight);

    sampleFilePathChooser.setBounds(x, psgInstrumentTemplateLabel.getY(), width, FilePathChooser::desiredHeight);

    fillPsgInstrumentTemplateComboBox();

    modalDialog->addComponentToModalDialog(nameLabel);
    modalDialog->addComponentToModalDialog(nameTextEditor);
    modalDialog->addComponentToModalDialog(colorLabel);
    modalDialog->addComponentToModalDialog(colorView);
    modalDialog->addComponentToModalDialog(typeLabel);
    modalDialog->addComponentToModalDialog(typeComboBox);
    modalDialog->addComponentToModalDialog(psgInstrumentTemplateLabel);
    modalDialog->addComponentToModalDialog(psgInstrumentTemplateComboBox);
    modalDialog->addComponentToModalDialog(sampleFilePathChooser, false);

    typeComboBox.setSelectedItemIndex(0, juce::NotificationType::sendNotification);

    ModalDialog::giveFocus(nameTextEditor);
}

void CreateNewInstrumentView::onDialogValidated() const noexcept
{
    // Calls the listener according to instrument type.
    switch (typeComboBox.getSelectedId()) {
        default:
            jassertfalse;           // Unknown type??
        case idDropdownInstrumentPsg: {
            const auto data = PsgInstrumentData(nameTextEditor.getText(), colorView.getColor(),
               static_cast<GenericInstrumentGenerator::TemplateId>(psgInstrumentTemplateComboBox.getSelectedId() - templateIdOffset));
            listener.onCreateNewPsgInstrumentValidated(data);
            break;
        }
        case idDropdownInstrumentSample: {
            const auto data = SampleInstrumentData(nameTextEditor.getText(), colorView.getColor(), sampleFilePathChooser.getSelectedFile().getFullPathName());
            listener.onCreateNewSampleInstrumentValidated(data);
            break;
        }
    }
}

void CreateNewInstrumentView::onDialogCancelled() const noexcept
{
    listener.onCreateNewInstrumentCancelled();
}

void CreateNewInstrumentView::fillPsgInstrumentTemplateComboBox() noexcept
{
    const auto& templateIdToName = GenericInstrumentGenerator::getTemplateIdToDisplayableName();
    for (const auto& [templateId, name] : templateIdToName) {
        psgInstrumentTemplateComboBox.addItem(name, static_cast<int>(templateId) + templateIdOffset);
    }
    psgInstrumentTemplateComboBox.setSelectedItemIndex(static_cast<int>(GenericInstrumentGenerator::TemplateId::singleBeep) + templateIdOffset,
            juce::NotificationType::dontSendNotification);
}

void CreateNewInstrumentView::onInstrumentTypeChanged() noexcept
{
    const auto isPsgSelected = (typeComboBox.getSelectedId() == idDropdownInstrumentPsg);
    const auto isSampleSelected = (typeComboBox.getSelectedId() == idDropdownInstrumentSample);

    psgInstrumentTemplateLabel.setVisible(isPsgSelected);
    psgInstrumentTemplateComboBox.setVisible(isPsgSelected);

    sampleFilePathChooser.setVisible(isSampleSelected);
}


// ColorView::Listener method implementations.
// ===============================================

void CreateNewInstrumentView::onColorClicked(const juce::Colour& color, int /*viewId*/) noexcept
{
    jassert(colorChooser == nullptr);           // Already opened?

    colorChooser = std::make_unique<ColorChooser>(*this, &color);
}


// ColorChooser::Listener method implementations.
// =================================================

void CreateNewInstrumentView::onColorSelectedInColorChooser(const juce::Colour color) noexcept
{
    colorChooser.reset();

    colorView.setColorAndRepaint(color);
}

void CreateNewInstrumentView::onColorInColorChooserCancelled() noexcept
{
    colorChooser.reset();
}

}   // namespace arkostracker

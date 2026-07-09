#pragma once

#include <memory>
#include <utility>

#include "../../../business/instrument/GenericInstrumentGenerator.h"
#include "../../components/EditText.h"
#include "../../components/FilePathChooser.h"
#include "../../components/colors/ColorChooser.h"
#include "../../components/colors/ColorView.h"
#include "../../components/dialogs/ModalDialog.h"

namespace arkostracker 
{

/**
 * Shows a Dialog to choose the data of the new Instrument (name, color, etc.).
 * Not complex enough to really implement MVI. Passively asks for the Controller to ask for data, once it has been notified
 * the instrument is validated by the user.
 */
class CreateNewInstrumentView final : public ColorView::Listener,
                                      public ColorChooser::Listener
{
public:

    /** The data to allow to create the Psg instrument. */
    class PsgInstrumentData
    {
    public:
        explicit PsgInstrumentData(juce::String pName, const juce::Colour pColor, const GenericInstrumentGenerator::TemplateId pTemplateId) :
                name(std::move(pName)),
                color(pColor),
                templateId(pTemplateId)
        {
        }

        juce::String getName() const noexcept
        {
            return name;
        }

        juce::Colour getColor() const noexcept
        {
            return color;
        }

        GenericInstrumentGenerator::TemplateId getTemplateId() const noexcept
        {
            return templateId;
        }

    private:
        juce::String name;
        juce::Colour color;
        GenericInstrumentGenerator::TemplateId templateId;
    };

    /** The data to allow to create the Sample instrument. */
    class SampleInstrumentData
    {
    public:
        explicit SampleInstrumentData(juce::String pName, const juce::Colour pColor, juce::String pSamplePath) :
                name(std::move(pName)),
                color(pColor),
                samplePath(std::move(pSamplePath))
        {
        }

        juce::String getName() const noexcept
        {
            return name;
        }

        juce::Colour getColor() const noexcept
        {
            return color;
        }

        juce::String getSamplePath() const noexcept
        {
            return samplePath;
        }

    private:
        juce::String name;
        juce::Colour color;
        juce::String samplePath;
    };

    /** Listener to the events of this class. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the user wants to create a PSG Instrument.
         * @param psgData data to create the PSG Instrument.
         */
        virtual void onCreateNewPsgInstrumentValidated(const PsgInstrumentData& psgData) noexcept = 0;

        /**
         * Called when the user wants to create a Sample Instrument.
         * @param sampleData data to create the Sample Instrument.
         */
        virtual void onCreateNewSampleInstrumentValidated(const SampleInstrumentData& sampleData) noexcept = 0;

        /** Called when the user cancelled the creation of the instrument. */
        virtual void onCreateNewInstrumentCancelled() noexcept = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to the events.
     * @param initialColor the color to show at first.
     */
    CreateNewInstrumentView(Listener& listener, const juce::Colour& initialColor) noexcept;

    // ColorView::Listener method implementations.
    // ===============================================
    void onColorClicked(const juce::Colour& color, int viewId) noexcept override;

    // ColorChooser::Listener method implementations.
    // =================================================
    void onColorSelectedInColorChooser(juce::Colour color) noexcept override;
    void onColorInColorChooserCancelled() noexcept override;

private:
    static constexpr auto idDropdownInstrumentPsg = 1;
    static constexpr auto idDropdownInstrumentSample = idDropdownInstrumentPsg + 1;

    static constexpr auto templateIdOffset = 1;         // Juce ComboBox cannot handle ID 0.

    /** Called when the name is validated in the dialog. */
    void onDialogValidated() const noexcept;
    /** Called if the dialog has been canceled. */
    void onDialogCancelled() const noexcept;

    /** Fills the combobox with the PSG Instrument templates. */
    void fillPsgInstrumentTemplateComboBox() noexcept;
    /** The instrument type changed in the combobox. */
    void onInstrumentTypeChanged() noexcept;

    Listener& listener;

    std::unique_ptr<ModalDialog> modalDialog;
    std::unique_ptr<ColorChooser> colorChooser;         // The dialog to choose a color.

    juce::Label nameLabel;                              // Label for the name of the Instrument.
    EditText nameTextEditor;                            // Where the name of the Instrument can be written.

    juce::Label colorLabel;                             // Label for the color of the Instrument.
    ColorView colorView;                                // Shows the color of the Instrument.

    juce::Label typeLabel;                              // Label for the type of the Instrument to create.
    juce::ComboBox typeComboBox;                        // To choose the type of the Instrument to create.

    juce::Label psgInstrumentTemplateLabel;
    juce::ComboBox psgInstrumentTemplateComboBox;

    FilePathChooser sampleFilePathChooser;
};

}   // namespace arkostracker

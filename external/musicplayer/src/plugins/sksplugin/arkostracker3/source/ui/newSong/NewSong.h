#pragma once

#include <memory>

#include "../../song/Song.h"
#include "NewSongDialog.h"

namespace arkostracker 
{

/** Manages the creation of a new song. */
class NewSong final : public NewSongDialog::Listener
{
public:
    /** To be aware of the events of the Dialog. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called if the user has validated a new song, or canceled.
         * @param song the new song, or nullptr if canceled.
         */
        virtual void onNewSongValidatedOrCanceled(std::unique_ptr<Song> song) noexcept = 0;
    };

    /**
     * Constructor. It does not show the Dialog.
     * @param listener the listener to the events of this class.
     */
    explicit NewSong(Listener& listener) noexcept;

    /** Shows the dialog. */
    void showDialog() noexcept;
    /** Hides the dialog. */
    void hideDialog() noexcept;

    // NewSongDialog::Listener method implementations.
    // ==================================================
    void onNewSongParametersValidated(int selectedTemplateId) noexcept override;
    void onNewSongParametersCanceled() noexcept override;

private:
    enum class TemplateId : std::uint8_t
    {
        AmstradCpc = 1,         // Must be >0, as JUCE reserves the 0.
        AmstradCpcPlaycity,
        Pcw,
        PcwTurboSound,
        Spectrum,
        SpectrumTurboSound,
        SpecNext,
        Pentagon,
        Msx,
        MsxFpgaDarky,
        SpectravideoSvi3x8,
        AtariSt,
        AtariXeXl,
        Apple2,
        Oric,
        Vectrex,
        SharpMZ700,
    };
    static const std::map<int, juce::String>& buildIdToTemplateName() noexcept;

    /**
     * @return the PSGs to use according to the given TemplateId.
     * @param templateId the template id.
     */
    static std::vector<Psg> createPsgsFromTemplate(TemplateId templateId) noexcept;
    /**
     * @return the PSGs to use according to the given TemplateId.
     * @param templateId the template id.
     */
    static SidPlayerCapability createSidCapabilityFromTemplate(TemplateId templateId) noexcept;

    Listener& listener;

    std::map<int, juce::String> idToTemplateName;
    TemplateId selectedTemplateId;                          // To show the previous template.

    std::unique_ptr<ModalDialog> dialog;
};

}   // namespace arkostracker

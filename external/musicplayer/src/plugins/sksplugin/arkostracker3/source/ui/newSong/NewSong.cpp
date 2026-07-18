#include "NewSong.h"

#include "../../business/instrument/GenericInstrumentGenerator.h"
#include "../../song/subsong/SubsongConstants.h"
#include "NewSongDialog.h"

namespace arkostracker 
{

NewSong::NewSong(Listener& pListener) noexcept :
        listener(pListener),
        idToTemplateName(buildIdToTemplateName()),
        selectedTemplateId(TemplateId::AmstradCpc),
        dialog()
{
}


// NewSongDialog::Listener method implementations.
// ==================================================

void NewSong::onNewSongParametersValidated(int newSelectedTemplateId) noexcept
{
    hideDialog();

    selectedTemplateId = static_cast<TemplateId>(newSelectedTemplateId);

    // Builds a Song, using the TemplateId to get the right PSG(s).
    auto song = std::make_unique<Song>(juce::translate("Untitled"), juce::translate("Unknown"), juce::translate("Unknown"),
                                       juce::String(), false);

    // Subsong.
    const auto psgs = createPsgsFromTemplate(selectedTemplateId);
    const auto sidPlayerCapability = createSidCapabilityFromTemplate(selectedTemplateId);
    auto subsong = std::make_unique<Subsong>(SubsongConstants::defaultFirstSubsongName, SubsongConstants::defaultSpeed, PsgFrequency::defaultReplayFrequencyHz,
        SubsongConstants::defaultDigiChannel, SubsongConstants::defaultPrimaryHighlight, SubsongConstants::defaultSecondaryHighlight,
                                             0, 0, psgs, sidPlayerCapability, true);
    song->addSubsong(std::move(subsong));

    // Instruments and Expressions.
    song->addEmptyInstrumentAndExpressions();
    song->addInstrument(GenericInstrumentGenerator::generateInstrumentForDecreasingVolume(juce::translate("First")));

    listener.onNewSongValidatedOrCanceled(std::move(song));
}

void NewSong::onNewSongParametersCanceled() noexcept
{
    hideDialog();

    listener.onNewSongValidatedOrCanceled(nullptr);
}


// ==================================================

void NewSong::showDialog() noexcept
{
    jassert(dialog == nullptr);         // Already present?

    if (dialog == nullptr) {
        dialog = std::make_unique<NewSongDialog>(*this, idToTemplateName, static_cast<int>(selectedTemplateId));
    }
}

void NewSong::hideDialog() noexcept
{
    jassert(dialog != nullptr);         // Already removed?
    dialog.reset();
}

const std::map<int, juce::String>& NewSong::buildIdToTemplateName() noexcept
{
    static const std::map<int, juce::String> idToTemplateNameMap = {
            { static_cast<int>(TemplateId::AmstradCpc),         juce::translate("Amstrad CPC") },
            { static_cast<int>(TemplateId::AmstradCpcPlaycity), juce::translate("Amstrad CPC + Playcity (9 channels)") },
            { static_cast<int>(TemplateId::Pcw),                juce::translate("Amstrad PCW") },
            { static_cast<int>(TemplateId::PcwTurboSound),      juce::translate("Amstrad PCW + TurboSound (6 channels)") },
            { static_cast<int>(TemplateId::Spectrum),           juce::translate("ZX Spectrum") },
            { static_cast<int>(TemplateId::SpectrumTurboSound), juce::translate("ZX Spectrum + TurboSound (6 channels)") },
            { static_cast<int>(TemplateId::SpecNext),           juce::translate("SpecNext (9 channels)") },
            { static_cast<int>(TemplateId::Pentagon),           juce::translate("Pentagon") },
            { static_cast<int>(TemplateId::Msx),                juce::translate("MSX") },
            { static_cast<int>(TemplateId::MsxFpgaDarky),       juce::translate("MSX + FPGA/Darky (6 channels)") },
            { static_cast<int>(TemplateId::SpectravideoSvi3x8), juce::translate("Spectravideo SVI 318/328") },
            { static_cast<int>(TemplateId::AtariSt),            juce::translate("Atari ST") },
            { static_cast<int>(TemplateId::AtariXeXl),          juce::translate("Atari XE/XL") },
            { static_cast<int>(TemplateId::Apple2),             juce::translate("Apple 2") },
            { static_cast<int>(TemplateId::Oric),               juce::translate("Oric") },
            { static_cast<int>(TemplateId::Vectrex),            juce::translate("Vectrex") },
            { static_cast<int>(TemplateId::SharpMZ700),         juce::translate("Sharp MZ-700") },
    };

    return idToTemplateNameMap;
}

std::vector<Psg> NewSong::createPsgsFromTemplate(const TemplateId templateId) noexcept
{
    switch (templateId) {
        default:
            jassertfalse;               // Unhandled template!
        case TemplateId::AmstradCpc:
            return { Psg::buildForCpc() };
        case TemplateId::AmstradCpcPlaycity:
            return { Psg::buildForPlaycity(), Psg::buildForCpc(), Psg::buildForPlaycity() };
        case TemplateId::Pcw:
            return { Psg::buildForPcw() };
        case TemplateId::PcwTurboSound:
            return { Psg::buildForPcw(), Psg::buildForPcw() };
        case TemplateId::Spectrum:
            return { Psg::buildForSpectrum() };
        case TemplateId::SpectrumTurboSound:
            return { Psg::buildForSpectrum(), Psg::buildForSpectrum() };
        case TemplateId::SpecNext:
            return { Psg::buildForSpecNext(), Psg::buildForSpecNext(), Psg::buildForSpecNext() };
        case TemplateId::Pentagon:
            return { Psg::buildForPentagon() };
        case TemplateId::Msx:
            return { Psg::buildForMsx() };
        case TemplateId::MsxFpgaDarky:
            return { Psg::buildForMsx(), Psg::buildForMsx() };
        case TemplateId::SpectravideoSvi3x8:
            return { Psg::buildForSvi3x8() };
        case TemplateId::AtariSt:
            return { Psg::buildForAtariSt() };
        case TemplateId::AtariXeXl:
            return { Psg::buildForAtariXeXl() };
        case TemplateId::Apple2:
            return { Psg::buildForApple2() };
        case TemplateId::Oric:
            return { Psg::buildForOric() };
        case TemplateId::Vectrex:
            return { Psg::buildForVectrex() };
        case TemplateId::SharpMZ700:
            return { Psg::buildForSharpMz700() };
    }
}

SidPlayerCapability NewSong::createSidCapabilityFromTemplate(const TemplateId templateId) noexcept
{
    if (templateId == TemplateId::AtariSt) {
        return SidPlayerCapability::buildForAtariSt();
    }
    if (templateId == TemplateId::AmstradCpc) {
        return SidPlayerCapability::buildForCpc();
    }
    if (templateId == TemplateId::AmstradCpcPlaycity) {
        return SidPlayerCapability::buildForCpc();
    }
    // Caveat: Amstrad Plus is not in the list...

    return SidPlayerCapability::buildDefault();
}

}   // namespace arkostracker

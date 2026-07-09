#include "GenericInstrumentGenerator.h"

namespace arkostracker 
{

const juce::String GenericInstrumentGenerator::singleBeepInstrumentName = juce::translate("Beep");                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String GenericInstrumentGenerator::decreasingVolumeInstrumentName = juce::translate("Decreasing volume");               // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String GenericInstrumentGenerator::attackSustainDecayInstrumentName = juce::translate("Attack, sustain, decay");        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String GenericInstrumentGenerator::smallAttackAndLoopInstrumentName = juce::translate("Small attack and loop");         // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String GenericInstrumentGenerator::hardBass1InstrumentName = juce::translate("HardBass1");                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String GenericInstrumentGenerator::hardBass2InstrumentName = juce::translate("HardBass2");                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String GenericInstrumentGenerator::bassdrumInstrumentName = juce::translate("Bass drum");                                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String GenericInstrumentGenerator::snareInstrumentName = juce::translate("Snare");                                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String GenericInstrumentGenerator::tomInstrumentName = juce::translate("Tom");                                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String GenericInstrumentGenerator::hihatInstrumentName = juce::translate("Hihat");                                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String GenericInstrumentGenerator::cymbalCrashInstrumentName = juce::translate("Crash cymbal");                         // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

std::unique_ptr<Instrument> GenericInstrumentGenerator::generateInstrumentForSingleBeep() noexcept
{
    PsgPart psgPart;
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(15));

    return Instrument::buildPsgInstrument(singleBeepInstrumentName, psgPart);
}

std::unique_ptr<Instrument> GenericInstrumentGenerator::generateInstrumentForEmpty() noexcept
{
    return Instrument::buildEmptyPsgInstrument();
}

std::unique_ptr<Instrument> GenericInstrumentGenerator::generateInstrumentForDecreasingVolume(const juce::String& name) noexcept
{
    PsgPart psgPart;
    for (auto volume = 15; volume > 0; --volume) {      // No need to reach 0.
        psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(volume));
    }
    psgPart.setMainLoop(Loop(14, 14));

    return Instrument::buildPsgInstrument(name.isEmpty() ? decreasingVolumeInstrumentName : name, psgPart);
}

std::unique_ptr<Instrument> GenericInstrumentGenerator::generateInstrumentForSmallAttackAndLoop() noexcept
{
    PsgPart psgPart;
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(15));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(14));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(13));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(12));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(11));
    psgPart.setMainLoop(Loop(4, 4, true));

    return Instrument::buildPsgInstrument(smallAttackAndLoopInstrumentName, psgPart);
}

std::unique_ptr<Instrument> GenericInstrumentGenerator::generateInstrumentForHardBass1() noexcept
{
    PsgPart psgPart;
    psgPart.addCell(PsgInstrumentCell(PsgInstrumentCellLink::softToHard, 0, 0, 0, 0, 0, 0,
                                      4, 0, 0, 0, 0, 8, false,
                                      PsgValues::defaultSidActivated, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
                                      PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch));
    psgPart.setMainLoop(Loop(0, 0, true));

    return Instrument::buildPsgInstrument(hardBass1InstrumentName, psgPart);
}

std::unique_ptr<Instrument> GenericInstrumentGenerator::generateInstrumentForHardBass2() noexcept
{
    PsgPart psgPart;
    psgPart.addCell(PsgInstrumentCell(PsgInstrumentCellLink::softToHard, 0, 0, 0, 0, 0, 0,
                                      4, 0, 0, 0, 0, 0xa, false,
                                      PsgValues::defaultSidActivated, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
                                      PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
                                      ));
    psgPart.setMainLoop(Loop(0, 0, true));

    return Instrument::buildPsgInstrument(hardBass2InstrumentName, psgPart);
}

std::unique_ptr<Instrument> GenericInstrumentGenerator::generateInstrumentForAttackSustainDecay() noexcept
{
    PsgPart psgPart;
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(15));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(14));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(13));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(12));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(11));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(11));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(11));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(11));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(11));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(11));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(11));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(11));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(10));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(9));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(8));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(7));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(6));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(5));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(4));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(3));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(2));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(1));
    psgPart.setMainLoop(Loop(4, 20));

    return Instrument::buildPsgInstrument(attackSustainDecayInstrumentName, psgPart);
}

std::unique_ptr<Instrument> GenericInstrumentGenerator::generateInstrumentForHihat() noexcept
{
    PsgPart psgPart;
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(15, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(13, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(11, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(8, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(5, 1, 0, 0, 0, false));
    psgPart.setMainLoop(Loop(4, 4, false));

    return Instrument::buildPsgInstrument(hihatInstrumentName, psgPart);
}

std::unique_ptr<Instrument> GenericInstrumentGenerator::generateInstrumentForBassDrum() noexcept
{
    PsgPart psgPart;
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(15, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(14, 0, 0, 0, -150, true));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(13, 0, 0, 0, -300, true));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(12, 0, 0, 0, -400, true));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(11, 0, 0, 0, -500, true));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(10, 0, 0, 0, -600, true));
    psgPart.setMainLoop(Loop(4, 5, false));

    return Instrument::buildPsgInstrument(bassdrumInstrumentName, psgPart);
}

std::unique_ptr<Instrument> GenericInstrumentGenerator::generateInstrumentForSnare() noexcept
{
    PsgPart psgPart;
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(15, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(14, 2, 0, 0, 10, true));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(12, 3, 0, 0, 30, true));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(11, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(10, 0, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(10, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(9, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(8, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(7, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(6, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(5, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(4, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(3, 1, 0, 0, 0, false));
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(2, 1, 0, 0, 0, false));
    psgPart.setMainLoop(Loop(0, 13));

    return Instrument::buildPsgInstrument(snareInstrumentName, psgPart);
}

std::unique_ptr<Instrument> GenericInstrumentGenerator::generateInstrumentForCrash() noexcept
{
    static const std::vector volumes = { 15, 14, 13, 12, 11, 9, 8, 7, 6, 5, 4, 3, 2, 1 };

    PsgPart psgPart;
    for (const auto volume : volumes) {
        psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(volume, 1, 0, 0, 0, false));
    }
    psgPart.setMainLoop(Loop(0, static_cast<int>(volumes.size()) - 1));
    psgPart.setSpeed(2);

    return Instrument::buildPsgInstrument(cymbalCrashInstrumentName, psgPart);
}

std::unique_ptr<Instrument> GenericInstrumentGenerator::generateInstrumentForTom(const int pitchOffset) noexcept
{
    PsgPart psgPart;

    constexpr auto cellCount = 9;
    auto volume = 15;
    for (auto cellIndex = 0; cellIndex < cellCount; ++cellIndex) {
        psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(volume, 0, 0, 0, -(cellIndex * 50) + pitchOffset, true));

        --volume;
    }
    psgPart.setMainLoop(Loop(0, cellCount - 1));

    return Instrument::buildPsgInstrument(tomInstrumentName, psgPart);
}

const std::map<GenericInstrumentGenerator::TemplateId, juce::String>& GenericInstrumentGenerator::getTemplateIdToDisplayableName() noexcept
{
    static const std::map<TemplateId, juce::String> map = {
        { TemplateId::empty, Instrument::emptyInstrumentName },
        { TemplateId::singleBeep, singleBeepInstrumentName },
        { TemplateId::decreasingVolume, decreasingVolumeInstrumentName },
        { TemplateId::attackSustainDecay, attackSustainDecayInstrumentName },
        { TemplateId::smallAttackAndLoop, smallAttackAndLoopInstrumentName },
        { TemplateId::hardBass1, hardBass1InstrumentName },
        { TemplateId::hardBass2, hardBass2InstrumentName },
        { TemplateId::bassdrum, bassdrumInstrumentName },
        { TemplateId::snare, snareInstrumentName },
        { TemplateId::tom, tomInstrumentName },
        { TemplateId::hihat, hihatInstrumentName },
        { TemplateId::cymbalCrash, cymbalCrashInstrumentName },
    };

    return map;
}

std::unique_ptr<Instrument> GenericInstrumentGenerator::buildFromTemplateId(const TemplateId templateId) noexcept
{
    switch (templateId) {
        default:
            jassertfalse;       // Missing?
        case TemplateId::singleBeep:
            return generateInstrumentForSingleBeep();
        case TemplateId::empty:
            return generateInstrumentForEmpty();
        case TemplateId::decreasingVolume:
            return generateInstrumentForDecreasingVolume();
        case TemplateId::attackSustainDecay:
            return generateInstrumentForAttackSustainDecay();
        case TemplateId::smallAttackAndLoop:
            return generateInstrumentForSmallAttackAndLoop();
        case TemplateId::hardBass1:
            return generateInstrumentForHardBass1();
        case TemplateId::hardBass2:
            return generateInstrumentForHardBass2();
        case TemplateId::bassdrum:
            return generateInstrumentForBassDrum();
        case TemplateId::snare:
            return generateInstrumentForSnare();
        case TemplateId::tom:
            return generateInstrumentForTom(0);
        case TemplateId::hihat:
            return generateInstrumentForHihat();
        case TemplateId::cymbalCrash:
            return generateInstrumentForCrash();
    }
}

}   // namespace arkostracker

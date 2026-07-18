#include "Psg.h"

#include "PsgFrequency.h"

namespace arkostracker 
{

Psg::Psg(const PsgType pPsgType, const int pPsgFrequency, const float pReferenceFrequency, const int pSamplePlayerFrequency, const PsgMixingOutput& pPsgMixingOutput) noexcept :
        psgType(pPsgType),
        psgFrequency(pPsgFrequency),
        referenceFrequency(pReferenceFrequency),
        samplePlayerFrequency(pSamplePlayerFrequency),
        psgMixingOutput(pPsgMixingOutput)
{
}

Psg Psg::withPsgFrequency(const int newPsgFrequency) const noexcept
{
    return Psg(psgType, newPsgFrequency, referenceFrequency, samplePlayerFrequency, psgMixingOutput);
}

Psg Psg::withSamplePlayerFrequency(const int newSamplePlayerFrequency) const noexcept
{
    return Psg(psgType, psgFrequency, referenceFrequency, newSamplePlayerFrequency, psgMixingOutput);
}

PsgType Psg::getType() const noexcept
{
    return psgType;
}

int Psg::getPsgFrequency() const noexcept
{
    return psgFrequency;
}

float Psg::getReferenceFrequency() const noexcept
{
    return referenceFrequency;
}

int Psg::getSamplePlayerFrequency() const noexcept
{
    return samplePlayerFrequency;
}

PsgMixingOutput Psg::getPsgMixingOutput() const noexcept
{
    return psgMixingOutput;
}

bool Psg::operator==(const Psg& other) const noexcept // NOLINT(fuchsia-overloaded-operator)
{
    return ((psgType == other.psgType) && (psgFrequency == other.psgFrequency) && juce::exactlyEqual(referenceFrequency, other.referenceFrequency)
            && (samplePlayerFrequency == other.samplePlayerFrequency) && (psgMixingOutput == other.psgMixingOutput));
}

bool Psg::operator!=(const Psg& other) const noexcept // NOLINT(fuchsia-overloaded-operator)
{
    return !operator==(other);
}

bool Psg::operator<(const Psg& rhs) const noexcept
{
    if (psgType < rhs.psgType) {
        return true;
    }
    if (rhs.psgType < psgType) {
        return false;
    }
    if (psgFrequency < rhs.psgFrequency) {
        return true;
    }
    if (rhs.psgFrequency < psgFrequency) {
        return false;
    }
    if (referenceFrequency < rhs.referenceFrequency) {
        return true;
    }
    if (rhs.referenceFrequency < referenceFrequency) {
        return false;
    }
    if (samplePlayerFrequency < rhs.samplePlayerFrequency) {
        return true;
    }
    if (rhs.samplePlayerFrequency < samplePlayerFrequency) {
        return false;
    }
    return psgMixingOutput < rhs.psgMixingOutput;
}

Psg Psg::buildForCpc() noexcept
{
    return Psg();
}

Psg Psg::buildForPcw() noexcept
{
    return Psg();
}

Psg Psg::buildForPlaycity() noexcept
{
    return Psg(PsgType::ym, PsgFrequency::psgFrequencyAtariST);
}

Psg Psg::buildForAtariSt() noexcept
{
    return Psg(PsgType::ym, PsgFrequency::psgFrequencyAtariST);
}

Psg Psg::buildForAtariXeXl() noexcept
{
    return Psg(PsgType::ay, PsgFrequency::psgFrequencyAtariXEXL);
}

Psg Psg::buildForApple2() noexcept
{
    return Psg(PsgType::ay, PsgFrequency::psgFrequencyApple2);
}

Psg Psg::buildForOric() noexcept
{
    return Psg(PsgType::ay, PsgFrequency::psgFrequencyOric);
}

Psg Psg::buildForVectrex() noexcept
{
    return Psg(PsgType::ay, PsgFrequency::psgFrequencyVectrex);
}

Psg Psg::buildForSpectrum() noexcept
{
    return Psg(PsgType::ay, PsgFrequency::psgFrequencySpectrum);
}

Psg Psg::buildForSpecNext() noexcept
{
    return Psg(PsgType::ay, PsgFrequency::psgFrequencySpecNext);
}

Psg Psg::buildForPentagon() noexcept
{
    return Psg(PsgType::ay, PsgFrequency::psgFrequencyPentagon128K);
}

Psg Psg::buildForMsx() noexcept
{
    return Psg(PsgType::ay, PsgFrequency::psgFrequencyMsxAndSvi);
}

Psg Psg::buildForSvi3x8() noexcept
{
    return Psg(PsgType::ay, PsgFrequency::psgFrequencySvi3x8);
}

Psg Psg::buildForSharpMz700() noexcept
{
    return Psg(PsgType::ay, PsgFrequency::psgFrequencySharpMz700);
}


// =================================================================
// PsgMetadataPsgAndReferenceFrequencyComparator
// =================================================================

bool PsgMetadataPsgAndReferenceFrequencyComparator::operator()(const Psg& left, const Psg& right) const noexcept // NOLINT(fuchsia-overloaded-operator)
{
    // If same PSG frequency, compares with the reference frequency.
    if (left.getPsgFrequency() == right.getPsgFrequency()) {
        return (left.getReferenceFrequency() < right.getReferenceFrequency());
    }

    return (left.getPsgFrequency() < right.getPsgFrequency());
}

}   // namespace arkostracker

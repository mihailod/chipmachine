#include "ChangeSample.h"

#include "../../../../controllers/SongController.h"

namespace arkostracker
{

ChangeSample::ChangeSample(SongController& pSongController, Id pId, std::unique_ptr<Sample> pSample, const int pSampleFrequencyHz, juce::String pOriginalFilename) noexcept :
        songController(pSongController),
        id(std::move(pId)),
        sample(std::move(pSample)),
        sampleFrequencyHz(pSampleFrequencyHz),
        originalFilename(std::move(pOriginalFilename)),
        oldLoop(),
        oldAmplificationRatio(1),
        oldSample(),
        oldSampleFrequencyHz(),
        oldOriginalFilename()
{
}

bool ChangeSample::perform()
{
    songController.performOnInstrument(id, [&] (Instrument& instrument) noexcept {
        jassert(instrument.getType() == InstrumentType::sampleInstrument);

        auto& samplePart = instrument.getSamplePart();
        oldSample = samplePart.getSample();
        oldLoop = samplePart.getLoop();
        const auto amplificationRatio = samplePart.getAmplificationRatio();         // Stores it only to be consistent.
        oldAmplificationRatio = amplificationRatio;
        oldSampleFrequencyHz = samplePart.getFrequencyHz();
        oldOriginalFilename = samplePart.getOriginalFileName();

        samplePart.setSample(sample, Loop(0, sample->getLength() - 1), amplificationRatio);
        samplePart.setFrequencyHz(sampleFrequencyHz);
        samplePart.setOriginalFilename(originalFilename);
    });

    notify();

    return true;
}

bool ChangeSample::undo()
{
    songController.performOnInstrument(id, [&] (Instrument& instrument) noexcept {
        jassert(instrument.getType() == InstrumentType::sampleInstrument);

        auto& samplePart = instrument.getSamplePart();
        samplePart.setSample(oldSample, oldLoop, oldAmplificationRatio);
        samplePart.setFrequencyHz(oldSampleFrequencyHz);
        samplePart.setOriginalFilename(oldOriginalFilename);
    });

    notify();

    return true;
}

void ChangeSample::notify() const noexcept
{
    songController.getInstrumentObservers().applyOnObservers([&](InstrumentChangeObserver* observer) noexcept {
        observer->onInstrumentChanged(id, InstrumentChangeObserver::What::other);
    });
}

}   // namespace arkostracker

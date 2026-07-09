#pragma once

#include "../../../editorWithBars/controller/QuickEditController.h"

namespace arkostracker
{

class Id;
class SongController;

/** Controller to manage the quick edit (show Dialogs, and manage inputs). */
class PsgQuickEditController final : public QuickEditController
{
public:
    /**
     * Constructor.
     * @param songController the song controller.
     */
    explicit PsgQuickEditController(SongController& songController) noexcept;

protected:

    // QuickEditController method implementations.
    // ========================================================
    int getItemLength(const Id &itemId) const noexcept override;
    juce::String validateNoise(const Id &instrumentId, int barIndex, const juce::String &values) const noexcept override;
    juce::String validateEnvelope(const Id &instrumentId, int barIndex, const juce::String &values) const noexcept override;
    juce::String validateSoundType(const Id& itemId, int barIndex, const juce::String& values) const noexcept override;
    juce::String validatePitch(bool isPrimaryPitch, const Id &instrumentId, int barIndex, const juce::String &values) const noexcept override;
    juce::String validateArpeggioNoteInOctave(bool isPrimaryArpeggioInNote, const Id &instrumentId, int barIndex, const juce::String &values) const noexcept override;
    juce::String validateArpeggioOctave(bool isPrimaryArpeggioOctave, const Id &instrumentId, int barIndex, const juce::String &values) const noexcept override;
    juce::String validatePeriod(bool isPrimaryPeriod, const Id &instrumentId, int barIndex, const juce::String &values) const noexcept override;
    juce::String validateSidLowShelfVolume(const Id& instrumentId, int barIndex, const juce::String& values) const noexcept override;
    juce::String validateSidBalancePercent(const Id& instrumentId, int barIndex, const juce::String& values) const noexcept override;
    juce::String validateSidIsActivatedAndRatio(const Id& itemId, int barIndex, const juce::String& values) const noexcept override;
    juce::String validateSidArpeggio(const Id& instrumentId, int barIndex, const juce::String& values) const noexcept override;
    juce::String validateSidPitch(const Id& instrumentId, int barIndex, const juce::String& values) const noexcept override;

private:
    SongController& songController;
};

}   // namespace arkostracker

#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include "../utils/Observers.h"
#include "MidiController.h"

namespace arkostracker 
{

/** Implementation of the Midi Controller. */
class MidiControllerImpl final : public MidiController,
                                 public juce::MidiInputCallback,
                                 juce::MessageListener             // Listens to the MIDI messages on the main thread.
{
public:
    /**
     * Constructor.
     * @param audioDeviceManager the Audio Device Manager to ask the MIDI input from.
     */
    explicit MidiControllerImpl(juce::AudioDeviceManager& audioDeviceManager) noexcept;

    /** Destructor. */
    ~MidiControllerImpl() override;

    // MidiController method implementations.
    // =========================================
    void scanMidiDevicesAndRegister() noexcept override;
    Observers<MidiController::MidiListener>& getMidiObservers() noexcept override;

    // juce::MidiInputCallback method implementations.
    // ==================================================
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

private:
    /** Stops all the stored Midi Inputs, and clears the stored list of Midi inputs. */
    void stopAndClearAllMidiInputs() noexcept;

    // MessageListener method implementations.
    // =====================================================
    void handleMessage(const juce::Message& message) override;

    juce::AudioDeviceManager& audioDeviceManager;
    Observers<MidiController::MidiListener> midiListeners;                      // Observes the midi events.
    std::vector<std::unique_ptr<juce::MidiInput>> midiInputs;                   // The Midi inputs to read from.
};

}   // namespace arkostracker

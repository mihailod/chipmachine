#include "MidiControllerImpl.h"

#include "messages/MessageMidiNote.h"
#include "messages/MessageMidiProgramChange.h"

namespace arkostracker 
{

MidiControllerImpl::MidiControllerImpl(juce::AudioDeviceManager& pAudioDeviceManager) noexcept :
        audioDeviceManager(pAudioDeviceManager),
        midiListeners(),
        midiInputs()
{
}

MidiControllerImpl::~MidiControllerImpl()
{
    stopAndClearAllMidiInputs();
}


// MidiController method implementations.
// =========================================

void MidiControllerImpl::scanMidiDevicesAndRegister() noexcept
{
    // First, disconnects all the MIDI Inputs.
    stopAndClearAllMidiInputs();

    // Searches among all MIDI devices about the ones to read from.
    for (const auto& device : juce::MidiInput::getAvailableDevices()) {
        if (audioDeviceManager.isMidiInputDeviceEnabled(device.identifier)) {
            // This device is enabled. Registers to it and stores it.
            auto midiInput(juce::MidiInput::openDevice(device.identifier, this));
            if (midiInput != nullptr) {
                midiInput->start();
                midiInputs.push_back(std::move(midiInput));

                DBG("Registered to MIDI In device = " + device.name);
            }
        }
    }
}


// juce::MidiInputCallback method implementations.
// ==================================================

void MidiControllerImpl::handleIncomingMidiMessage(juce::MidiInput* /*source*/, const juce::MidiMessage& message)
{
    // THIS IS A HIGH-PRIORITY THREAD.

    if (message.isNoteOn()) {
        const auto noteNumber = message.getNoteNumber();
        auto* noteMessage = new MessageMidiNote(noteNumber);    // Don't worry, reference counted object.

        // Transmits it as a JUCE message.
        postMessage(noteMessage);
    } else if (message.isProgramChange()) {
        const auto programChangeNumber = message.getProgramChangeNumber();
        auto* programChangeMessage = new MessageMidiProgramChange(programChangeNumber);    // Don't worry, reference counted object.

        // Transmits it as a JUCE message.
        postMessage(programChangeMessage);
    }
}


// MessageListener method implementations.
// ==========================================

void MidiControllerImpl::handleMessage(const juce::Message& incomingMessage)
{
    // This is the main thread.

    if (midiListeners.empty()) {        // If no observer, nothing to do.
        DBG("MIDI Message on main thread, but no observers.");
        return;
    }

    // Is it a Midi note?
    if (const auto* message = dynamic_cast<const MessageMidiNote*>(&incomingMessage); message != nullptr) {
        const auto noteNumber = message->getData();
        DBG("MIDI Message on main thread. Note number = " + juce::String(noteNumber));

        // Sends it to the listeners.
        midiListeners.applyOnObservers([&] (MidiListener* listener) {
            listener->onMidiNoteReceived(noteNumber);
        });
        return;
    }

    // Is it a Program Change?
    if (const auto* message = dynamic_cast<const MessageMidiProgramChange*>(&incomingMessage); message != nullptr) {
        const auto programNumber = message->getData();
        DBG("MIDI Message on main thread. Program change = " + juce::String(programNumber));

        // Sends it to the listeners.
        midiListeners.applyOnObservers([&] (MidiListener* listener) {
            listener->onMidiProgramChangeReceived(programNumber);
        });
    }
}


// ==================================================

Observers<MidiController::MidiListener>& MidiControllerImpl::getMidiObservers() noexcept
{
    return midiListeners;
}

void MidiControllerImpl::stopAndClearAllMidiInputs() noexcept
{
    // Stops all the inputs.
    for (auto& midiInput : midiInputs) {
        if (midiInput != nullptr) {
            midiInput->stop();
        }
    }

    midiInputs.clear();
}

}   // namespace arkostracker

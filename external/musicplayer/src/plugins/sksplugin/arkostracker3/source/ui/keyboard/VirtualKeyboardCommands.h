#pragma once

#include <unordered_map>
#include <vector>

#include "../utils/TargetCommands.h"
#include "VirtualKeyboardLayout.h"

namespace arkostracker 
{

/**
 * This regroups the commands of the virtual keyboard (the PC keyboard to play notes), because several components may
 * want to use it (the PatternViewer, the "main", or sound effect slots). The great thing is that by using the same commands,
 * JUCE allows several components to declare it, yet each can define their own actions. And only one instance of the keys is seen in
 * the keyboard mapping editor.
 *
 * The methods are analog to ApplicationCommandTarget, which the other components can derive from and derive to this object.
 */
class VirtualKeyboardCommands final : public TargetCommands
{
public:
    /**
     * Constructor.
     * @param virtualKeyboardLayouts the layout to use. If SHOULD be the ones from the Preferences.
     */
    explicit VirtualKeyboardCommands(VirtualKeyboardLayout virtualKeyboardLayouts) noexcept;

    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandId, juce::ApplicationCommandInfo& result) override;
    /**
     * Checks whether a note has been found from the command.
     * @return -1 if no note is found, else the base note (>=0, probably <24). The current octave should be added before use.
     */
    int performForNote(const juce::ApplicationCommandTarget::InvocationInfo& info);

private:
    /** Simple holder of the data of the commands, in order to populate the commands quickly without copy/pasteAfter. */
    class CommandData
    {
    public:
        CommandData(juce::String pDisplayedNote, int pNoteOffset) :
                displayedNote(std::move(pDisplayedNote)),
                noteOffset(pNoteOffset)
        {
        }

        const juce::String& getDisplayedNote() const noexcept
        {
            return displayedNote;
        }

        int getNoteOffset() const noexcept
        {
            return noteOffset;
        }

    private:
        juce::String displayedNote;
        int noteOffset;
    };

    static const std::unordered_map<int, CommandData> commandIdToCommandData;
    static const std::vector<int> azertyBiKeycodes;     // Encoded by pair, or 0 if the second is not used.
    static const std::vector<int> qwertyBiKeycodes;     // Encoded by pair, or 0 if the second is not used.
    static const std::vector<int> qwertzBiKeycodes;     // Encoded by pair, or 0 if the second is not used.

    VirtualKeyboardLayout virtualKeyboardLayout;                  // The current layout (azerty, qwerty, etc.).
};

}   // namespace arkostracker

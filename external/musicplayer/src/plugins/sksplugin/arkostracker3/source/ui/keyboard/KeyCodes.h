#pragma once

#ifndef JUCE_LINUX
#include <juce_gui_basics/juce_gui_basics.h>
#endif

namespace arkostracker 
{

/** Some specific keycodes, as they are different from Linux to other OSes. */
class KeyCodes
{
public:
    /** Prevents instantiation. */
    KeyCodes() = delete;

#if JUCE_LINUX
    constexpr static int keypadMinus() { return 0xad; }      // Required on Linux. Soft Hyphen.
    constexpr static int keypad0() { return '0'; }
    constexpr static int keypad1() { return '1'; }          // Doesn't work with JUCE????
    constexpr static int keypad2() { return '2'; }
#else
    constexpr static int keypadMinus() { return '-'; }
    constexpr static int keypad0() { return juce::KeyPress::numberPad0; }
    constexpr static int keypad1() { return juce::KeyPress::numberPad1; }
    constexpr static int keypad2() { return juce::KeyPress::numberPad2; }
#endif
};

}   // namespace arkostracker

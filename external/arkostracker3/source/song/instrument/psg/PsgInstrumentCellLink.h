#pragma once

#include <juce_core/juce_core.h>

#include "../../../utils/OptionalValue.h"

namespace arkostracker 
{

/** Link between the software and hardware part of a cell. */
enum class PsgInstrumentCellLink : juce::uint8
{
    first = 0,

    /** Only software. The most common sound. */
    softOnly = first,
    /** Only hardware. Not so common. */
    hardOnly,
    /** Pass the software period to the hardware. Very common for bass. */
    softToHard,
    /** Pass the hardware period to the software. Equals to the "sync" mode of STarKos. */
    hardToSoft,
    /** Software parallel to hardware. Not very common. */
    softAndHard,
    /** No software or hardware. Useful for drums. */
    noSoftNoHard,

    last = noSoftNoHard
};


/** Helper class to convert a Link to String. */
class PsgInstrumentCellLinkHelper
{
public:
    /**
     * Converts the given Link to a human-readable (and serializable) String.
     * @param link the link.
     * @return the String.
     */
    static juce::String toString(const PsgInstrumentCellLink link) noexcept
    {
        switch (link) {
            case PsgInstrumentCellLink::softOnly:
                return "softwareOnly";
            case PsgInstrumentCellLink::hardOnly:
                return "hardwareOnly";
            case PsgInstrumentCellLink::hardToSoft:
                return "hardwareToSoftware";
            case PsgInstrumentCellLink::softToHard:
                return "softwareToHardware";
            case PsgInstrumentCellLink::softAndHard:
                return "softwareAndHardware";
            case PsgInstrumentCellLink::noSoftNoHard:
                return "noSoftwareNoHardware";
            default:
                jassertfalse;       // Shouldn't happen.
                return { };
        }
    }

    /**
     * Converts the given String (such as serialized) into a Link.
     * @param linkString the link, as a String.
     * @return the Link, or empty if unknown.
     */
    static OptionalValue<PsgInstrumentCellLink> fromString(const juce::String& linkString) noexcept
    {
        if (linkString == "softwareOnly") {
            return PsgInstrumentCellLink::softOnly;
        }
        if (linkString == "hardwareOnly") {
            return PsgInstrumentCellLink::hardOnly;
        }
        if (linkString == "hardwareToSoftware") {
            return PsgInstrumentCellLink::hardToSoft;
        }
        if (linkString == "softwareToHardware") {
            return PsgInstrumentCellLink::softToHard;
        }
        if (linkString == "softwareAndHardware") {
            return PsgInstrumentCellLink::softAndHard;
        }
        if (linkString == "noSoftwareNoHardware") {
            return PsgInstrumentCellLink::noSoftNoHard;
        }

        return { };
    }

    /**
     * @return true if the Link is about hardware sound.
     * @param link the Link.
     */
    static bool isHardware(const PsgInstrumentCellLink link) noexcept
    {
        switch (link) {
            case PsgInstrumentCellLink::hardOnly:
            case PsgInstrumentCellLink::hardToSoft:
            case PsgInstrumentCellLink::softToHard:
            case PsgInstrumentCellLink::softAndHard:
                return true;
            case PsgInstrumentCellLink::softOnly:
            case PsgInstrumentCellLink::noSoftNoHard:
            default:
                return false;
        }
    }

    /**
     * @return true if the Link has its primary section software. No soft no hard is considered software.
     * @param link the Link.
     */
    static bool isPrimarySoftware(const PsgInstrumentCellLink link) noexcept
    {
        switch (link) {
            case PsgInstrumentCellLink::softToHard:
            case PsgInstrumentCellLink::softAndHard:
            case PsgInstrumentCellLink::softOnly:
            case PsgInstrumentCellLink::noSoftNoHard:
            default:
                return true;
            case PsgInstrumentCellLink::hardOnly:
            case PsgInstrumentCellLink::hardToSoft:
                return false;
        }
    }
};

}   // namespace arkostracker

#pragma once

namespace arkostracker
{

enum class EventsExportType
{
    exportAllEvents = 1,        // Juce doesn't like ID 0.
    exportOnlyEvents,
    exportOnlySamples,
};

}   // namespace arkostracker

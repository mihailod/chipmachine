#pragma once

#include <memory>

#include "../storage/DocumentArrangement.h"

namespace arkostracker 
{

/** Class to generate default Arrangements. */
class ArrangementFactory
{
public:
    /** Prevents instantiation. */
    ArrangementFactory() = delete;

    /** @return the main Stored Arrangement (typically, the default one). */
    static std::unique_ptr<DocumentArrangement> createStoredArrangementMain() noexcept;

    /** @return the Stored Arrangement mainly with the PV. */
    static std::unique_ptr<DocumentArrangement> createStoredArrangementFocusOnPatternViewer() noexcept;

    /** @return the Stored Arrangement with the expressions. */
    static std::unique_ptr<DocumentArrangement> createStoredArrangementFocusOnExpressions() noexcept;

    /** @return the Stored Arrangement focusing on listening. */
    static std::unique_ptr<DocumentArrangement> createStoredArrangementFocusOnListening() noexcept;

private:
    static const int screenWidth;
    static const int mixerWidth;
    static const int mixerBesidesPvWidth;
};

}   // namespace arkostracker

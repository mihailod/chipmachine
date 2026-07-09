#include "ToggleRetrigAction.h"

namespace arkostracker
{

PsgInstrumentCell ToggleRetrigAction::operator()(int /*iterationIndex*/, const PsgInstrumentCell& cell) noexcept
{
    return cell.withRetrig(!cell.isRetrig());
}

}   // namespace arkostracker

#include "AkgPatternItem.h"

namespace arkostracker 
{

AkgPatternItem::AkgPatternItem(int pTrackIndex, int pTransposition) noexcept :
    trackIndex(pTrackIndex),
    transposition(pTransposition)
{
}

bool AkgPatternItem::operator==(const AkgPatternItem& item) const noexcept
{
    return (trackIndex == item.trackIndex) && (transposition == item.transposition);
}

}   // namespace arkostracker


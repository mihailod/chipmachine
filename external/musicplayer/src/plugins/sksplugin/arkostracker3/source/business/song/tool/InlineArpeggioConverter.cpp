#include "InlineArpeggioConverter.h"

#include "../../../song/Song.h"
#include "browser/CellBrowser.h"

namespace arkostracker 
{

InlineArpeggioConverter::InlineArpeggioConverter(Song& pSong, int pMaximumIndexArpeggio) noexcept :
        song(pSong),
        maximumIndexArpeggio(pMaximumIndexArpeggio),
        inlineArpeggio3NotesToArpeggioIndex(),
        inlineArpeggio4NotesToArpeggioIndex()
{
}

bool InlineArpeggioConverter::convert() noexcept
{
    inlineArpeggio3NotesToArpeggioIndex.clear();
    inlineArpeggio4NotesToArpeggioIndex.clear();

    const auto subsongIds = song.getSubsongIds();

    CellBrowser cellBrowser(song);

    // Parses each Subsong.
    for (const auto& subsongId : subsongIds) {
        auto success = true;
        cellBrowser.browseSubsongCells(true, subsongId, [&](const Cell& cell) noexcept {
            bool localSuccess;
            auto newCell = readCellAndManageArpeggios(cell, localSuccess);
            success &= localSuccess;

            return newCell;
        });

        if (!success) {
            return false;
        }
    }

    return true;
}

std::unique_ptr<Cell> InlineArpeggioConverter::readCellAndManageArpeggios(const Cell& cell, bool& success) noexcept
{
    success = true;

    // Is there a "Arpeggio 3 notes" or "4 notes" effect? Only the first one is read (other ones would be dismissed anyway).
    auto isArpeggio3Notes = true;
    auto [effectIndex, cellEffectOpt] = cell.findWithIndex(Effect::arpeggio3Notes);

    if (cellEffectOpt.isAbsent()) {
        // Not an Arpeggio 3. 4 notes maybe?
        isArpeggio3Notes = false;

        auto [effectIndexArp4, cellEffectOptArp4] = cell.findWithIndex(Effect::arpeggio4Notes);
        if (cellEffectOptArp4.isPresent()) {
            effectIndex = effectIndexArp4;
            cellEffectOpt = cellEffectOptArp4;
        }
    }

    // Nothing found, exits.
    if (effectIndex < 0) {
        return nullptr;
    }

    // An Arpeggio effect!
    const auto cellEffect = cellEffectOpt.getValueRef();
    const auto arpeggioValue = (cellEffect.getEffectRawValue() & (isArpeggio3Notes ? 0xfff0 : 0xffff));      // Keeps only what is necessary, as a security.

    int arpeggioIndex;
    // Only 0s? Then easy: forces the Arpeggio Table 0.
    if (arpeggioValue == 0) {
        arpeggioIndex = 0;
    } else {
        // The value is a "real" arpeggio.
        // Already stored? What map to use?
        auto& inlineArpeggiosToArpeggioIndex = isArpeggio3Notes ? inlineArpeggio3NotesToArpeggioIndex : inlineArpeggio4NotesToArpeggioIndex;

        auto iterator = inlineArpeggiosToArpeggioIndex.find(arpeggioValue);
        if (iterator == inlineArpeggiosToArpeggioIndex.cend()) {
            // Not stored yet. Generates the Arpeggio and inserts it.
            auto& expressionHandler = song.getExpressionHandler(true);

            Expression newArpeggio(true, "Generated arpeggio", false);
            fillArpeggio(newArpeggio, arpeggioValue, isArpeggio3Notes);
            newArpeggio.setLoop(Loop(0, isArpeggio3Notes ? 2 : 3, true));   // It loops on its whole length.

            arpeggioIndex = expressionHandler.addExpression(newArpeggio);

            // Last reached? Stops!
            if (arpeggioIndex > maximumIndexArpeggio) {
                success = false;
                return nullptr;
            }

            // Links the value to the index.
            inlineArpeggiosToArpeggioIndex.insert(std::make_pair(arpeggioValue, arpeggioIndex));
        } else {
            // It already exists. What is its index?
            arpeggioIndex = iterator->second;
        }
    }

    // Replaces the Arpeggio effect with an ArpeggioTable.
    jassert(arpeggioIndex >= 0);
    auto newCell = std::make_unique<Cell>(cell);
    newCell->setEffect(effectIndex, CellEffect::buildFromLogicalValue(Effect::arpeggioTable, arpeggioIndex));

    return newCell;
}

void InlineArpeggioConverter::fillArpeggio(Expression& arpeggio, int arpeggioValue, bool isArpeggio3Notes) noexcept
{
    jassert(arpeggio.isArpeggio());
    jassert(arpeggio.getLength() == 0);

    auto nibbleCount = isArpeggio3Notes ? 3 : 4;

    // Encodes the most significant nibble, from left to right (047x -> 0, 4, 7 or 047C -> 0, 4, 7, C for example).
    auto encodedNibbleIndex = 0;
    for (auto nibbleIndex = 3; nibbleCount > 0; --nibbleIndex, --nibbleCount, ++encodedNibbleIndex) {
        const auto value = ((arpeggioValue >> (4 * nibbleIndex)) & 0xf);
        arpeggio.addValue(value);
    }
}

}   // namespace arkostracker


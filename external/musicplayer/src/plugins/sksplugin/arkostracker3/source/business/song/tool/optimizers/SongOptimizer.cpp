#include "SongOptimizer.h"

#include "../../../../export/ExportConfiguration.h"
#include "../../../../song/Song.h"
#include "../CellOperations.h"
#include "../InlineArpeggioConverter.h"
#include "../RetrigInInstrumentHeaderNormalizer.h"
#include "../browser/CellBrowser.h"
#include "ExpressionsOptimizer.h"
#include "InstrumentsOptimizer.h"
#include "PatternOptimizer.h"

namespace arkostracker 
{

std::unique_ptr<Song> SongOptimizer::optimize(const Song& song, const std::vector<Id>& subsongIdsToKeep, const bool keepSamples) noexcept
{
    return optimize(song, true, true, true, keepSamples, true, true, true, true, true, true, subsongIdsToKeep);
}

std::unique_ptr<Song> SongOptimizer::optimize(const Song& song, ExportConfiguration& exportConfiguration) noexcept
{
    auto newSong = optimize(song, true, true, true, exportConfiguration.areSamplesExported(), true, true, true, true, true, true, exportConfiguration.getSubsongIds());

    // Substitutes the new IDs in the Export Configuration.
    if (newSong != nullptr) {
        const auto newSubsongIds = newSong->getSubsongIds();
        exportConfiguration.setSubsongIds(newSubsongIds);
    }

    return newSong;
}

std::unique_ptr<Song> SongOptimizer::optimize(const Song& originalSong, const bool optimizePatterns, const bool normalizeCellsAndOptimizeTracks,
                                              const bool optimizeInstruments, const bool keepSamples, const bool flattenInstruments, const bool transferInstrumentRetrig,
                                              const bool optimizeExpressions, const bool flattenExpressions,
                                              const bool convertInlineArpeggios, const bool convertLegatoToEffect,
                                              const std::vector<Id>& originalSubsongIdsToKeep) noexcept
{
    jassert(!originalSubsongIdsToKeep.empty());

    std::unique_ptr<Song> newSong;

    // Optimizes the Patterns a first time.
    if (optimizePatterns) {
        PatternOptimizer optimizer(originalSong, originalSubsongIdsToKeep);
        newSong = optimizer.optimize(true, normalizeCellsAndOptimizeTracks);
    } else {
        newSong = std::make_unique<Song>(originalSong);
    }

    // Removes the Instrument Retrig of all the Instruments.
    if (optimizeInstruments && transferInstrumentRetrig) {
        RetrigInInstrumentHeaderNormalizer retrigInInstrumentHeaderNormalizer(*newSong);
        retrigInInstrumentHeaderNormalizer.apply();
    }

    if (optimizeInstruments) {
        InstrumentsOptimizer optimizer(*newSong, keepSamples, flattenInstruments, transferInstrumentRetrig);
        newSong = optimizer.optimize();
    }

    if (optimizeExpressions) {
        ExpressionsOptimizer optimizer(*newSong, flattenExpressions);
        newSong = optimizer.optimize();
    }

    if (convertInlineArpeggios) {
        InlineArpeggioConverter inlineArpeggioConverter(*newSong);
        auto success = inlineArpeggioConverter.convert();
        if (!success) {
            jassertfalse;              // May happen if there are too many direct Arpeggios.
            return nullptr;
        }
    }

    // Browses each pattern to convert the Legato into an effect. Many exporters require this.
    if (convertLegatoToEffect) {
        const CellBrowser cellBrowser(*newSong);
        cellBrowser.browseCells(false, LegatoToEffect());
    }

    // Optimizes the Patterns a last time.
    // The Song is not the same anymore, so we cannot use the original SubsongIds. Use all the ones from the new Song,
    // which contain only the right ones.
    if (optimizePatterns) {
        const auto newSubsongIds = newSong->getSubsongIds();

        PatternOptimizer patternOptimizer(*newSong, newSubsongIds);
        newSong = patternOptimizer.optimize(true, normalizeCellsAndOptimizeTracks);
    }

    return newSong;
}

}   // namespace arkostracker

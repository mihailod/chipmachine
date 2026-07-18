#include "SongLoader.h"

#include "../../utils/FileUtil.h"
#include "../SongImporter.h"
#include "../at1/At1SongImporter.h"
#include "../at2/At2SongImporter.h"
#include "../at3/At3SongImporter.h"
#include "../chp/ChpSongImporter.h"
#include "../midi/MidiImporter.h"
#include "../mod/ModSongImporter.h"
#include "../soundtrakker/Stk128Importer.h"
#include "../starkos/StarkosImporter.h"
#include "../vt2/Vt2SongImporter.h"
#include "../wyz/WyzSongImporter.h"

namespace arkostracker 
{

std::unique_ptr<SongLoader::Result> SongLoader::loadSong(const juce::File& file, const bool failIfRequireConfiguration,       // NOLINT(readability-convert-member-functions-to-static)
                                                         std::unique_ptr<ImportConfiguration> configuration) noexcept
{
    // Does the file exist?
    if (!file.existsAsFile()) {
        return std::make_unique<Result>(ImportStatus::fileNotFound, nullptr, nullptr,
                                                    ConfigurationType::none, ImportFirstPassReturnData(), ImportedFormat::unknown);
    }

    // Unzip, if needed.
    const auto inputStream = FileUtil::openFileOrZip(file);
    if (inputStream == nullptr) {
        return std::make_unique<Result>(ImportStatus::loadingFailed, nullptr, nullptr,
                                                    ConfigurationType::none, ImportFirstPassReturnData(), ImportedFormat::unknown);
    }

    // Gets the possible extension, with the ".".
    auto extension = file.getFileExtension();
    if (extension.startsWith(".")) {
        extension = extension.substring(1);
    }

    return loadSong(*inputStream, extension, failIfRequireConfiguration, std::move(configuration));
}

std::unique_ptr<SongLoader::Result> SongLoader::loadSong(juce::InputStream& inputStream, const juce::String& extension, const bool failIfRequireConfiguration,  // NOLINT(readability-convert-member-functions-to-static)
                                                         const std::unique_ptr<ImportConfiguration>& configuration) noexcept
{
    // Tests each importer. In this order!
    std::vector<std::unique_ptr<SongImporter>> importers;
    importers.push_back(std::make_unique<At3SongImporter>());
    importers.push_back(std::make_unique<At2SongImporter>());
    importers.push_back(std::make_unique<StarkosImporter>());
    importers.push_back(std::make_unique<MidiImporter>());
    importers.push_back(std::make_unique<Vt2SongImporter>());
    importers.push_back(std::make_unique<ChpSongImporter>());
    importers.push_back(std::make_unique<At1SongImporter>());
    importers.push_back(std::make_unique<WyzSongImporter>());
    importers.push_back(std::make_unique<ModSongImporter>());       // Among the last, because the detection may be based on the extension.
    importers.push_back(std::make_unique<Stk128Importer>());        // The last, as the detection is based on the extension.

    // Use either the given configuration (if present), else use a default one.
    const auto configurationToUse = (configuration == nullptr) ? ImportConfiguration() : *configuration;

    for (const auto& importer : importers) {
        auto rewindSuccess = inputStream.setPosition(0);         // Important! Rewind! The position has been possibly moved by the previous iteration.
        jassert(rewindSuccess);

        // Is this a right importer?
        if (!importer->doesFormatMatch(inputStream, extension)) {
            continue;
        }

        // Loads the song. NOTE: we do it even if the configuration is required. This is tested just after (a paradox...) but this allows
        // to load the song and get some data about it (useful for the import panels).
        rewindSuccess = inputStream.setPosition(0);         // Important! Rewind! The position has been moved when checking the format.
        jassert(rewindSuccess);
        const auto importerResult = importer->loadSong(inputStream, configurationToUse);

        // If a Configuration is required but there is none, and we want to stop in this case, stops.
        const auto configurationType = importer->requireConfiguration();
        if ((configurationType != ConfigurationType::none) && failIfRequireConfiguration && (configuration == nullptr)) {
            return std::make_unique<Result>(ImportStatus::configurationRequired, nullptr, nullptr, configurationType,
                                                        importer->getReturnData(), ImportedFormat::unknown);
        }

        // Did the song successfully generated?
        const auto status = (importerResult->song == nullptr) ? ImportStatus::decodingFailure : ImportStatus::ok;
        return std::make_unique<Result>(status, std::move(importerResult->song), std::move(importerResult->errorReport), configurationType,
                                                    ImportFirstPassReturnData(),   // No need for the return data.
                                                    importer->getFormat());
    }

    // Nothing found!
    return std::make_unique<Result>(ImportStatus::noMatchingFormat, nullptr, nullptr,
                                                ConfigurationType::none, ImportFirstPassReturnData(), ImportedFormat::unknown);
}

}   // namespace arkostracker

#pragma once

#include <memory>
#include <utility>

#include "../../song/Song.h"
#include "../../utils/ErrorReport.h"
#include "../ConfigurationType.h"
#include "../ImportedFormat.h"
#include "configuration/ImportConfiguration.h"
#include "configuration/ImportFirstPassReturnData.h"

namespace arkostracker
{

/**
 * Loads a Song in any format (128, SKS, ATx...). Some importer may need configuration.
 * The input file may be zipped.
 *
 * This is synchronous. If the caller wants to provide a configuration, it can.
 * If not, and if an importer requires one, the caller must ask the SongLoader about it, else the importer will use a default.
 *
 * This way, both synchronous and asynchronous callers will work the same way.
 */
class SongLoader
{
public:

    /** How the import went. */
    enum class ImportStatus
    {
        ok,
        fileNotFound,
        /** A zip file that couldn't be parsed, for example. */
        loadingFailed,
        /** The selected importer returned no song. It must be malformed. */
        decodingFailure,
        noMatchingFormat,
        configurationRequired
    };

    /** The result about the import. */
    class Result
    {
    public:
        Result(ImportStatus pStatus, std::unique_ptr<Song> pSong, std::unique_ptr<ErrorReport> pErrorReport, ConfigurationType pConfigurationType,
               ImportFirstPassReturnData pImportReturnData, ImportedFormat pFormat) :
                status(pStatus),
                configurationType(pConfigurationType),
                song(std::move(pSong)),
                errorReport(std::move(pErrorReport)),
                importReturnData(std::move(pImportReturnData)),
                format(pFormat)
        {
        }

        const ImportStatus status;
        const ConfigurationType configurationType;                      // Useful to know what configuration panel to show.
        std::unique_ptr<Song> song;                                     // Nullptr if an error occurred.
        std::unique_ptr<ErrorReport> errorReport;                       // May be nullptr if the song generation couldn't be started.
        const ImportFirstPassReturnData importReturnData;               // Data returned by the importer, to inject into the import panel.
        const ImportedFormat format;                                    // The format of the loaded song.
    };

    /**
     * Loads the song, in any format, zipped or not.
     * @param file the file.
     * @param failIfRequireConfiguration if true, this will return with a "configuration required" if no configuration is given and the selected importer requires one.
     * @param configuration the configuration. If not present, the importer may use a default one.
     * @return the result. Never null.
     */
    std::unique_ptr<Result> loadSong(const juce::File& file, bool failIfRequireConfiguration = false,
                                     std::unique_ptr<ImportConfiguration> configuration = std::make_unique<ImportConfiguration>()) noexcept;

    /**
     * Loads the song, in any format. It must NOT be zipped.
     * @param unzippedInputStream the stream. It must NOT be zipped.
     * @param extension the extension, if known (without ".").
     * @param failIfRequireConfiguration if true, this will return with a "configuration required" if no configuration is given and the selected importer requires one.
     * @param configuration the configuration. If not present, the importer may use a default one.
     * @return the result. Never null.
     */
    std::unique_ptr<Result> loadSong(juce::InputStream& unzippedInputStream, const juce::String& extension, bool failIfRequireConfiguration = false,
                                     const std::unique_ptr<ImportConfiguration>& configuration = std::make_unique<ImportConfiguration>()) noexcept;
};

}   // namespace arkostracker

#pragma once

#include <memory>

#include "../song/Song.h"
#include "../utils/ErrorReport.h"
#include "ConfigurationType.h"
#include "loader/configuration/ImportConfiguration.h"
#include "loader/configuration/ImportFirstPassReturnData.h"
#include "ImportedFormat.h"

namespace arkostracker 
{

/** Abstract class to import a Song, checking the format of the input file first. */
class SongImporter
{
public:
    /** The result of the import. */
    class Result
    {
    public:
        /**
         * Constructor.
         * @param pSong the song, or nullptr if an error occurred.
         * @param pErrorReport the error report. Always present.
         */
        Result(std::unique_ptr<Song> pSong, std::unique_ptr<ErrorReport> pErrorReport) :
                song(std::move(pSong)),
                errorReport(std::move(pErrorReport))
        {
        }

        std::unique_ptr<Song> song;
        std::unique_ptr<ErrorReport> errorReport;
    };

    /** Destructor. */
    virtual ~SongImporter() = default;

    /** @return the configuration this importer requires. By default, requires nothing. */
    virtual ConfigurationType requireConfiguration() const noexcept
    {
        return ConfigurationType::none;
    }

    /**
     * @return the data that might be used when the import panel is opened. A first load has been done, which
     * the implementation must use to prepare the object this method must return.
     * The default implementation returns default values.
     */
    virtual ImportFirstPassReturnData getReturnData() const noexcept
    {
        return { };
    }

    /**
     * @return true if the file is worth being parsed. If true, no other format will be checked besides this one.
     * @param inputStream the file data, rewind.
     * @param extension the extension, without the dot ("128", "aks", etc.).
     */
    virtual bool doesFormatMatch(juce::InputStream& inputStream, const juce::String& extension) const noexcept = 0;

    /**
     * Loads the Song. This is called only if the "doesFormatMatch" is successful.
     * @param inputStream the stream, rewind.
     * @param configuration the possible configuration.
     * @return the result. Never null, but the Song might be.
     */
    virtual std::unique_ptr<Result> loadSong(juce::InputStream& inputStream, const ImportConfiguration& configuration) noexcept = 0;

    /** @return the format of the Importer. */
    virtual ImportedFormat getFormat() noexcept = 0;
};

}   // namespace arkostracker


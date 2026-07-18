#pragma once

#include <juce_core/juce_core.h>

#include "OptionalValue.h"

namespace arkostracker 
{

/** How critical is the level of a line. */
enum class ReportLevel
{
    warning,
    error
};

/** A text and level, plus a possible line number. */
class ReportLine
{
public:
    /**
        Constructor.
        @param level the level of the line.
        @param text the text to show.
        @param lineNumber the line number where the report is about, or -1 if unknown or not relevant.
    */
    ReportLine(ReportLevel level, juce::String text, OptionalInt lineNumber = -1);

    /** @return the level of the line. */
    ReportLevel getLevel() const noexcept;
    /** @return the text. */
    const juce::String& getText() const noexcept;
    /** @return the line number, if any. */
    OptionalInt getLineNumber() const noexcept;

private:
    ReportLevel level;                  // The level of the line.
    juce::String text;                  // The text to show.
    OptionalInt lineNumber;             // The line number where the report is about, if any.
};



/** A report for any operation that would require to log possibly several errors, along with possible lines. */
class ErrorReport
{
public:
    /**
     * Constructor of an empty report.
     * @param maximumLines how many lines of warnings/errors the report can hold. 0 not to have any limit.
     */
    explicit ErrorReport(int maximumLines = 1000);

    /** @return how many lines the report contains. */
    int getLineCount() const noexcept;
    /** @return the report line at the desired index. */
    const ReportLine& getLine(int index) const noexcept;

    /** @return the warning count this report holds. */
    int getWarningCount() const noexcept;
    /** @return the error count this report holds. */
    int getErrorCount() const noexcept;

    /** @return whether no critical error was found. Basically, if everything went fine, this returns true (though warnings may have happened). */
    bool isOk() const noexcept;
    /** @return true if no error and no warnings have been stored. */
    bool isEmpty() const noexcept;

    /**
     * Adds the lines from the given Report to the current one.
     * @param sourceReport the report to get the data from.
     */
    void addFrom(const ErrorReport& sourceReport) noexcept;

    /**
     * Adds a warning line.
     * @param text a human-readable string.
     * @param lineNumber the line number where the report is about, if any.
     */
    void addWarning(const juce::String& text, OptionalInt lineNumber = { }) noexcept;

    /**
     * Adds an error line.
     * @param text a human-readable string.
     * @param lineNumber the line number where the report is about, if any.
     */
    void addError(const juce::String& text, OptionalInt lineNumber = { }) noexcept;


private:
    int maximumLines;                               // How many lines of warnings/errors the report can hold. 0 not to have any limit.
    std::vector<ReportLine> reportLines;            // The lines of reports.
    int warningCount;                               // The warning count this report holds.
    int errorCount;                                 // The error count this report holds.

    /**
     * Adds an error/warning line. Nothing happens if the limit has been reached.
     * @param text the text.
     * @param level the level of the problem.
     * @param lineNumber the line number.
     */
    void addLine(const juce::String& text, ReportLevel level, OptionalInt lineNumber) noexcept;
};



}   // namespace arkostracker


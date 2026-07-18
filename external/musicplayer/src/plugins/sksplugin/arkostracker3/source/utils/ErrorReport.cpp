#include "ErrorReport.h"

#include <utility>

namespace arkostracker 
{

// ReportLine.
// ==========================================================================

ReportLine::ReportLine(ReportLevel pLevel, juce::String pText, OptionalInt pLineNumber) :
        level(pLevel),
        text(std::move(pText)),
        lineNumber(pLineNumber)
{
}

ReportLevel ReportLine::getLevel() const noexcept
{
    return level;
}

const juce::String& ReportLine::getText() const noexcept
{
    return text;
}

OptionalInt ReportLine::getLineNumber() const noexcept
{
    return lineNumber;
}


// ErrorReport.
// ==========================================================================
ErrorReport::ErrorReport(int pMaximumLines) :
        maximumLines(pMaximumLines),
        reportLines(),
        warningCount(0),
        errorCount(0)
{
}

int ErrorReport::getLineCount() const noexcept
{
    return static_cast<int>(reportLines.size());
}

const ReportLine& ErrorReport::getLine(const int index) const noexcept
{
    jassert(index >= 0);

    const auto indexSizeT = static_cast<size_t>(index);
    jassert(indexSizeT < reportLines.size());

    return reportLines[indexSizeT];
}

int ErrorReport::getWarningCount() const noexcept
{
    return warningCount;
}

int ErrorReport::getErrorCount() const noexcept
{
    return errorCount;
}

bool ErrorReport::isOk() const noexcept
{
#ifdef JUCE_DEBUG
    for (const auto& reportLine : reportLines) {
        if (reportLine.getLevel() == ReportLevel::error) {
            DBG("REPORT ERROR: " + reportLine.getText());
        }
    }
#endif

    return (errorCount == 0);
}

bool ErrorReport::isEmpty() const noexcept
{
    return ((warningCount == 0) && (errorCount == 0));
}

void ErrorReport::addFrom(const ErrorReport& sourceReport) noexcept
{
    for (int index = 0, count = sourceReport.getLineCount(); index < count; ++index) {
        auto& line = sourceReport.getLine(index);
        addLine(line.getText(), line.getLevel(), line.getLineNumber());
    }
}

void ErrorReport::addWarning(const juce::String& text, OptionalInt lineNumber) noexcept
{
    addLine(text, ReportLevel::warning, lineNumber);
}

void ErrorReport::addError(const juce::String& text, OptionalInt lineNumber) noexcept
{
    addLine(text, ReportLevel::error, lineNumber);
}

void ErrorReport::addLine(const juce::String& text, ReportLevel level, OptionalInt lineNumber) noexcept
{
    // Limit not reached?
    if ((maximumLines == 0) || (static_cast<int>(reportLines.size()) < maximumLines)) {
        reportLines.emplace_back(level, text, lineNumber);

        switch (level) {
            default:
                jassertfalse;       // Unknown level!
            case ReportLevel::warning:
                ++warningCount;
                break;
            case ReportLevel::error:
                ++errorCount;
                break;
        }
    }
}


}   // namespace arkostracker


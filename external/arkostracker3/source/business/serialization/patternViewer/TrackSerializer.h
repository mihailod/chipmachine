#pragma once

#include <memory>
#include <vector>

#include <juce_core/juce_core.h>

#include "../../../song/cells/Cell.h"
#include "../../../ui/patternViewer/controller/CaptureFlags.h"
#include "../../../song/cells/SpecialCell.h"
#include "../../../ui/patternViewer/controller/PasteData.h"

namespace arkostracker 
{

/** Serializes and deserializes Tracks/Special Tracks from the PatternViewer. It is specific because it also holds capture flags. */
class TrackSerializer
{
public:
    // Public nodes, useful for testing.
    static const juce::String nodeRoot;
    static const juce::String nodeHeight;
    static const juce::String nodeTrack;
    static const juce::String nodeCaptureFlags;
    static const juce::String nodeIsNoteCaptured;
    static const juce::String nodeIsInstrumentCaptured;
    static const juce::String nodeIsEffect1Captured;
    static const juce::String nodeIsEffect2Captured;
    static const juce::String nodeIsEffect3Captured;
    static const juce::String nodeIsEffect4Captured;
    static const juce::String nodeSpeedTrack;
    static const juce::String nodeEventTrack;

    /** Prevents instantiation. */
    TrackSerializer() = delete;

    /**
     * Serializes the given data into XML.
     * @param tracksOfCells the cells, for each tracks. May be empty if there are no "normal" tracks, but if not, there must be at least one cell in a track.
     * @param captureFlags the capture flags, for each track. Must be the same count as the tracksOfCells above.
     * @param speedCells the speed cells. Empty if there is none to copy.
     * @param eventCells the event cells. Empty if there is none to copy.
     * @return the XML node. Nullptr if an error occurred.
     */
    static std::unique_ptr<juce::XmlElement> serialize(const std::vector<std::vector<Cell>>& tracksOfCells, const std::vector<CaptureFlags>& captureFlags,
                                                const std::vector<SpecialCell>& speedCells, const std::vector<SpecialCell>& eventCells) noexcept;

    /**
     * Tries to deserialize the node. If fails, returns an invalid PasteData.
     * @param parentNode the parent node.
     * @return the PasteData, valid or not.
     */
    static PasteData deserialize(const juce::XmlElement& parentNode) noexcept;

private:
    /**
     * Serializes a normal Track.
     * @param parentNode the parent node.
     * @param cells the cells. May be empty.
     * @param captureFlag the flags indicating what to paste.
     */
    static void serializeTrack(juce::XmlElement& parentNode, const std::vector<Cell>& cells, const CaptureFlags& captureFlag) noexcept;

    /**
     * Serializes a normal Track.
     * @param parentNode the parent node.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param specialCells the cells. May be empty.
     */
    static void serializeSpecialTrack(juce::XmlElement& parentNode, bool isSpeedTrack, const std::vector<SpecialCell>& specialCells) noexcept;

    /**
     * @return the extracted cells from the given Special node (speed or event track). Empty if there is none.
     * @param height the height the selection has.
     * @param specialNode the node to extract the data from. If nullptr, it means there is no data.
     */
    static std::vector<SpecialCell> deserializeSpecialCells(int height, const juce::XmlElement* specialNode) noexcept;
};

}   // namespace arkostracker

#include "TrackSerializer.h"

#include "../song/CellSerializer.h"
#include "../song/SpecialCellSerializer.h"
#include "../../../utils/MapUtil.h"
#include "../../../utils/XmlHelper.h"

namespace arkostracker 
{

const juce::String TrackSerializer::nodeRoot = "tracks";                                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TrackSerializer::nodeHeight = "height";                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TrackSerializer::nodeTrack = "track";                                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TrackSerializer::nodeCaptureFlags = "captureFlags";                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TrackSerializer::nodeIsNoteCaptured = "isNoteCaptured";                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TrackSerializer::nodeIsInstrumentCaptured = "isInstrumentCaptured";      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TrackSerializer::nodeIsEffect1Captured = "isEffect1Captured";            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TrackSerializer::nodeIsEffect2Captured = "isEffect2Captured";            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TrackSerializer::nodeIsEffect3Captured = "isEffect3Captured";            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TrackSerializer::nodeIsEffect4Captured = "isEffect4Captured";            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TrackSerializer::nodeSpeedTrack = "speedTrack";                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TrackSerializer::nodeEventTrack = "eventTrack";                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

std::unique_ptr<juce::XmlElement> TrackSerializer::serialize(const std::vector<std::vector<Cell>>& tracksOfCells, const std::vector<CaptureFlags>& captureFlags,
                                                             const std::vector<SpecialCell>& speedCells, const std::vector<SpecialCell>& eventCells) noexcept
{
    if (tracksOfCells.size() != captureFlags.size()) {
        jassertfalse;           // Should never happen!
        return nullptr;
    }

    auto rootNote = std::make_unique<juce::XmlElement>(nodeRoot);

    // Gets the height, first from the Tracks, then from the Special Tracks. It should not be 0!
    auto height = tracksOfCells.empty() ? 0 : tracksOfCells.at(0).size();
    if (height == 0) {
        // Tries with the Special Tracks.
        height = speedCells.empty() ? eventCells.size() : speedCells.size();

        if (height == 0) {
            jassertfalse;           // No height. Should never happen.
            return nullptr;
        }
    }
    XmlHelper::addValueNode(*rootNote, nodeHeight, height);

    // Encodes the Tracks, if any.
    size_t channelIndex = 0;
    for (const auto& cells : tracksOfCells) {
        serializeTrack(*rootNote, cells, captureFlags.at(channelIndex));
        ++channelIndex;
    }

    // Encodes the Special Tracks, if any.
    serializeSpecialTrack(*rootNote, true, speedCells);
    serializeSpecialTrack(*rootNote, false, eventCells);

    return rootNote;
}

void TrackSerializer::serializeTrack(juce::XmlElement& parentNode, const std::vector<Cell>& cells, const CaptureFlags& captureFlag) noexcept
{
    jassert(!cells.empty());

    auto& trackNode = XmlHelper::addNode(parentNode, nodeTrack);

    // Encodes the CaptureFlags.
    auto& captureFlagsNode = XmlHelper::addNode(trackNode, nodeCaptureFlags);
    XmlHelper::addValueNode(captureFlagsNode, nodeIsNoteCaptured, captureFlag.isNoteCaptured());
    XmlHelper::addValueNode(captureFlagsNode, nodeIsInstrumentCaptured, captureFlag.isInstrumentCaptured());
    XmlHelper::addValueNode(captureFlagsNode, nodeIsEffect1Captured, captureFlag.isEffect1Captured());
    XmlHelper::addValueNode(captureFlagsNode, nodeIsEffect2Captured, captureFlag.isEffect2Captured());
    XmlHelper::addValueNode(captureFlagsNode, nodeIsEffect3Captured, captureFlag.isEffect3Captured());
    XmlHelper::addValueNode(captureFlagsNode, nodeIsEffect4Captured, captureFlag.isEffect4Captured());

    // Encodes all the non-empty cells. So, there may not be any.
    CellSerializer cellSerializer;
    auto cellIndex = 0;
    for (const auto& cell : cells) {
        if (!cell.isEmpty()) {
            auto xmlCell = cellSerializer.serialize(cellIndex, cell);
            if (xmlCell == nullptr) {
                jassertfalse;            // Should never happen!
            } else {
                trackNode.addChildElement(xmlCell.release());       // Ownership to the parent node.
            }
        }

        ++cellIndex;
    }
}

void TrackSerializer::serializeSpecialTrack(juce::XmlElement& parentNode, const bool isSpeedTrack, const std::vector<SpecialCell>& specialCells) noexcept
{
    // No cells? Then don't do anything.
    if (specialCells.empty()) {
        return;
    }

    const auto& specialNode = isSpeedTrack ? nodeSpeedTrack : nodeEventTrack;
    auto& specialTrackNode = XmlHelper::addNode(parentNode, specialNode);

    // Encodes all the non-empty Special Cells. So, there may not be any.
    SpecialCellSerializer specialCellSerializer;
    auto specialCellIndex = 0;
    for (const auto& specialCell : specialCells) {
        if (!specialCell.isEmpty()) {
            auto xmlCell = specialCellSerializer.serialize(specialCellIndex, specialCell);
            if (xmlCell == nullptr) {
                jassertfalse;            // Should never happen!
            } else {
                specialTrackNode.addChildElement(xmlCell.release());       // Ownership to the parent node.
            }
        }

        ++specialCellIndex;
    }
}

PasteData TrackSerializer::deserialize(const juce::XmlElement& parentNode) noexcept
{
    if (parentNode.getTagName() != nodeRoot) {
        return { };
    }
    // Height?
    const auto height = XmlHelper::readInt(parentNode, nodeHeight);
    if (height <= 0) {
        return { };
    }

    // Any tracks?
    std::vector<std::vector<Cell>> tracksToCells;
    std::vector<CaptureFlags> captureFlags;
    for (const auto* trackNode : XmlHelper::getChildrenList(parentNode, nodeTrack)) {
        CellSerializer cellSerializer;

        // Reads the track cells. There may be none, if the track has no non-empty cell.
        std::unordered_map<int, Cell> indexToCell;
        for (const auto* cellNode : XmlHelper::getChildrenList(trackNode, CellSerializer::nodeCell)) {
            auto[cell, cellIndex] = cellSerializer.deserialize(*cellNode);
            if (cellIndex < 0) {
                jassertfalse;           // Invalid or not encoded cell index, abnormal.
                return { };
            }
            indexToCell[cellIndex] = cell;
        }

        // Converts the map to a vector, filling the holes with empty cells.
        const auto cells = MapUtil::createVectorFromMap<Cell, decltype(indexToCell)>(0, height, indexToCell);
        tracksToCells.push_back(cells);

        // Capture flags. Must be present.
        const auto* captureFlagNode = trackNode->getChildByName(nodeCaptureFlags);
        if (captureFlagNode != nullptr) {
            const auto isNoteCaptured = XmlHelper::readBool(*captureFlagNode, nodeIsNoteCaptured);
            const auto isInstrumentCaptured = XmlHelper::readBool(*captureFlagNode, nodeIsInstrumentCaptured);
            const auto isEffect1Captured = XmlHelper::readBool(*captureFlagNode, nodeIsEffect1Captured);
            const auto isEffect2Captured = XmlHelper::readBool(*captureFlagNode, nodeIsEffect2Captured);
            const auto isEffect3Captured = XmlHelper::readBool(*captureFlagNode, nodeIsEffect3Captured);
            const auto isEffect4Captured = XmlHelper::readBool(*captureFlagNode, nodeIsEffect4Captured);
            captureFlags.emplace_back(isNoteCaptured, isInstrumentCaptured, isEffect1Captured, isEffect2Captured, isEffect3Captured, isEffect4Captured);
        } else {
            jassertfalse;           // No CaptureFlags node! Abnormal.
        }
    }

    if (tracksToCells.size() != captureFlags.size()) {
        jassertfalse;       // Abnormal.
        return { };
    }

    // Speed Cells.
    const auto speedCells = deserializeSpecialCells(height, parentNode.getChildByName(nodeSpeedTrack));
    const auto eventCells = deserializeSpecialCells(height, parentNode.getChildByName(nodeEventTrack));

    return { height, tracksToCells, captureFlags, speedCells, eventCells };
}

std::vector<SpecialCell> TrackSerializer::deserializeSpecialCells(const int height, const juce::XmlElement* specialNode) noexcept
{
    if (specialNode == nullptr) {
        return { };            // No data.
    }

    SpecialCellSerializer specialCellSerializer;

    // Reads the cells. There may be none, if the track has no non-empty cell.
    std::unordered_map<int, SpecialCell> indexToSpecialCell;
    for (const auto& specialCellNode : XmlHelper::getChildrenList(specialNode, SpecialCellSerializer::nodeSpecialCell)) {
        auto[specialCell, cellIndex] = specialCellSerializer.deserialize(*specialCellNode);
        if (cellIndex < 0) {
            return { };
        }
        indexToSpecialCell[cellIndex] = specialCell;
    }

    // Converts the map to a vector, filling the holes with empty cells.
    return MapUtil::createVectorFromMap<SpecialCell, decltype(indexToSpecialCell)>(0, height, indexToSpecialCell);
}


}   // namespace arkostracker


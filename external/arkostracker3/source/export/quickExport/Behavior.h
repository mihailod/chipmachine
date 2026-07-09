#pragma once

#include "../ExportConfiguration.h"
#include "../PlayerType.h"
#include "../../song/Song.h"
#include "../../ui/utils/backgroundTask/BackgroundTaskWithProgress.h"

namespace arkostracker
{

class SourceGeneratorConfiguration;

/** Indicates what to perform according to the player or DSK/SNA. */
class Behavior
{
public:
    class InnerBehavior
    {
    public:
        virtual ~InnerBehavior() = default;
        virtual std::unique_ptr<juce::MemoryBlock> buildAdditionalMemoryBlock() = 0;
    };

    class InnerBehaviorAkyDigidrums : public InnerBehavior
    {
    public:
        InnerBehaviorAkyDigidrums(std::shared_ptr<const Song> song, ExportConfiguration exportConfiguration);
        std::unique_ptr<juce::MemoryBlock> buildAdditionalMemoryBlock() override;

    private:
        std::shared_ptr<const Song> song;
        ExportConfiguration exportConfiguration;
    };

    Behavior(const std::shared_ptr<const Song>& song, ExportConfiguration exportConfiguration, PlayerType playerType, bool toDsk, juce::String dskFilePath,
        juce::String startAddressString, std::unique_ptr<InnerBehavior> innerBehavior);
    virtual ~Behavior() = default;

    /** @return the Behavior to use according to the given PlayerType and export type. */
    static std::unique_ptr<Behavior> buildBehavior(const std::shared_ptr<const Song>& song, const ExportConfiguration& exportConfiguration, PlayerType playerType,
        bool toDsk, juce::String dskFilePath, juce::String startAddressString);

    virtual std::vector<juce::String> buildInitialLines() = 0;
    virtual std::vector<juce::String> buildClosingLines() = 0;

    /** @return a possible MemoryBlock to add before the end. Empty of not needed, nullptr if an error occurred */
    virtual std::unique_ptr<juce::MemoryBlock> buildAdditionalMemoryBlock();

protected:
    std::shared_ptr<const Song> song;
    ExportConfiguration exportConfiguration;
    PlayerType playerType;
    bool toDsk;
    juce::String dskFilePath;
    juce::String startAddressString;
    std::unique_ptr<InnerBehavior> innerBehavior;
};

class BehaviorDsk : public Behavior
{
public:
    BehaviorDsk(const std::shared_ptr<const Song>& song, const ExportConfiguration& exportConfiguration, PlayerType playerType, bool toDsk, juce::String dskFilePath,
        juce::String startAddressString, std::unique_ptr<InnerBehavior> innerBehavior);

    std::vector<juce::String> buildInitialLines() override;
    std::vector<juce::String> buildClosingLines() override;
};

class BehaviorSna : public Behavior
{
public:
    BehaviorSna(const std::shared_ptr<const Song>& song, const ExportConfiguration& exportConfiguration, PlayerType playerType, bool toDsk, juce::String dskFilePath,
        juce::String startAddressString, std::unique_ptr<InnerBehavior> innerBehavior);

    std::vector<juce::String> buildInitialLines() override;
    std::vector<juce::String> buildClosingLines() override;
};

}   // namespace arkostracker

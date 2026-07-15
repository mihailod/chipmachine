#pragma once

#include <musicplayer/src/chipplugin.h>

namespace chipmachine {

class GZPlugin : public musix::ChipPlugin
{
public:
    GZPlugin() = default;
    explicit GZPlugin(std::vector<std::shared_ptr<musix::ChipPlugin>>& plugins)
        : plugins(plugins)
    {}
    [[nodiscard]] virtual std::string name() const override
    {
        return "GZPlugin";
    }
    musix::ChipPlayer* fromFile(const std::string& fileName) override;

    bool canHandle(const std::string& name) override;
    // MUST stay in sync with canHandle(). The base class defaults this to EMPTY,
    // and callers that need the set up front -- the archive track picker
    // (MusicPlayerList::archiveExtensions), the cmtest priority_map audit --
    // see only what this returns.
    // This is a WRAPPER, not a format: it gunzips and re-dispatches to whichever
    // plugin handles the decompressed file. Declaring "gz" is still correct --
    // that is the extension it routes on -- but note it also puts .gz in the
    // picker's "song" bucket, so a .gz member inside a zip is now offered as a
    // track and unwrapped at play time, which is what the app does with a loose
    // .gz anyway.
    std::set<std::string> getSupportedExtensions() const override
    {
        return { "gz" };
    }

private:
    std::vector<std::shared_ptr<musix::ChipPlugin>> plugins;
};

} // namespace chipmachine

#pragma once

#include "../../chipplugin.h"

namespace musix {

class PTKPlugin : public ChipPlugin
{
public:
    PTKPlugin();
    virtual ~PTKPlugin() = default;
    std::string name() const override { return "ProTrekkr"; }
    bool canHandle(const std::string& name) override;
    // MUST stay in sync with canHandle(). Callers that need the set up front --
    // the archive track picker (MusicPlayerList::archiveExtensions) and the
    // cmtest priority_map audit -- can only see what this returns, and the base
    // class defaults it to EMPTY. While it was missing, a loose .ptk played but a
    // .ptk inside a zip was never found.
    std::set<std::string> getSupportedExtensions() const override
    {
        return { "ptk", "ntk" };
    }
    ChipPlayer* fromFile(const std::string& name) override;
};

} // namespace musix

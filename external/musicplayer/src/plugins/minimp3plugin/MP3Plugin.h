#pragma once

#include "../../chipplugin.h"

namespace musix {

class MP3Plugin : public ChipPlugin
{
public:
    std::string name() const override { return "libmpg123"; }
    bool canHandle(const std::string& name) override;
    // MUST stay in sync with canHandle(). The base class defaults this to EMPTY,
    // and callers that need the set up front (the archive track picker, the
    // cmtest priority_map audit) can only see what this returns.
    std::set<std::string> getSupportedExtensions() const override
    {
        return { "mp3" };
    }
    ChipPlayer* fromFile(const std::string& fileName) override;
    ChipPlayer* fromStream(std::shared_ptr<utils::Fifo<uint8_t>> fifo) override;
};

} // namespace musix


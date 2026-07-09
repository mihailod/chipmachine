#pragma once

#include "../../chipplugin.h"

namespace musix {

// Plays MONOTONE (.mon) music -- a PC-speaker tracker by Trixter/Hornet (a few
// square-wave tracks summed into one 1-bit channel). These are NOT Amiga
// modules: the extension collides with UADE's "Maniacs of Noise" player, which
// crashes when fed a Monotone file. We render them with the vendored, BSD-3
// PTPlayer library instead. See MonotonePlugin.cpp for the details.
class MonotonePlugin : public ChipPlugin
{
public:
    std::string name() const override { return "Monotone"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix

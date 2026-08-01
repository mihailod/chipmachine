#ifndef HTPLAYER_H
#define HTPLAYER_H

#include "../../chipplugin.h"

namespace musix {

class HTPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "HTPlugin"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    std::string displayName() const override { return "Dreamcast & Saturn (DSF/SSF)"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    std::vector<std::string> getSecondaryFiles(const std::string& name) override;
    ChipPlayer* fromFile(const std::string& fileName) override;

    // Above AdPlug/AOSDK's 0 so this keeps ".ssf"/".minissf" (136 Saturn songs).
    // AOPlugin's aosdk/eng_ssf claims the same two extensions, and NEITHER
    // plugin content-gates them -- both are extension-only -- so the winner is
    // decided purely by list order. That used to fall out of std::sort's
    // unspecified handling of equal priorities; once createPlugins() was made a
    // stable_sort, registration order would have handed them to aoplugin
    // (registered ~9 lines earlier) and quietly moved those songs onto the older
    // Audio Overload Saturn core. Stated here instead of left to line numbers:
    // Highly Theoretical is the maintained engine and already owns Dreamcast
    // .dsf outright, so keeping both Sega formats together is also what the
    // plugin filter screen should show.
    int priority() override { return 1; }
};

} // namespace musix

#endif // HTPLAYER_H

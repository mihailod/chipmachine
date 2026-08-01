#ifndef RSN_PLAYER_H
#define RSN_PLAYER_H

#include "../../chipplugin.h"

namespace musix {

class RSNPlugin : public ChipPlugin {
public:
    RSNPlugin() {}
    // RSNPlugin(std::vector<std::shared_ptr<ChipPlugin>> &plugins) :
    // plugins(plugins) {}
    virtual std::string name() const { return "RSNPlugin"; }
    // Display-only label for the TAB plugin screen; see ChipPlugin.
    virtual std::string displayName() const { return "Super Nintendo (RSN)"; }
    virtual ChipPlayer* fromFile(const std::string& fileName);

    virtual bool canHandle(const std::string& name);
    std::set<std::string> getSupportedExtensions() const override;

private:
    std::vector<std::shared_ptr<ChipPlugin>> plugins;
};

} // namespace musix

#endif // RSN_PLAYER_H

#include "VicePlugin.h"
#include <chipplugin.h>

extern "C" void vicepluginbridge_register() {
    musix::ChipPlugin::addPluginConstructor([](std::string const& config) {
        return std::make_shared<musix::VicePlugin>(config);
    });
}

#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <set>

namespace utils {
template <typename T> class Fifo;
} // namespace utils

#include "chipplayer.h"

void register_plugins();

namespace musix {

class ChipPlugin
{
public:
    virtual ~ChipPlugin() = default;

    // Must be implemented.
    //
    // This is the plugin's IDENTITY, not a label: it is a key in the
    // formatOverride / formatPlayer tables in MusicDatabase.cpp, is compared
    // literally in a few places (MusicPlayer's "ffmpeg" checks, MusicPlayerList's
    // renderer set), is what ChipPlugin::getPlugin() looks up, and is what cmtest
    // derives a plugin's testmus/ fixture directory from. Renaming one means
    // chasing all of those. To change what the TAB plugin screen SHOWS, override
    // displayName() instead and leave this alone.
    virtual std::string name() const = 0;

    // Optional human-readable label for the TAB plugin filter screen.
    //
    // Empty (the default) means "no override" and the screen falls back to
    // name(), so plugins whose name is already presentable need not implement
    // this. Only the plugin screen consults it -- it is display-only and is
    // deliberately NOT usable as a lookup key, so it is free of every constraint
    // listed on name() above.
    //
    // The screen's type-to-narrow searches whatever it displays, so overriding
    // this also makes the plugin findable by its friendly name.
    virtual std::string displayName() const { return ""; }
    virtual bool canHandle(const std::string& name) = 0;
    virtual ChipPlayer* fromFile(const std::string& fileName) = 0;

    virtual std::set<std::string> getSupportedExtensions() const { return {}; }

    virtual ChipPlayer*
    fromStream(std::shared_ptr<utils::Fifo<uint8_t>> /*unused*/)
    {
        return nullptr;
    }
    virtual int priority() { return 0; }

    // Normally a player stops playing when music is silent, but can be
    // overriden by plugin
    virtual bool checkSilence() const { return true; }

    // Return other files required for playing the provided file. The returned
    // files should normally not contain a path if it assumed they recide in
    // the same directory.
    virtual std::vector<std::string>
    getSecondaryFiles(const std::string& /*file*/)
    {
        return {};
    }

    // Plugin registration stuff

    using PluginConstructor =
        std::function<std::shared_ptr<ChipPlugin>(const std::string&)>;

    static inline bool was_registered = false;
    static inline void createPlugins(const std::string& configDir)
    {
        if (was_registered) { return; }
        was_registered = true;
        register_plugins();
        auto& plugins = getPlugins();
        for (const auto& f : constructors) {
            plugins.push_back(f(configDir));
        }

        if (plugins.empty()) {
            fprintf(stderr, "No plugins registered!\n");
        }

        std::sort(plugins.begin(), plugins.end(),
                  [](auto const& a, auto const& b) {
                      return a->priority() > b->priority();
                  });
        constructors.clear();
    }

    static inline void addPlugin(const std::shared_ptr<ChipPlugin>& plugin, bool first)
    {
        if (first) {
            getPlugins().insert(getPlugins().begin(), plugin);
        } else {
            getPlugins().push_back(plugin);
        }
    }

    template <typename PLUGIN>
    static inline void addPlugin() {
        addPlugin(std::make_shared<PLUGIN>());
    }

    static inline std::shared_ptr<ChipPlugin> getPlugin(const std::string& name)
    {
        for (auto& p : getPlugins()) {
            if (p->name() == name) { return p; }
        }
        return nullptr;
    }

    static inline void addPluginConstructor(PluginConstructor const& pc)
    {
        constructors.push_back(pc);
    }

    // Static instances of this struct is used for automatic registration of
    // linked plugins
    struct RegisterMe
    {
        explicit RegisterMe(PluginConstructor const& f)
        {
            fprintf(stderr, "DEPRECATED!\n");
            //ChipPlugin::addPluginConstructor(f);
        };
    };

    static inline std::vector<std::shared_ptr<ChipPlugin>>& getPlugins()
    {
        static std::vector<std::shared_ptr<ChipPlugin>> plugins;
        return plugins;
    }

private:
    static inline std::vector<PluginConstructor> constructors;
};

} // namespace musix

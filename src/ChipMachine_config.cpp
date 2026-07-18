#include "ChipMachine.h"

#include <coreutils/file.h>
#include <coreutils/searchpath.h>

#include <algorithm>

namespace chipmachine {

void ChipMachine::loadScrollFonts(const std::string& folder)
{
    using utils::File;

    // Resolve the folder: try as given (may be absolute or relative to cwd),
    // then relative to workDir (the resource root). NOTE: File::isDir() THROWS
    // if the path does not exist, so guard every isDir() call with exists()
    // first and short-circuit.
    File dir(folder);
    if (!dir.exists())
        dir = File((workDir / folder).string());
    if (!dir.exists() || !dir.isDir()) {
        LOGW("Scroll font folder not found: %s", folder);
        return;
    }

    // Skip if we already loaded this exact (resolved) folder -- the constructor
    // seeds the default folder and the config typically names the same one.
    if (dir.getName() == scrollFontDir && scrollEffect.fontCount() > 0)
        return;
    scrollFontDir = dir.getName();

    // Collect every .otf (case-insensitive) and sort alphabetically by filename
    // so the rotation order is stable and predictable across machines.
    std::vector<std::string> fontPaths;
    for (auto const& f : dir.listFiles()) {
        if (utils::toLower(utils::path_extension(f.getName())) == "otf")
            fontPaths.push_back(f.getName());
    }
    std::sort(fontPaths.begin(), fontPaths.end(),
              [](std::string const& a, std::string const& b) {
                  return utils::toLower(utils::path_filename(a)) <
                         utils::toLower(utils::path_filename(b));
              });

    if (fontPaths.empty()) {
        LOGW("No .otf fonts in scroll font folder: %s", dir.getName());
        return;
    }

    scrollEffect.clearFonts();
    for (auto const& p : fontPaths)
        scrollEffect.addFont(p);
    // Start each launch on a random font from the chain (rotation continues
    // from there on wrap/text-change).
    scrollEffect.randomizeFont();
    LOGD("Loaded %d scroll font(s) from %s (first: %s)",
         (int)scrollEffect.fontCount(), dir.getName(),
         scrollEffect.currentFontName());
}

void ChipMachine::setVariable(const std::string& name, int index,
                              const std::string& val)
{

    using namespace grappix;
    using namespace tween;

    // The text fields that are configurable from lua
    static std::map<std::string, TextField*> fields = {
        { "main_title", &currentInfoField[0] },
        { "main_composer", &currentInfoField[1] },
        { "main_format", &currentInfoField[2] },

        { "next_title", &nextInfoField[0] },
        { "next_composer", &nextInfoField[1] },
        { "next_format", &nextInfoField[2] },

        { "exit_title", &prevInfoField[0] },
        { "exit_composer", &prevInfoField[1] },
        { "exit_format", &prevInfoField[2] },

        { "enter_title", &outsideInfoField[0] },
        { "enter_composer", &outsideInfoField[1] },
        { "enter_format", &outsideInfoField[2] },

        { "length_field", &lengthField },
        { "time_field", &timeField },
        { "song_field", &songField },
        { "next_field", &nextField },
        { "xinfo_field", &xinfoField },
        { "search_field", &searchField },
        { "command_field", &commandField },
        { "top_status", &topStatus },
        { "source_status", &sourceStatus },
        { "toast_field", &toastField },
        { "result_field", &resultFieldTemplate },
        { "main_filter", &mainFilterField }
    };

    auto path = makeSearchPath({ workDir }, false);

    if (fields.count(name) > 0) {
        auto& f = (*fields[name]);
        if (index >= 4) {
            auto c = Color(stoll(val));
            if (name == "time_field") timeColor = c;
            f.color = c;
            if (name == "result_field") {
                markColor = c;
                markTween = Tween::make()
                                .sine()
                                .repeating()
                                .from(markColor, hilightColor)
                                .seconds(1.0);
                markTween.start();
            }
        } else {
            auto x = stod(val);
            if (index == 1)
                f.pos.x = x;
            else if (index == 2)
                f.pos.y = x;
            else
                f.scale = x;
        }
    } else if (name == "spectrum") {
        if (index <= 2)
            spectrumPos[index - 1] = stol(val);
        else if (index == 3)
            spectrumWidth = stol(val);
        else if (index == 4)
            spectrumHeight = stod(val);
        else if (index == 5)
            spectrumColorMain = Color(stoll(val));
        else
            spectrumColorSearch = Color(stoll(val));
    } else if (name == "font") {

        if (auto fontFile = findFile(path, val)) {
            font = Font(fontFile->string(), 48, 512 | Font::DISTANCE_MAP);
            for (auto& f : fields)
                f.second->setFont(font);
        } else
            throw utils::file_not_found_exception(val);

    } else if (name == "list_font") {
        if (auto fontFile = findFile(path, val)) {
            listFont =
                Font(fontFile->string(), 32, 256); // | Font::DISTANCE_MAP);
            resultFieldTemplate.setFont(listFont);
        } else
            throw utils::file_not_found_exception(val);
    } else if (name == "favicon") {
        favPos[index - 1] = stol(val);
    } else if (name == "background") {
        bgcolor = stol(val);
    } else if (name == "stars") {
        starsOn = stol(val) != 0;
    } else if (name == "top_left") {
        topLeft[index - 1] = stol(val);
        updateLists();
    } else if (name == "down_right") {
        downRight[index - 1] = stol(val);
        updateLists();
    } else if (name == "scroll") {
        switch (index) {
        case 1: scrollEffect.scrolly = stol(val); break;
        case 2: scrollEffect.scrollsize = stod(val); break;
        case 3: scrollEffect.scrollspeed = stol(val); break;
        case 4: loadScrollFonts(val); break;
        case 5: scrollEffect.set("sine_amplitude", val); break;
        case 6: scrollEffect.set("sine_frequency", val); break;
        case 7: scrollEffect.set("sine_speed", val); break;
        case 8: scrollEffect.set("sine_on", val); break;
        case 9: scrollEffect.set("sine_interval", val); break;
        case 10: scrollEffect.set("sine_transition", val); break;
        case 11: scrollEffect.set("vbob_amplitude", val); break;
        case 12: scrollEffect.set("vbob_speed", val); break;
        case 13: scrollEffect.set("vbob_on", val); break;
        case 14: scrollEffect.set("vbob_interval", val); break;
        case 15: scrollEffect.set("vbob_transition", val); break;
        case 16: scrollEffect.font_swap_interval = stod(val); break;
        }
    } else if (name == "hilight_color") {
        hilightColor = Color(stoll(val));
        markTween = Tween::make()
                        .sine()
                        .repeating()
                        .from(markColor, hilightColor)
                        .seconds(1.0);
        markTween.start();
    } else if (name == "result_lines") {
        numLines = stol(val);
        songList.setVisible(numLines);
        // The help list may need more slots than numLines to fit its dividers;
        // fitCommandList() accounts for them (and falls back to numLines).
        fitCommandList();
    }
}

} // namespace chipmachine

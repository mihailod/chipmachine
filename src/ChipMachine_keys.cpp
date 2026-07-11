#include "ChipMachine.h"
#include "modutils.h"
#include <algorithm>
#include <chrono>
#include <random>

using tween::Tween;

namespace chipmachine {

// see also: https://github.com/Attnam/ivan/pull/407
static std::mt19937
    rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());

void ChipMachine::addKey(uint32_t key, statemachine::Condition const& cond,
                         std::string const& cmd)
{

    auto screen = currentScreen;
    bool onMain = false;
    bool onSearch = false;

    currentScreen = NO_SCREEN;
    if (!cond.check()) {
        currentScreen = MAIN_SCREEN;
        onMain = cond.check();
        currentScreen = SEARCH_SCREEN;
        onSearch = cond.check();
    }
    currentScreen = screen;

    auto it = std::find(commands.begin(), commands.end(), cmd);
    if (it != commands.end()) {
        smac.add(key, cond,
                 static_cast<uint32_t>(std::distance(commands.begin(), it)));
        if (key == keycodes::BACKSPACE) return;
        if (it->shortcut == "") {
            std::string name;
            if (key & SHIFT) name += "shift+";
            if (key & ALT) name += "alt+";
            if (key & CTRL) name += "ctrl+";
            key &= 0xffff;
            if (key >= keycodes::UP && key <= keycodes::F12)
                name += utils::toLower(key_names[key - keycodes::UP]);
            else if (key < 0x80)
                name.append(1, tolower(key));
            if (onSearch) name += " [search]";
            if (onMain) name += " [main]";
            it->shortcut = name;
        }
    }
}

void ChipMachine::setupRules()
{

    using namespace statemachine;

    addKey(keycodes::F1, "show_main");
    addKey(keycodes::F2, "show_search");
    addKey(
        { keycodes::UP, keycodes::DOWN, keycodes::PAGEUP, keycodes::PAGEDOWN },
        if_equals(currentScreen, MAIN_SCREEN), "show_search");
    addKey(keycodes::F5, "play_pause");
    addKey(keycodes::F3, "show_command");
    addKey(keycodes::F9, "show_advanced");

    addKey(keycodes::BACKSPACE,
           if_equals(currentScreen, SEARCH_SCREEN) && if_null(currentDialog) &&
               if_false(haveSearchChars),
           "clear_filter");

    addKey(keycodes::ESCAPE, if_not_null(currentDialog), "close_dialog");
    addKey(keycodes::ESCAPE, if_equals(currentScreen, COMMAND_SCREEN),
           "clear_command");
    addKey(keycodes::ESCAPE, if_equals(currentScreen, SEARCH_SCREEN),
           "clear_search");

    addKey(keycodes::F6, "next_song");
    addKey(keycodes::ENTER, if_equals(currentScreen, MAIN_SCREEN), "next_song");
    addKey(keycodes::ENTER, if_equals(currentScreen, SEARCH_SCREEN),
           "play_song");
    addKey(keycodes::ENTER, if_equals(currentScreen, COMMAND_SCREEN),
           "execute_selected_command");
    addKey(keycodes::ENTER, if_equals(currentScreen, ADVANCED_SCREEN),
           "select_filter");
    addKey(keycodes::ESCAPE, if_equals(currentScreen, ADVANCED_SCREEN),
           "close_advanced");
    addKey(keycodes::ENTER | SHIFT, if_equals(currentScreen, SEARCH_SCREEN),
           "enque_song");
    addKey(keycodes::F9, if_equals(currentScreen, SEARCH_SCREEN), "enque_song");
    addKey(keycodes::DOWN | SHIFT, if_equals(currentScreen, SEARCH_SCREEN),
           "next_composer");
    addKey(keycodes::F7, if_equals(currentScreen, SEARCH_SCREEN),
           "add_list_favorite");
    addKey(keycodes::F7, if_equals(currentScreen, MAIN_SCREEN),
           "add_current_favorite");
    addKey(keycodes::F8, "clear_songs");
    addKey(keycodes::LEFT,
           if_not_equals(currentScreen, COMMAND_SCREEN) &&
               if_null(currentDialog),
           "prev_subtune");
    addKey(keycodes::RIGHT,
           if_not_equals(currentScreen, COMMAND_SCREEN) &&
               if_null(currentDialog),
           "next_subtune");
    addKey(keycodes::F4, "layout_screen");
    addKey(keycodes::ESCAPE | SHIFT, "quit");
    addKey(keycodes::F4 | ALT, "quit");

    addKey('d' | CTRL, "download_current");
    addKey('z' | CTRL, "next_screenshot");
    addKey('n' | CTRL, "next_scroll_font");
    addKey('r' | CTRL, "random_shuffle");
    addKey('f' | CTRL, "format_shuffle");
    addKey('c' | CTRL, "composer_shuffle");
    addKey('s' | CTRL, "result_shuffle");
    addKey('o' | CTRL, "collection_shuffle");
    addKey('g' | CTRL, "favorite_shuffle");
    addKey('-', "volume_down");
    addKey({ '+', '=' }, "volume_up");
    addKey('m' | CTRL, "Spectrum_Analyzer_Mode");
    addKey(keycodes::TAB, "toggle_command");
    addKey(keycodes::HOME, "local_file_playback");
    std::string empty("");
    addKey('i' | CTRL, if_equals(filter, empty), "set_collection_filter");
    addKey('i' | CTRL, if_not_equals(filter, empty), "clear_filter");
}

void ChipMachine::showScreen(Screen screen)
{
    if (currentScreen != screen) {
        hasMoved = (screen != SEARCH_SCREEN);
        currentScreen = screen;
        if (screen == MAIN_SCREEN) {
            Tween::make().to(spectrumColor, spectrumColorMain).seconds(0.5);
            Tween::make().to(scrollEffect.alpha, 1.0).seconds(0.5);
        } else {
            Tween::make().to(spectrumColor, spectrumColorSearch).seconds(0.5);
            Tween::make().to(scrollEffect.alpha, 0.0).seconds(0.5);
        }
        // Sync the platform-logo previews to the (new) screen: show them for the
        // current selection on the search / F9 filter screen, clear elsewhere.
        updateSearchLogo();
        updateFilterLogo();
    }
}

SongInfo ChipMachine::getSelectedSong()
{
    int i = songList.selected();
    if (i < 0) return SongInfo();
    return musicDatabase.getSongInfo(iquery->getIndex(i));
}

void ChipMachine::shuffleFavorites()
{
    std::vector<SongInfo> target =
        musicDatabase.getPlaylist(currentPlaylistName);
    std::shuffle(target.begin(), target.end(), rng);
    playSongs(target);
}

void ChipMachine::shuffleSongs(int what, int limit)
{
    std::vector<SongInfo> target;
    SongInfo match =
        (currentScreen == SEARCH_SCREEN) ? getSelectedSong() : dbInfo;

    LOGD("SHUFFLE %s / %s", match.composer, match.format);

    if (!(what & Shuffle::Format)) match.format = "";
    if (!(what & Shuffle::Composer)) match.composer = "";
    if (!(what & Shuffle::Collection)) match.path = "";
    match.title = match.game;

    musicDatabase.getSongs(target, match, limit, true);
    playSongs(target);
}

void ChipMachine::playSongs(std::vector<SongInfo> const& songs)
{
    player.clearSongs();
    for (const auto& s : songs) {
        if (!utils::endsWith(s.path, ".plist")) player.addSong(s);
    }
    showScreen(MAIN_SCREEN);
    player.nextSong();
}

void ChipMachine::updateKeys()
{

    using namespace grappix;

    haveSearchChars = (iquery->getString().length() > 0);

    searchUpdated = false;
    auto last_selection = songList.selected();
    auto last_adv_selection = advancedList.selected();

    auto key = screen.get_key();

    if ((key & 0x80000000) != 0) return;

    // LOGD("KEY %x", key);

    if (indexingDatabase) return;

    uint32_t event = key;

    VerticalList* currentList = nullptr;
    if (currentScreen == SEARCH_SCREEN)
        currentList = &songList;
    else if (currentScreen == COMMAND_SCREEN)
        currentList = &commandList;
    else if (currentScreen == ADVANCED_SCREEN)
        currentList = &advancedList;

    bool ascii = (event >= 'A' && event <= 'Z');
    if (ascii) event = tolower(event);
    if (screen.key_pressed(keycodes::SHIFT_LEFT) ||
        screen.key_pressed(keycodes::SHIFT_RIGHT)) {
        if (ascii)
            event = toupper(event);
        else if (event == keycodes::DOWN)
            key = keycodes::UP;
        else
            event |= SHIFT;
    }

    if (screen.key_pressed(keycodes::CTRL_LEFT) ||
        screen.key_pressed(keycodes::CTRL_RIGHT)) {
        if (event == keycodes::DOWN)
            key = keycodes::PAGEDOWN;
        else if (event == keycodes::UP)
            key = keycodes::PAGEUP;
        else
            event |= CTRL;
    }
    if (screen.key_pressed(keycodes::ALT_LEFT) ||
        screen.key_pressed(keycodes::ALT_RIGHT))
        event |= ALT;

    if ((event & (CTRL | SHIFT)) == 0 && currentList) {
        // The F9 platform list wraps around: Up from the first entry goes to the
        // last, Down from the last goes back to the first.
        int n = advancedList.size();
        if (currentScreen == ADVANCED_SCREEN && key == keycodes::UP &&
            advancedList.selected() == 0)
            advancedList.select(n - 1);
        else if (currentScreen == ADVANCED_SCREEN && key == keycodes::DOWN &&
                 advancedList.selected() == n - 1)
            advancedList.select(0);
        else
            currentList->onKey(key);
    }

    if (event == (keycodes::RIGHT | SHIFT)) event = keycodes::LEFT;

    lastKey = key;

    if (!smac.put_event(event)) {
        if ((key >= ' ' && key <= 'z') || key == keycodes::LEFT ||
            key == keycodes::RIGHT || key == keycodes::BACKSPACE ||
            key == keycodes::ESCAPE || key == keycodes::ENTER) {
            if (currentDialog != nullptr) {
                currentDialog->on_key(event);
            } else if (currentScreen == COMMAND_SCREEN) {
                commandField.on_key(event);
                auto ctext = commandField.getText();
                if (ctext == "")
                    clearCommand();
                else {
                    matchingCommands.resize(commands.size());
                    int j = 0;
                    for (int i = 0; i < commands.size(); i++) {
                        if (utils::toLower(commands[i].name).find(ctext) !=
                            std::string::npos)
                            matchingCommands[j++] = &commands[i];
                    }
                    matchingCommands.resize(j);
                }
                commandList.setTotal(matchingCommands.size());
            } else {
                if (hasMoved && event != ' ' && event != keycodes::BACKSPACE)
                    searchField.setText("");
                hasMoved = false;
                showScreen(SEARCH_SCREEN);
                if (event >= 0x20 && event <= 0xff) event = tolower(event);
                searchField.on_key(event);
                searchUpdated = true;
            }
        }
    }
    while (smac.actionsLeft() > 0) {
        auto action = smac.next_action();
        commands[action.id].fn();
    }

    if (songList.selected() != last_selection && iquery->numHits() > 0) {
        int i = songList.selected();
        SongInfo song = musicDatabase.getSongInfo(iquery->getIndex(i));
        auto ext = getTypeFromName(song.path);
        // UnExoticA uses Amiga "prefix-form" member names ("cust.title_and_ingame",
        // "fp.title", ...) where the part after the dot is a song descriptor, not a
        // file type. getTypeFromName surfaces that descriptor as a bogus
        // "(TITLE)" / "(TITLE_AND_INGAME)" parenthetical. song.format already names
        // the real format (CUSTOM, NOISE PACKER 3.X), so drop the parenthetical.
        if (utils::startsWith(song.path, "unexotica::")) ext = "";
        // Podcasts are streamed audio; the enclosure "extension" is derived from
        // the URL and may carry a query string (".mp3?p=f"). Drop it -> "Podcast".
        if (MusicDatabase::classifyFormat(song.format, song.path) == PODCAST)
            ext = "";
        // Drop a redundant extension that just repeats the format, so we show
        // "MP3" / "M3U" rather than "MP3 (MP3)" / "M3U (M3U)" (and likewise for
        // any song whose format string already equals its file extension).
        if (utils::toLower(ext) == utils::toLower(song.format)) ext = "";

        bool isoffline = remoteLoader.isOffline(song.path);
	// "+" = served straight from a local_dir mirror on disk (thus never
	// cached); "*" = a cached remote file. isLocalFile tracks the real on-disk
	// condition load() serves from, with isLocalAsset's prefix check as a
	// cheap belt-and-suspenders for the app-shipped collections.
	bool islocal = remoteLoader.isLocalFile(song.path) ||
	               RemoteLoader::isLocalAsset(song.path);
	if (islocal) {
	    topStatus.setText(utils::format("Format: %s (%s)%s", song.format,ext, "+"));	
	} else {
            if (ext != "") topStatus.setText(utils::format("Format: %s (%s)%s", song.format,ext, isoffline ? "*" : ""));
            else topStatus.setText(utils::format("Format: %s %s", song.format, isoffline ? "*" : ""));
        }
        searchField.visible(false);
        filterField.visible(false);
        topStatus.visible(true);
        updateSearchLogo();
    }

    // Refresh the F9 filter's centred platform-logo backdrop when the highlight
    // moves to a different platform/category.
    if (currentScreen == ADVANCED_SCREEN &&
        advancedList.selected() != last_adv_selection)
        updateFilterLogo();

    if (searchUpdated) {
        auto s = searchField.getText();
        if (s[0] == '\\') {
            int pos = s.find(' ');
            if (pos != std::string::npos) {
                auto f = s.substr(1, pos - 1);
                if (f != filter) {
                    filter = f;
                    s = s.substr(pos + 1);
                    searchField.setText(s);
                }
            }
        }

        if (filter != filterField.getText()) {
            LOGD("Filter now %s", filter);
            filterField.setText(filter);
            musicDatabase.setFilter(filter);
            iquery->invalidate();
        }

        iquery->setString(s);
        // Prompt hint for an empty query under a platform filter: a small filter
        // auto-lists everything ("showing all N"), a large one waits for input
        // ("type to search N"). Otherwise just a bare "#".
        if (s.empty() && iquery->numHits() > 0)
            searchField.setPrompt(
                utils::format("# [showing all %s %s tunes]",
                              withCommas(iquery->numHits()), selectedFilterName));
        else if (s.empty() && activeFilterCount > 0)
            searchField.setPrompt(utils::format("# [type to search %s %s tunes]",
                                                withCommas(activeFilterCount),
                                                selectedFilterName));
        else if (s.empty() && selectedFilterName.empty() &&
                 !filterCounts.empty())
            searchField.setPrompt(utils::format("# [type to search %s tunes]",
                                                withCommas(filterCounts[0])));
        else
            searchField.setPrompt("#");
        searchField.visible(true);
        filterField.visible(true);
        searchField.pos.x = filterField.pos.x + filterField.getWidth() + 5;
        topStatus.visible(false);
        songList.setTotal(iquery->numHits());
        // The result set (and thus the song under the cursor) just changed, even
        // if the selection index didn't, so refresh the platform-logo preview.
        updateSearchLogo();
        searchUpdated = false;
    }
}

} // namespace chipmachine

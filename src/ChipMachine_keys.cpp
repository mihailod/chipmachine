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

std::mt19937& ChipMachine::shuffleRng()
{
    return rng;
}

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
            else if (key == ' ')
                name += "space bar";
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

    addKey(
        { keycodes::UP, keycodes::DOWN, keycodes::PAGEUP, keycodes::PAGEDOWN },
        if_equals(currentScreen, MAIN_SCREEN), "show_search");
    // Space toggles play/pause during playback. On the main screen it always
    // pauses; on the search screen it only pauses while the query is empty --
    // once the user has typed a word, space is a normal search separator (a
    // leading space is never a valid first search character anyway).
    addKey(' ', if_equals(currentScreen, MAIN_SCREEN), "pause_/_resume_playback");
    addKey(' ',
           if_equals(currentScreen, SEARCH_SCREEN) && if_null(currentDialog) &&
               if_false(haveSearchChars),
           "pause_/_resume_playback");
    // TAB cycles the four filter screens: Platforms -> Formats -> Databases ->
    // Plugins -> back to Platforms (see the command). One key for all four.
    addKey(keycodes::TAB, "cycle_platform/format/db/plugin_filters");

    addKey(keycodes::BACKSPACE,
           if_equals(currentScreen, SEARCH_SCREEN) && if_null(currentDialog) &&
               if_false(haveSearchChars),
           "clear_filter");

    addKey(keycodes::ESCAPE, if_not_null(currentDialog), "close_dialog");
    addKey(keycodes::ESCAPE, if_equals(currentScreen, COMMAND_SCREEN),
           "clear_command");
    addKey(keycodes::ESCAPE, if_equals(currentScreen, SEARCH_SCREEN),
           "clear_search");

    // Next / previous song in the play queue. SHIFT+LEFT/RIGHT mirrors the plain
    // LEFT/RIGHT subtune step one level up: same axis, bigger unit. NOT
    // CTRL+LEFT/RIGHT -- macOS reserves those for Mission Control's
    // move-a-space, so the app never receives them. Listed before the ENTER
    // binding so the command list advertises this one.
    addKey(keycodes::RIGHT | SHIFT, "next_/_prev_shuffle_song");
    addKey(keycodes::LEFT | SHIFT, "prev_shuffle_song");
    addKey(keycodes::ENTER, if_equals(currentScreen, MAIN_SCREEN),
           "next_/_prev_shuffle_song");
    addKey(keycodes::ENTER, if_equals(currentScreen, SEARCH_SCREEN),
           "play_song");
    // ENTER on the ADVANCED (TAB) screen applies the highlighted platform
    // filter. Note there is deliberately NO ENTER binding for COMMAND_SCREEN:
    // the help menu is a display-only list, so ENTER does nothing there. Its
    // only key is the ESCAPE above.
    addKey(keycodes::ENTER, if_equals(currentScreen, ADVANCED_SCREEN),
           "select_filter");
    addKey(keycodes::ESCAPE, if_equals(currentScreen, ADVANCED_SCREEN),
           "clear_/_close_/_go_back");
    // ENTER applies the highlighted extension; ESCAPE pops back to lastScreen.
    addKey(keycodes::ENTER, if_equals(currentScreen, FORMAT_SCREEN),
           "select_format");
    addKey(keycodes::ESCAPE, if_equals(currentScreen, FORMAT_SCREEN),
           "clear_/_close_/_go_back");
    // Same for the Databases screen.
    addKey(keycodes::ENTER, if_equals(currentScreen, DATABASE_SCREEN),
           "select_database");
    addKey(keycodes::ESCAPE, if_equals(currentScreen, DATABASE_SCREEN),
           "clear_/_close_/_go_back");
    // Same for the Plugins screen.
    addKey(keycodes::ENTER, if_equals(currentScreen, PLUGIN_SCREEN),
           "select_plugin");
    addKey(keycodes::ESCAPE, if_equals(currentScreen, PLUGIN_SCREEN),
           "clear_/_close_/_go_back");
    // The single-column Formats / Databases / Plugins screens don't use
    // LEFT/RIGHT: bind them to a no-op so they're consumed rather than falling
    // through to the "start a search" handler (which would jump to search).
    addKey(keycodes::LEFT,
           (if_equals(currentScreen, FORMAT_SCREEN) ||
            if_equals(currentScreen, DATABASE_SCREEN) ||
            if_equals(currentScreen, PLUGIN_SCREEN)) &&
               if_null(currentDialog),
           "filter_list_noop");
    addKey(keycodes::RIGHT,
           (if_equals(currentScreen, FORMAT_SCREEN) ||
            if_equals(currentScreen, DATABASE_SCREEN) ||
            if_equals(currentScreen, PLUGIN_SCREEN)) &&
               if_null(currentDialog),
           "filter_list_noop");
    // CTRL+F ("favor") on both screens -- one key, and which song it favors
    // follows from where you are. Deliberately not an F key: macOS hijacks those
    // for its own system functions. (The favorites SHUFFLE is CTRL+P.)
    addKey('f' | CTRL, if_equals(currentScreen, SEARCH_SCREEN),
           "favor/unfavor_highlighted_search_result");
    addKey('f' | CTRL, if_equals(currentScreen, MAIN_SCREEN),
           "favor/unfavor_playing_song");
    // CTRL+L ("list"): move favorites into a new named playlist. Sits just before
    // CLEAR FAVORITES LIST in the help menu (registration order in setupCommands).
    addKey('l' | CTRL, "move_favorites_to_a_new_playlist");
    // CTRL+SHIFT+C, not CTRL+C (that is the composer shuffle): SHIFT and CTRL are
    // both folded into the event, so this is a distinct binding, not a variant.
    addKey('c' | CTRL | SHIFT, "clear_favorites_list");
    addKey(keycodes::LEFT,
           if_not_equals(currentScreen, COMMAND_SCREEN) &&
               if_not_equals(currentScreen, ADVANCED_SCREEN) &&
               if_not_equals(currentScreen, FORMAT_SCREEN) &&
               if_not_equals(currentScreen, DATABASE_SCREEN) &&
               if_not_equals(currentScreen, PLUGIN_SCREEN) &&
               if_null(currentDialog),
           "prev_subtune");
    addKey(keycodes::RIGHT,
           if_not_equals(currentScreen, COMMAND_SCREEN) &&
               if_not_equals(currentScreen, ADVANCED_SCREEN) &&
               if_not_equals(currentScreen, FORMAT_SCREEN) &&
               if_not_equals(currentScreen, DATABASE_SCREEN) &&
               if_not_equals(currentScreen, PLUGIN_SCREEN) &&
               if_null(currentDialog),
           "next_/_prev_subtune");
    // The platform filter is a two-column list; LEFT/RIGHT hop between the
    // columns on the same row (UP/DOWN walk within one).
    addKey(keycodes::LEFT,
           if_equals(currentScreen, ADVANCED_SCREEN) && if_null(currentDialog),
           "filter_column_left");
    addKey(keycodes::RIGHT,
           if_equals(currentScreen, ADVANCED_SCREEN) && if_null(currentDialog),
           "filter_column_right");
    addKey(keycodes::F4, "layout_screen");

    addKey('d' | CTRL, "download_playing_song");
    addKey('a' | CTRL, "next_song_artwork");
    addKey('n' | CTRL, "next_scroll_font");
    addKey('r' | CTRL, "shuffle_all_songs_randomly");
    addKey('s' | CTRL, "shuffle_search_results_randomly");
    // NOTE: this is CTRL+P ("play my favorites"), NOT CTRL+F -- CTRL+F is the
    // favor/unfavor toggle above. Format shuffle is CTRL+E; the old CTRL+O /
    // CTRL+G are retired.
    addKey('p' | CTRL, "shuffle_your_favorites");
    addKey('c' | CTRL, "shuffle_playing_song's_composer's_songs");
    addKey('b' | CTRL, "shuffle_playing_song's_database's_songs");
    addKey('e' | CTRL, "shuffle_playing_song's_format/extension");
    addKey('-', "volume_down");
    addKey({ '+', '=' }, "volume_up_/_down");
    addKey('m' | CTRL, "Spectrum_Analyzer_Mode");
    addKey('h' | CTRL, "this_help_menu");
    // CTRL+O ("open"). Frees HOME, which was its only binding.
    addKey('o' | CTRL, "local_file_playback");
    // NOTE: "set_/_clear_collection_filter" is deliberately NOT bound to a key
    // (it used to be CTRL+I). The command itself is still registered and works
    // -- see setupCommands() for why it is kept -- but with no addKey() call it
    // has no shortcut, so nothing can trigger it and clearCommand()'s
    // empty-shortcut filter keeps it out of the help menu. Do not re-add a
    // binding without reading the note at the command.

    // Keep some commands out of the command list by clearing their (display-only)
    // shortcut so the empty-shortcut filter drops them (see clearCommand()); the
    // keys still work. show_search is reached by typing / the arrow keys and is
    // advertised in the startup scroller. close_dialog / clear_command /
    // clear_search are the other ESC actions -- ESC is represented once by the
    // "CLEAR / CLOSE / GO BACK" entry, so hide these duplicates. volume_down (the
    // '-' key) is folded into the "VOLUME UP / DOWN   + / -" row. select_filter /
    // select_format / select_database (ENTER on the three filter screens) are
    // self-evident once you're there, as is filter_list_noop (LEFT/RIGHT swallowed
    // on the single-column screens). The three filter screens share one help row,
    // "CYCLE PLATFORM/FORMAT/DB FILTERS   tab" (the cycle command carries the key).
    // this_help_menu (CTRL+H) is advertised in the help title itself, so it
    // doesn't need its own row. filter_column_left/right (LEFT/RIGHT on the
    // platform-filter screen) are covered by that screen's own title hint.
    // prev_subtune (LEFT) and prev_shuffle_song (SHIFT+LEFT) are folded into the
    // "NEXT / PREV ..." rows their next_ counterparts render, exactly as
    // volume_down is folded into the volume row.
    for (auto const& name : { "show_search", "close_dialog", "clear_command",
                              "clear_search", "volume_down", "select_filter",
                              "select_format", "select_database", "select_plugin",
                              "filter_list_noop", "this_help_menu",
                              "filter_column_left", "filter_column_right",
                              "prev_subtune", "prev_shuffle_song" }) {
        auto it = std::find(commands.begin(), commands.end(), name);
        if (it != commands.end()) it->shortcut.clear();
    }

    // F4 "layout screen" hot-reloads lua/screen.lua -- a developer affordance.
    // Keep the key working, but only surface it in the command list under -d.
    if (!debugMode) {
        auto it = std::find(commands.begin(), commands.end(), "layout_screen");
        if (it != commands.end()) it->shortcut.clear();
    }
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
        // current selection on the search / Platforms / Formats / Databases /
        // Plugins screen, clear elsewhere.
        updateSearchLogo();
        updateFilterLogo();
        updateFormatLogo();
        updateDatabaseLogo();
        updatePluginLogo();
        // The help menu is display-only (typing starts a search instead), so
        // always show the title and keep the unused input field (and its cursor)
        // hidden.
        if (screen == COMMAND_SCREEN) {
            commandTitle.visible(true);
            commandField.visible(false);
        }
    }
}

SongInfo ChipMachine::getSelectedSong()
{
    int i = songList.selected();
    if (i < 0) return SongInfo();
    return musicDatabase.getSongInfo(iquery->getIndex(i));
}

void ChipMachine::clearFavorites()
{
    auto count = musicDatabase.getPlaylist(currentPlaylistName).size();
    musicDatabase.clearPlaylist(currentPlaylistName);
    // Whatever is playing cannot still be a favorite: drop the heart icon in
    // step with the list, the same way the favor/unfavor toggle does.
    isFavorite = false;
    Tween::make()
        .to(favIcon.color, Color(favColor | (0x00 << 24)))
        .seconds(0.25);
    toast(utils::format("CLEARED %d FAVORITES!", (int)count));
}

void ChipMachine::shuffleFavorites()
{
    std::vector<SongInfo> target =
        musicDatabase.getPlaylist(currentPlaylistName);
    std::shuffle(target.begin(), target.end(), rng);
    playSongs(target);
}

SongInfo ChipMachine::shuffleSeed()
{
    return (currentScreen == SEARCH_SCREEN) ? getSelectedSong() : dbInfo;
}

bool ChipMachine::isUnknownComposer(std::string const& composer)
{
    // Single source of truth now lives in SongInfo.h (::isUnknownComposer), shared
    // with the search dedup guard and the displayComposer() UI fold. Kept as a
    // thin static member so existing callers (shuffle seed) don't change.
    return ::isUnknownComposer(composer);
}

void ChipMachine::shuffleSongs(int what, int limit)
{
    std::vector<SongInfo> target;
    SongInfo match = shuffleSeed();

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
    shuffleList.clear();
    for (const auto& s : songs) {
        if (!utils::endsWith(s.path, ".plist")) shuffleList.push_back(s);
    }
    playShuffleFrom(0);
}

int ChipMachine::currentShuffleIndex() const
{
    if (shuffleList.empty()) return -1;
    // Everything still queued sits after the current song, so what has been
    // consumed identifies it. Clamped: the queue is briefly out of step while a
    // just-issued clear/add batch is still pending on the player thread.
    int index = (int)shuffleList.size() - player.listSize() - 1;
    if (index < 0) return 0;
    if (index >= (int)shuffleList.size()) return (int)shuffleList.size() - 1;
    return index;
}

void ChipMachine::playShuffleFrom(int index)
{
    player.clearSongs();
    for (size_t i = index; i < shuffleList.size(); i++) {
        player.addSong(shuffleList[i]);
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
    auto last_format_selection = formatList.selected();
    auto last_database_selection = databaseList.selected();
    auto last_plugin_selection = pluginList.selected();

    auto key = screen.get_key();

    if ((key & 0x80000000) != 0) return;

    // LOGD("KEY %x", key);

    if (indexingDatabase) return;

    // An armed CLEAR FAVORITES LIST owns the next key press: Y wipes the list,
    // anything else cancels. The key is swallowed either way, so a stray CTRL+R
    // answers the prompt instead of also starting a shuffle. Modifier presses
    // arrive as key events of their own and are skipped -- otherwise a user
    // typing a capital Y would cancel with the SHIFT that precedes it. (Key
    // releases and idle frames never get here: NO_KEY and every release carry
    // bit 31, which the guard above returns on.)
    if (pendingFavoritesClear) {
        bool isModifier =
            (key >= keycodes::SHIFT_LEFT && key <= keycodes::WINDOW_RIGHT) ||
            key == keycodes::CAPS_LOCK;
        if (!isModifier) {
            pendingFavoritesClear = false;
            if (key == 'y')
                clearFavorites();
            else
                toast("CLEAR CANCELLED");
        }
        return;
    }

    // Same one-key confirm dance as CLEAR FAVORITES above, for a DEL-armed
    // playlist deletion: Y deletes the file, anything else cancels. Re-run the
    // search afterwards -- deleting a playlist shifts every later PLAYLIST_INDEX
    // offset, so the result list must be rebuilt, not just redrawn.
    if (!pendingPlaylistDelete.empty()) {
        bool isModifier =
            (key >= keycodes::SHIFT_LEFT && key <= keycodes::WINDOW_RIGHT) ||
            key == keycodes::CAPS_LOCK;
        if (!isModifier) {
            std::string name = pendingPlaylistDelete;
            pendingPlaylistDelete.clear();
            if (key == 'y') {
                musicDatabase.deletePlaylist(name);
                // Rebuild the result list NOW. The deferred searchUpdated path
                // only runs at the end of updateKeys, which the NO_KEY guard
                // skips on idle frames -- so until the next keypress songList and
                // iquery->finalResult would still hold the deleted row, and
                // playing it would look up a file that no longer exists (the
                // vector has also shifted under the stale PLAYLIST_INDEX offsets).
                // Re-run the query synchronously to drop the row immediately.
                iquery->invalidate();
                iquery->setString(searchField.getText());
                songList.setTotal(iquery->numHits());
                songList.select(0);
                toast(utils::format("DELETED PLAYLIST %s", name));
            } else
                toast("DELETE CANCELLED");
        }
        return;
    }

    uint32_t event = key;

    VerticalList* currentList = nullptr;
    if (currentScreen == SEARCH_SCREEN)
        currentList = &songList;
    else if (currentScreen == COMMAND_SCREEN)
        currentList = &commandList;
    else if (currentScreen == ADVANCED_SCREEN)
        currentList = &advancedList;
    else if (currentScreen == FORMAT_SCREEN)
        currentList = &formatList;
    else if (currentScreen == DATABASE_SCREEN)
        currentList = &databaseList;
    else if (currentScreen == PLUGIN_SCREEN)
        currentList = &pluginList;

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

    // A modal text dialog (e.g. naming a new playlist) owns all input while it is
    // open: route keys straight to it, ahead of the state machine and the
    // background list so neither acts on them (SEARCH_SCREEN's ENTER would
    // otherwise play a song instead of committing the name). ENTER commits, ESC
    // cancels; both close it. Modifier presses and CTRL/ALT combos are swallowed
    // (so CTRL+L cannot reopen a second dialog) but do nothing.
    if (currentDialog != nullptr) {
        bool isModifier =
            (key >= keycodes::SHIFT_LEFT && key <= keycodes::WINDOW_RIGHT) ||
            key == keycodes::CAPS_LOCK;
        if (!isModifier && (event & (CTRL | ALT)) == 0)
            currentDialog->on_key(event);
        return;
    }

    if ((event & (CTRL | SHIFT)) == 0 && currentList) {
        // The TAB platform list wraps around: Up from the first entry goes to the
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

    // NOTE: SHIFT+RIGHT used to be rewritten to a plain LEFT here, making it an
    // undocumented alias for "prev subtune". SHIFT+LEFT/RIGHT are the shuffle
    // steppers now, so the rewrite is gone -- it would have swallowed the event
    // before the state machine ever saw it. Plain LEFT is still prev subtune.

    lastKey = key;

    // FORMAT_SCREEN / PLUGIN_SCREEN type-to-narrow: printable characters and
    // BACKSPACE edit the narrowing query in place instead of falling through to
    // the generic handler below (which would jump to the search screen).
    // ENTER/ESC/arrows are left to their bindings above; modified keys
    // (CTRL/ALT) are not narrowing input.
    if ((currentScreen == FORMAT_SCREEN || currentScreen == PLUGIN_SCREEN) &&
        currentDialog == nullptr && (event & (CTRL | ALT)) == 0) {
        bool printable = (event >= ' ' && event < 0x7f);
        if (printable || key == keycodes::BACKSPACE) {
            std::string& text = (currentScreen == FORMAT_SCREEN)
                                     ? formatFilterText
                                     : pluginFilterText;
            if (key == keycodes::BACKSPACE) {
                if (!text.empty()) text.pop_back();
            } else {
                text += (char)tolower((unsigned char)event);
            }
            if (currentScreen == FORMAT_SCREEN)
                rebuildFormatVisible();
            else
                rebuildPluginVisible();
            return;
        }
    }

    // DEL on the Database-filter Playlists screen (a search-results list where
    // every row is a playlist) arms deletion of the highlighted list. Only here:
    // the same key elsewhere must keep falling through to a normal search. The
    // Favorites list is the built-in heart list and is not deletable.
    if (currentScreen == SEARCH_SCREEN && selectedFilterName == "Playlists" &&
        key == keycodes::DELETE) {
        auto song = getSelectedSong();
        if (utils::startsWith(song.path, "playlist::")) {
            if (song.title == "Favorites") {
                toast("CAN'T DELETE FAVORITES", ERROR);
            } else {
                pendingPlaylistDelete = song.title;
                toast(utils::format("Y TO DELETE PLAYLIST %s!", song.title),
                      STICKY_ALERT);
            }
        }
        return;
    }

    if (!smac.put_event(event)) {
        if ((key >= ' ' && key <= 'z') || key == keycodes::LEFT ||
            key == keycodes::RIGHT || key == keycodes::BACKSPACE ||
            key == keycodes::ESCAPE || key == keycodes::ENTER) {
            if (currentDialog != nullptr) {
                currentDialog->on_key(event);
            } else if (currentScreen == COMMAND_SCREEN &&
                       !(event > ' ' && event < 0x80)) {
                // The help menu is display-only. Non-character keys do nothing
                // here (ENTER / editing keys); UP/DOWN scroll the list and ESC
                // goes back, both handled elsewhere. A printable character falls
                // through to the search path below and starts a normal search.
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
        // Source DB tag: getSongInfo prefixes song.path with the collection id
        // ("hvsc::...", "mirsoft::..."). Show it in the smaller sourceStatus field
        // just to the right of the format line. Skip the non-collection pseudo
        // prefixes (playlists / group / show / product rows).
        std::string src;
        auto dp = song.path.find("::");
        if (dp != std::string::npos) src = song.path.substr(0, dp);
        if (src == "playlist" || src == "otherplatform" || src == "othergroup" ||
            src == "podcastshow" || src == "product")
            src = "";
        sourceStatus.setText(src);
        sourceStatus.pos.x = topStatus.pos.x + topStatus.getWidth() + 15;
        sourceStatus.visible(src != "");

        searchField.visible(false);
        filterField.visible(false);
        topStatus.visible(true);
        updateSearchLogo();
    }

    // Refresh the TAB filter's centred platform-logo backdrop when the highlight
    // moves to a different platform/category.
    if (currentScreen == ADVANCED_SCREEN &&
        advancedList.selected() != last_adv_selection)
        updateFilterLogo();

    // Refresh the Formats screen's extension-logo backdrop on cursor moves.
    if (currentScreen == FORMAT_SCREEN &&
        formatList.selected() != last_format_selection)
        updateFormatLogo();

    // Same for the Databases screen's collection-logo backdrop.
    if (currentScreen == DATABASE_SCREEN &&
        databaseList.selected() != last_database_selection)
        updateDatabaseLogo();

    // Same for the Plugins screen's plugin-logo backdrop.
    if (currentScreen == PLUGIN_SCREEN &&
        pluginList.selected() != last_plugin_selection)
        updatePluginLogo();

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
        if (s.empty() && iquery->numHits() > 0) {
            // Every filter now pre-populates its whole (alphabetical) category.
            // Both cases invite narrowing -- typing sub-filters the set at any
            // size, so "type to narrow" always applies. The result list is capped
            // at the query's searchLimit (20,000), so when the true category is
            // bigger, say "first N of TOTAL" rather than falsely claiming "all";
            // the >= cap guard keeps small filters (where dedup trims a few)
            // reading "showing all".
            int shown = iquery->numHits();
            if (shown >= 20000 && activeFilterCount > shown)
                searchField.setPrompt(utils::format(
                    "# [showing first %s of %s %s tunes -- type to narrow]",
                    withCommas(shown), withCommas(activeFilterCount),
                    selectedFilterName));
            else {
                std::string prompt = utils::format(
                    "# [showing all %s %s tunes -- type to narrow]",
                    withCommas(shown), selectedFilterName);
                // On the Playlists listing every row is a deletable list, so
                // advertise the DEL shortcut right in the title (see the DEL
                // handler in updateKeys).
                if (selectedFilterName == "Playlists")
                    prompt += "  [DEL to delete selected]";
                searchField.setPrompt(prompt);
            }
        } else if (s.empty() && activeFilterCount > 0)
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
        sourceStatus.visible(false);
        songList.setTotal(iquery->numHits());
        // The result set (and thus the song under the cursor) just changed, even
        // if the selection index didn't, so refresh the platform-logo preview.
        updateSearchLogo();
        searchUpdated = false;
    }
}

} // namespace chipmachine

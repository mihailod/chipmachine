#include "ChipMachine.h"
#include "modutils.h"

#include <coreutils/environment.h>

namespace chipmachine {

void ChipMachine::setupCommands()
{
    using namespace tween;

    auto cmd = [=](std::string const& name, std::function<void()> const& f) {
        commands.emplace_back(name, f);
    };

    // Sets the displayed shortcut of the command just registered. addKey() only
    // fills a shortcut that is still empty, so seeding one here wins over the
    // plain text it would generate. Used for the mnemonic labels -- "ctrl+o(pen)"
    // rather than a bare "ctrl+o", so the row says WHY the letter is that letter
    // -- and for rows that fold two keys into one ("right / left").
    auto shortcut = [=](std::string const& s) { commands.back().shortcut = s; };

    // Listed first: the ESC key. Name renders (with '_' -> ' ') as the command
    // label "CLEAR / CLOSE / GO BACK" -- the one entry that represents every ESC
    // action (clear the search/command text, close a dialog, or pop back a
    // screen). The other ESC-bound commands are hidden from the list (their
    // shortcuts are cleared in setupRules) so ESC appears only once here. This
    // binding itself pops back from the platform-filter screen.
    cmd("clear_/_close_/_go_back", [=] {
        // On the TAB screen, ESC from inside a drilled-in group submenu pops
        // back to the top-level platform list rather than closing the screen.
        if (currentScreen == ADVANCED_SCREEN && activeFilterOptions != nullptr) {
            setFilterLevel(nullptr, drillReturnIndex);
            return;
        }
        showScreen(lastScreen);
    });

    cmd("show_search", [=]() {
        if (currentScreen != SEARCH_SCREEN) {
            showScreen(SEARCH_SCREEN);
            songList.onKey(lastKey);
        } else {
            showScreen(SEARCH_SCREEN);
        }
        searchUpdated = true;
    });

    // TAB cycles the four filter screens: (from anywhere else) Platforms ->
    // Formats -> Databases -> Plugins -> back to Platforms. ESC exits to the
    // screen you were on before entering the cycle. One help row / one key -- the
    // name renders "CYCLE PLATFORM/FORMAT/DB/PLUGIN FILTERS", shortcut is "tab".
    cmd("cycle_platform/format/db/plugin_filters", [=] {
        Screen next;
        if (currentScreen == ADVANCED_SCREEN)
            next = FORMAT_SCREEN;
        else if (currentScreen == FORMAT_SCREEN)
            next = DATABASE_SCREEN;
        else if (currentScreen == DATABASE_SCREEN)
            next = PLUGIN_SCREEN;
        else if (currentScreen == PLUGIN_SCREEN)
            next = ADVANCED_SCREEN;
        else {
            // Entering the cycle from a real screen: remember it for ESC.
            lastScreen = currentScreen;
            next = ADVANCED_SCREEN;
        }
        // Per-screen setup, then show.
        if (next == ADVANCED_SCREEN) {
            // Ensure the per-format tune counts are ready (e.g. when the index was
            // loaded from cache and the indexing-finished path never ran).
            if (filterCounts.empty()) computeFilterCounts();
            // Always open at the top level (reset any prior drill state).
            if (activeFilterOptions != nullptr)
                setFilterLevel(nullptr, drillReturnIndex);
        } else if (next == FORMAT_SCREEN) {
            // Fresh entry: clear any prior narrowing query, then size the list
            // (row 0 is [no filter], then one row per surviving group).
            formatFilterText.clear();
            rebuildFormatVisible();
        } else if (next == DATABASE_SCREEN) {
            auto const& groups = musicDatabase.databaseGroups();
            databaseList.setTotal((int)groups.size() + 1);
            if (databaseList.selected() >= (int)groups.size() + 1)
                databaseList.select(0);
        } else { // PLUGIN_SCREEN
            // Fresh entry: clear any prior narrowing query, then size the list
            // (row 0 is [no filter], then one row per surviving plugin).
            pluginFilterText.clear();
            rebuildPluginVisible();
        }
        showScreen(next);
    });
    shortcut("tab");

    // No-op for LEFT/RIGHT on the single-column filter screens (Formats /
    // Databases). Without a binding those keys fall through updateKeys' generic
    // handler and get treated as "start a search", jumping to the search screen
    // -- unexpected on a browse list. Bound (and hidden) in setupRules.
    cmd("filter_list_noop", [=] {});

    // LEFT/RIGHT on the platform-filter screen hop between the two columns,
    // landing on the entry displayed on the SAME visual row. The list is laid out
    // column-major (left column = indices [0,rows), right = [rows,n)) with the
    // right column shifted DOWN one row so "[no filter]" sits alone at the top
    // (see the advancedList render lambda) -- hence the -1/+1 in the mapping:
    // left index i renders on row i, right index j renders on row (j-rows)+1.
    // A drilled-in submenu is a single column, so both are no-ops there.
    cmd("filter_column_left", [=] {
        if (activeFilterOptions != nullptr) return;
        int n = (int)currentFilterOptions().size();
        int rows = (n + 1) / 2;
        int i = advancedList.selected();
        if (i < rows) return; // already in the left column
        int t = i - rows + 1;
        // The right column runs one row lower, so its last entry has no left
        // counterpart -- clamp to the bottom of the left column.
        if (t > rows - 1) t = rows - 1;
        advancedList.select(t);
    });

    cmd("filter_column_right", [=] {
        if (activeFilterOptions != nullptr) return;
        int n = (int)currentFilterOptions().size();
        int rows = (n + 1) / 2;
        int i = advancedList.selected();
        if (i >= rows || rows >= n) return; // already right, or no right column
        // "[no filter]" (row 0) has no right-column neighbour (that row is left
        // deliberately empty), so step to the first right-column entry.
        int t = (i == 0) ? rows : rows + i - 1;
        if (t >= n) t = n - 1;
        advancedList.select(t);
    });

    cmd("select_filter", [=] {
        int idx = advancedList.selected();
        auto const& opts = currentFilterOptions();
        if (idx >= 0 && idx < (int)opts.size()) {
            auto const& sel = opts[idx];
            // A group ("Nintendo"/"Sony"): drill into its child platforms. Users
            // return to the top level with ESC (no explicit back row).
            if (!sel.children.empty()) {
                drillReturnIndex = idx;
                drillOptions.clear();
                for (auto const& ch : sel.children)
                    drillOptions.push_back(ch);
                setFilterLevel(&drillOptions, 0);
                return;
            }
        }
        bool hasFilter = false;
        if (idx >= 0 && idx < (int)opts.size()) {
            auto const& opt = opts[idx];
            // The no-filter entry is the one with no formats (its label is
            // user-editable, so match on that rather than the name).
            hasFilter = !opt.matchedFormats.empty();
            selectedFilterName = hasFilter ? opt.name : "";
            activeFilterCount = hasFilter ? filterOptionCount(opt) : 0;
            musicDatabase.setFormatFilter(opt.matchedFormats);

            iquery->invalidate();
            // Start from an empty query so the search re-runs cleanly with the
            // new filter -- and a *small* filter pre-populates with all of its
            // songs (see MusicDatabase::search). Without this the list keeps the
            // stale pre-filter results until the user edits the search text.
            searchField.setText("");
            songList.select(0); // show the pre-populated list from the top
            searchUpdated = true;

            mainFilterField.setText(
                hasFilter ? selectedFilterName + "  (TAB to change)"
                          : "");
        }
        // Land on the search screen so the (pre-populated) results are visible
        // immediately; selecting "no filter" just returns to the main screen.
        showScreen(hasFilter ? SEARCH_SCREEN : MAIN_SCREEN);
    });

    // ENTER on the Formats screen: apply the highlighted extension as a filter.
    // Mirrors select_filter, but for the per-extension filter. Row 0 is the
    // [no filter] entry, so group g sits at list index g+1.
    cmd("select_format", [=] {
        int idx = formatList.selected();
        auto const& groups = musicDatabase.extensionGroups();
        // idx-1 is a position in the (possibly narrowed) visible list; map it
        // back to the real extensionGroups() index before applying the filter.
        bool hasFilter = (idx > 0 && (idx - 1) < (int)formatVisibleGroups.size());
        if (hasFilter) {
            int gid = formatVisibleGroups[idx - 1];
            auto const& grp = groups[gid];
            musicDatabase.setExtensionFilter(gid);
            // Prompt / persistent-label token: the uppercased extension (concise
            // and unambiguous), e.g. ".XM".
            std::string token = "." + grp.ext;
            for (auto& ch : token) ch = (char)toupper((unsigned char)ch);
            selectedFilterName = token;
            activeFilterCount = grp.count;
            mainFilterField.setText(token + "  (TAB to change)");
        } else {
            // Row 0 ([no filter]) or an out-of-range index: clear the filter.
            musicDatabase.setExtensionFilter(-1);
            selectedFilterName = "";
            activeFilterCount = 0;
            mainFilterField.setText("");
        }
        iquery->invalidate();
        // Empty query so the search re-runs cleanly with the new filter (a small
        // filter pre-populates all its songs; see MusicDatabase::search).
        searchField.setText("");
        songList.select(0);
        searchUpdated = true;
        showScreen(hasFilter ? SEARCH_SCREEN : MAIN_SCREEN);
    });

    // ENTER on the Databases screen: restrict search to the highlighted source
    // collection. Same shape as select_format; row 0 is [no filter].
    cmd("select_database", [=] {
        int idx = databaseList.selected();
        auto const& groups = musicDatabase.databaseGroups();
        bool hasFilter = (idx > 0 && (idx - 1) < (int)groups.size());
        if (hasFilter) {
            auto const& grp = groups[idx - 1];
            musicDatabase.setDatabaseFilter(grp.rowid);
            selectedFilterName = grp.name; // e.g. "HVSC"
            activeFilterCount = grp.count;
            mainFilterField.setText(grp.name + "  (TAB to change)");
        } else {
            musicDatabase.setDatabaseFilter(-1);
            selectedFilterName = "";
            activeFilterCount = 0;
            mainFilterField.setText("");
        }
        iquery->invalidate();
        searchField.setText("");
        songList.select(0);
        searchUpdated = true;
        showScreen(hasFilter ? SEARCH_SCREEN : MAIN_SCREEN);
    });

    // ENTER on the Plugins screen: restrict search to the songs the highlighted
    // plugin claims by extension. Same shape as select_format; row 0 is [no
    // filter], so the narrowed plugin at visible position idx-1 sits at list
    // index idx -- map through pluginVisibleGroups to the real gid
    // setPluginFilter expects (pluginGroups() order).
    cmd("select_plugin", [=] {
        int idx = pluginList.selected();
        auto const& groups = musicDatabase.pluginGroups();
        bool hasFilter = (idx > 0 && (idx - 1) < (int)pluginVisibleGroups.size());
        if (hasFilter) {
            int gid = pluginVisibleGroups[idx - 1];
            auto const& grp = groups[gid];
            musicDatabase.setPluginFilter(gid);
            selectedFilterName = grp.name; // the plugin's full name()
            activeFilterCount = grp.count;
            mainFilterField.setText(grp.name + "  (TAB to change)");
        } else {
            musicDatabase.setPluginFilter(-1);
            selectedFilterName = "";
            activeFilterCount = 0;
            mainFilterField.setText("");
        }
        iquery->invalidate();
        searchField.setText("");
        songList.select(0);
        searchUpdated = true;
        showScreen(hasFilter ? SEARCH_SCREEN : MAIN_SCREEN);
    });

    cmd("play_song", [=] {
        if (haveSelection()) {
            auto song = getSelectedSong();
            // A podcast SHOW row: drill into its episodes instead of playing.
            if (utils::startsWith(song.path, "podcastshow::")) {
                musicDatabase.setPodcastShow(std::stoi(song.path.substr(13)));
                songList.select(0);
                searchUpdated = true;
                return;
            }
            // An Other-platforms FAMILY row (e.g. "Virtual Platforms"): drill into
            // its children (a submenu of sub-platforms), not into songs.
            if (utils::startsWith(song.path, "othergroup::")) {
                musicDatabase.setOtherParent(std::stoi(song.path.substr(12)));
                songList.select(0);
                searchUpdated = true;
                return;
            }
            // An Other-platforms GROUP row: drill into its songs instead.
            if (utils::startsWith(song.path, "otherplatform::")) {
                musicDatabase.setOtherPlatform(std::stoi(song.path.substr(15)));
                songList.select(0);
                searchUpdated = true;
                return;
            }
            // Picking a song by hand leaves the shuffle -- the song now playing
            // is no longer one of its entries.
            shuffleList.clear();
            player.playSong(song);
            showScreen(MAIN_SCREEN);
        }
    });
    // Plain "enter": addKey() would append " [search]", but a song can only be
    // picked on the search screen anyway, so the suffix says nothing.
    shortcut("enter");

    // Displayed as one row "NEXT / PREV SUBTUNE   RIGHT / LEFT": this is the
    // RIGHT key (next subtune); prev_subtune below is the LEFT key and is hidden
    // from the list so the pair collapses to a single entry. Pre-seed the
    // shortcut so addKey() does not overwrite it with the plain "right".
    cmd("next_/_prev_subtune", [=] {
        if (currentInfo.numtunes == 0)
            player.seek(-1, player.getPosition() + 10);
        else if (currentTune < currentInfo.numtunes - 1)
            player.seek(currentTune + 1);
    });
    commands.back().shortcut = "right / left";

    cmd("prev_subtune", [=] {
        if (currentInfo.numtunes == 0)
            player.seek(-1, player.getPosition() - 10);
        else if (currentTune > 0)
            player.seek(currentTune - 1);
    });

    cmd("pause_/_resume_playback", [=] {
        // Nothing loaded -> SPACE is a no-op (no pause state, no mute overlay).
        if (!player.isPlaying()) return;
        auto isPaused = player.isPaused();
        player.pause(!isPaused);
        if (!isPaused) {
            Tween::make()
                .sine()
                .repeating()
                .to(timeField.add, 1.0)
                .seconds(0.5);
        } else
            Tween::make().to(timeField.add, 0.0).seconds(0.5);
    });
    // Plain "space bar": addKey() would append " [main]" from the first of its
    // two bindings, but SPACE pauses on the search screen too.
    shortcut("space bar");

    // Displayed as one row "VOLUME UP / DOWN   + / -": this is the '+' key (raise
    // volume); volume_down below is the '-' key and is hidden from the list so
    // the pair collapses to a single entry. Pre-seed the shortcut so addKey()
    // does not overwrite it with the plain "+".
    cmd("volume_up_/_down", [=] {
        player.setVolume(player.getVolume() + 0.1);
        showVolume = 30;
    });
    commands.back().shortcut = "+ / -";
    cmd("volume_down", [=] {
        player.setVolume(player.getVolume() - 0.1);
        showVolume = 30;
    }); 

    cmd("Spectrum_Analyzer_Mode", [=] {
        // Cycle Auto -> Mono -> Stereo so the user keeps manual control while
        // still being able to return to automatic content detection.
        if (autoStereoDetect) { 
            autoStereoDetect = false;
            stereoSpectrum = false;
            toast("Spectrum: Mono", NORMAL);
        } else if (!stereoSpectrum) {
            stereoSpectrum = true;
            toast("Spectrum: Stereo", NORMAL);
        } else {
            autoStereoDetect = true;
            stereoDiffAccum = 0;
            stereoSumAccum = 0; 
            stereoDetectFrames = 0;
            toast("Spectrum: Auto", NORMAL);
        }
        musicBarsWidth = stereoSpectrum ? spectrumWidth : spectrumWidth * 2;
        musicBars.setup(musicBarsWidth, spectrumHeight);
    });
    shortcut("ctrl+m(ode)");

    cmd("next_song_artwork", [=] { transitions.next(); });
    shortcut("ctrl+a(rtwork)");

    cmd("next_scroll_font", [=] {
        auto name = scrollEffect.nextFont();
        // Drop the .otf (or any) extension from the on-screen toast.
        auto dot = name.find_last_of('.');
        if (dot != std::string::npos) name = name.substr(0, dot);
        if (!name.empty()) toast("Font: " + name, NORMAL);
    });
    shortcut("ctrl+n(ext)");

    cmd("local_file_playback", [=] {
        std::string path = open_file_dialog();
        if (path != "") {
            SongInfo si;
            si.path = path;
            player.playSong(si);
            showScreen(MAIN_SCREEN);
        }
    });
    shortcut("ctrl+o(pen) or drag and drop");


    cmd("download_playing_song", [=] {
        auto target = Environment::getHomeDir() / "Downloads";
        utils::create_directory(target);
        
        auto files = player.getSongFiles();
        if (files.size() == 0) return;
        for (auto const& fromFile : files) {
            utils::path from = fromFile.getName();
            std::string fileName;
            std::string title = currentInfo.title;
            std::string composer = currentInfo.composer;
            if (composer == "" || composer == "?") composer = "Unknown";
            if (title == "") title = currentInfo.game;
            auto ext = utils::path_extension(from.string());
            if (title == "" || utils::endsWith(ext, "lib"))
                fileName = from.string();
            else
                fileName = utils::format("%s - %s.%s", composer, title, ext);
            auto to = target / fileName;
            LOGD("Downloading to '%s'", to.string());
            if (!utils::copy(from, to)) {
                to = target / from.filename();
                utils::copy(from, to);
            }
        }
        toast("Downloaded file");
    });
    shortcut("ctrl+d(ownload)");

    // Toggles: favors the playing song, or un-favors it when it already is one
    // (the heart icon shows which way it went).
    cmd("favor/unfavor_playing_song", [=] {
        auto song = dbInfo;
        song.starttune = currentTune;
        if (isFavorite) {
            musicDatabase.removeFromPlaylist(currentPlaylistName, song);
        } else {
            musicDatabase.addToPlaylist(currentPlaylistName, song);
        }
        isFavorite = !isFavorite;
        uint32_t alpha = isFavorite ? 0xff : 0x00;
        Tween::make()
            .to(favIcon.color, Color(favColor | (alpha << 24)))
            .seconds(0.25);
    });
    // Keep a suffix (addKey() would have appended "[main]"): CTRL+F appears on
    // two rows, and the suffix is what says they are not a duplicate. "[playback]"
    // rather than "[main]" -- it names what the key acts on, not the screen's
    // internal name.
    shortcut("ctrl+f(avor) [playback]");

    // Search-screen counterpart of the toggle above.
    cmd("favor/unfavor_highlighted_search_result", [=] {
        if (!haveSelection()) return;
        auto song = getSelectedSong();
        auto& favorites = musicDatabase.getPlaylist(currentPlaylistName);
        auto it = std::find_if(
            favorites.begin(), favorites.end(),
            [&](SongInfo const& s) { return s.path == song.path; });
        if (it == favorites.end()) {
            musicDatabase.addToPlaylist(currentPlaylistName, song);
            toast("FAVORED!");
            return;
        }
        // Remove the STORED entry, not the freshly looked-up song: a favorite
        // added while playing carries the subtune it was added at, which
        // removeFromPlaylist() matches on -- the search row's own starttune
        // would not line up. Copy it first; the vector is erased underneath.
        SongInfo stored = *it;
        musicDatabase.removeFromPlaylist(currentPlaylistName, stored);
        toast("UNFAVORED!");
    });
    shortcut("ctrl+f(avor) [search]");

    // Snapshot the current favorites into a freshly named playlist, then empty
    // the favorites -- a "move", so the heart list clears as the new list fills.
    // CTRL+L opens a naming dialog (ENTER commits, ESC cancels). The name is
    // validated -- trimmed, '/' and '\' folded to '-', non-empty, no
    // case-insensitive collision with an existing list -- before the playlist is
    // created; a rejected name keeps the dialog open with an error toast.
    cmd("move_favorites_to_a_new_playlist", [=] {
        auto favorites = musicDatabase.getPlaylist(currentPlaylistName);
        if (favorites.empty()) {
            toast("NO FAVORITES TO MOVE!", ERROR);
            return;
        }
        auto dialog = std::make_shared<Dialog>(grappix::screenptr, font,
                                               "NAME NEW PLAYLIST:", 1.5F);
        dialog->on_ok([=](std::string const& raw) -> bool {
            std::string name = utils::lrstrip(raw);
            std::replace(name.begin(), name.end(), '/', '-');
            std::replace(name.begin(), name.end(), '\\', '-');
            if (name.empty()) {
                toast("NAME CAN'T BE EMPTY!", ERROR);
                return false;
            }
            for (auto const& existing : musicDatabase.playlistNames()) {
                if (utils::toLower(existing) == utils::toLower(name)) {
                    toast("PLAYLIST ALREADY EXISTS!", ERROR);
                    return false;
                }
            }
            int n = (int)favorites.size();
            musicDatabase.createPlaylist(name, favorites);
            // Empties favorites + drops the heart icon; its "CLEARED N" toast is
            // immediately superseded by the "MOVED" one below.
            clearFavorites();
            toast(utils::format("MOVED %d FAVORITES TO %s", n, name));
            return true;
        });
        overlay.add(dialog);
        currentDialog = dialog;
    });
    shortcut("ctrl+l(ist)");

    // Destructive and irreversible (the list is rewritten to disk immediately),
    // hence the deliberately awkward CTRL+SHIFT+C -- it must not sit next to
    // CTRL+C. The shortcut is pre-seeded below because addKey() would render the
    // modifiers as "shift+ctrl+c".
    cmd("clear_favorites_list", [=] {
        auto& favorites = musicDatabase.getPlaylist(currentPlaylistName);
        if (favorites.empty()) {
            toast("NO FAVORITES TO CLEAR!", ERROR);
            return;
        }
        // Do NOT wipe on the shortcut alone -- this rewrites the list to disk at
        // once and there is no undo. Arm the confirmation instead; updateKeys()
        // resolves it on the next key press. STICKY so the prompt does not fade
        // out from under the decision.
        pendingFavoritesClear = true;
        toast(utils::format("Y TO CLEAR %d FAVORITES!", (int)favorites.size()),
              STICKY_ALERT);
    });
    // "shift+ctrl+" (not "ctrl+shift+") to read consistently with SHIFT+LEFT /
    // SHIFT+RIGHT. The physical combo is unchanged; this is the label only.
    shortcut("shift+ctrl+c(lear)");

    cmd("clear_filter", [=] {
        filter = "";
        searchUpdated = true;
    });

    // Toggle: with no filter active, adopt the selected song's collection
    // prefix; otherwise clear the filter. (clear_filter above is separate and
    // stays -- it is the search-screen BACKSPACE binding.)
    //
    // UNBOUND AND HIDDEN ON PURPOSE -- KEEP. This has no key (it was CTRL+I) and
    // so never appears in the help menu, because as a user-facing feature it was
    // confusing: it duplicated what the CTRL+B database shuffle already says, and
    // silently did nothing off the search screen.
    //
    // It is kept because it is the working core of a planned feature: a screen
    // listing every database/collection (like the TAB platform screen does for
    // platforms), each entry offering "restrict search to this database". That
    // screen should REUSE this path instead of reinventing it. The pieces:
    //   - a song's source archive is the "<collection>::" prefix of its path,
    //     which is what the split below extracts;
    //   - MusicDatabase::setFilter(name) resolves that name to a collection
    //     ROWID and installs the title-index predicate that does the actual
    //     restricting (searches then only return that collection);
    //   - the `filter` member is applied, and filterField updated, in
    //     updateKeys() where searchUpdated is handled;
    //   - cmd("clear_filter") (BACKSPACE on an empty search) already clears it.
    // The new screen only needs to call setFilter() with the chosen collection;
    // this toggle can then be dropped, or re-bound, whichever fits.
    cmd("set_/_clear_collection_filter", [=] {
        if (filter.empty()) {
            auto const& song = getSelectedSong();
            auto p = utils::split(song.path, "::");
            if (p.size() < 2) return;
            filter = p[0];
        } else {
            filter = "";
        }
        searchUpdated = true;
    });


    cmd("clear_search", [=] {
        // Inside a drilled-in podcast show: ESC pops back to the show list
        // rather than leaving the search screen.
        if (musicDatabase.podcastShow() >= 0 && searchField.getText() == "") {
            musicDatabase.setPodcastShow(-1);
            songList.select(0);
            searchUpdated = true;
            return;
        }
        // Inside a drilled-in Other-platform: ESC pops back to the platform list
        // (which is the family submenu if we drilled in via one).
        if (musicDatabase.otherPlatform() >= 0 && searchField.getText() == "") {
            musicDatabase.setOtherPlatform(-1);
            songList.select(0);
            searchUpdated = true;
            return;
        }
        // Inside a family submenu (e.g. Virtual Platforms): ESC pops to the top
        // Other list. Steps back one level, mirroring the platform pop above.
        if (musicDatabase.otherParent() >= 0 && searchField.getText() == "") {
            musicDatabase.setOtherParent(-1);
            songList.select(0);
            searchUpdated = true;
            return;
        }
        // A filter is active (reached via a TAB-cycle screen): ESC steps back up
        // to whichever filter screen set it rather than jumping all the way out to
        // the main screen. Only an unfiltered search returns to MAIN.
        if (searchField.getText() == "" && musicDatabase.hasFormatFilter()) {
            Screen back = ADVANCED_SCREEN;
            if (musicDatabase.pluginFilter() >= 0)
                back = PLUGIN_SCREEN;
            else if (musicDatabase.databaseFilter() >= 0)
                back = DATABASE_SCREEN;
            else if (musicDatabase.extensionFilter() >= 0)
                back = FORMAT_SCREEN;
            showScreen(back);
            return;
        }
        if (searchField.getText() == "")
            showScreen(MAIN_SCREEN);
        else {
            searchField.setText("");
            searchUpdated = true;
        }
    });

    cmd("clear_command", [=] {
        LOGD("CMD %s", commandField.getText());
        if (commandField.getText() == "")
            showScreen(MAIN_SCREEN);
        else {
            commandField.setText("");
            clearCommand();
            commandList.setTotal(matchingCommands.size());
        }
    });

    cmd("layout_screen", [=] { layoutScreen(); });

    // The shuffle set. Registration order here IS the help-menu order, and each
    // name doubles as its help label ('_' renders as a space), so these read as
    // what they actually do: every one but "all songs" / "search results" seeds
    // itself from the PLAYING song and varies only which of its fields is kept
    // as a filter (see shuffleSongs).
    cmd("shuffle_all_songs_randomly", [=] {
        toast("Shuffling all songs!");
        shuffleSongs(Shuffle::All, 100);
    });
    shortcut("ctrl+r(andom song)");

    cmd("shuffle_search_results_randomly", [=] {
        toast("Shuffling search results!");
        std::vector<SongInfo> target;
        for (int i = 0; i < iquery->numHits(); i++) {
            auto res = iquery->getResult(i);
            LOGD("%s", res);
            auto parts = utils::split(res, "\t");

            int f = atoi(parts[3]) & 0xff;
            if (f == PLAYLIST) continue;

            SongInfo song;
            song.title = parts[0];
            song.composer = parts[1];
            song.path = std::string("index::") + parts[2];
            target.push_back(song);
        }
        // Shuffle up front rather than random-inserting each song as it is added:
        // same resulting order, but it goes through playSongs() so the shuffle is
        // recorded and CTRL+LEFT can step back through it like the others.
        std::shuffle(target.begin(), target.end(), shuffleRng());
        playSongs(target);
    });
    shortcut("ctrl+s(earch results randomly)");

    cmd("shuffle_your_favorites", [=]() {
        // getPlaylist() hands back a shared static empty vector when the playlist
        // does not exist, so this covers both "never favourited anything" and
        // "emptied it". Without the guard, shuffling nothing would still clear
        // the play queue and stop whatever is playing.
        if (musicDatabase.getPlaylist(currentPlaylistName).empty()) {
            toast("NO FAVORITES TO SHUFFLE!", ERROR);
            return;
        }
        toast("Shuffling your favorites!");
        shuffleFavorites();
    });
    shortcut("ctrl+p(lay favorites)");

    cmd("shuffle_playing_song's_composer's_songs", [=] {
        // Without this, an unknown composer silently degrades into a random
        // shuffle: getSongs() only adds "AND composer=?" when the seed's composer
        // is non-empty, so a blank one drops the filter entirely and returns 1000
        // random songs. The "?" / "Unknown" markers are just as useless -- they
        // are a bucket of unrelated authors, not one person.
        if (isUnknownComposer(shuffleSeed().composer)) {
            toast("NO COMPOSER TO SHUFFLE!", ERROR);
            return;
        }
        toast("Shuffling this composer!");
        shuffleSongs(Shuffle::Composer, 1000);
    });
    shortcut("ctrl+c(omposer)");

    // "Database" here is the source archive a song was onboarded from (modland,
    // HVSC, Zophar, ...) -- stored as the "<collection>::" prefix on every path,
    // which is what Shuffle::Collection filters on.
    cmd("shuffle_playing_song's_database's_songs", [=] {
        toast("Shuffling this database!");
        shuffleSongs(Shuffle::Collection, 100);
    });
    shortcut("ctrl+b(ase)");

    cmd("shuffle_playing_song's_format/extension", [=] {
        toast("Shuffling this format!");
        shuffleSongs(Shuffle::Format, 100);
    });
    shortcut("ctrl+e(xtension)");

    // Displayed as one row "NEXT / PREV SHUFFLE SONG   SHIFT+RIGHT / SHIFT+LEFT":
    // this is SHIFT+RIGHT (next); prev_shuffle_song below is SHIFT+LEFT and is
    // hidden from the list so the pair collapses to a single entry. Pre-seed the
    // shortcut so addKey() does not overwrite it with the plain "shift+right".
    cmd("next_/_prev_shuffle_song", [=] {
        showScreen(MAIN_SCREEN);
        player.nextSong();
    });
    commands.back().shortcut = "shift+right / shift+left";

    // Step back through the current shuffle. Only shuffles keep the list of what
    // they queued, so outside one there is nothing to step back into.
    cmd("prev_shuffle_song", [=] {
        if (shuffleList.empty()) {
            toast("Not shuffling");
            return;
        }
        int index = currentShuffleIndex();
        if (index <= 0) {
            toast("First shuffled song");
            return;
        }
        playShuffleFrom(index - 1);
    });

    cmd("close_dialog", [=] {
        if (currentDialog) currentDialog->remove();
        currentDialog = nullptr;
    });

    // Listed last, after a group divider (see groupBreaks in clearCommand).
    cmd("this_help_menu", [=] {
        if (currentScreen != COMMAND_SCREEN) {
            lastScreen = currentScreen;
            showScreen(COMMAND_SCREEN);
        } else
            showScreen(lastScreen);
    });
}

} // namespace chipmachine

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

    cmd("show_main", [=] { showScreen(MAIN_SCREEN); });

    cmd("show_search", [=]() {
        if (currentScreen != SEARCH_SCREEN) {
            showScreen(SEARCH_SCREEN);
            songList.onKey(lastKey);
        } else {
            showScreen(SEARCH_SCREEN);
        }
        searchUpdated = true;
    });

    cmd("show_command", [=] {
        if (currentScreen != COMMAND_SCREEN) lastScreen = currentScreen;
        showScreen(COMMAND_SCREEN);
    });

    cmd("show_advanced", [=] {
        if (currentScreen != ADVANCED_SCREEN) lastScreen = currentScreen;
        // Ensure the per-format tune counts are ready (e.g. when the index was
        // loaded from cache and the indexing-finished path never ran).
        if (filterCounts.empty()) computeFilterCounts();
        showScreen(ADVANCED_SCREEN);
    });

    cmd("select_filter", [=] {
        int idx = advancedList.selected();
        bool hasFilter = false;
        if (idx >= 0 && idx < filterOptions.size()) {
            auto const& opt = filterOptions[idx];
            // The no-filter entry is the one with no formats (its label is
            // user-editable, so match on that rather than the name).
            hasFilter = !opt.matchedFormats.empty();
            selectedFilterName = hasFilter ? opt.name : "";
            activeFilterCount = (hasFilter && idx < (int)filterCounts.size())
                                    ? filterCounts[idx]
                                    : 0;
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
                hasFilter ? selectedFilterName + "  (F9 to change)"
                          : "");
        }
        // Land on the search screen so the (pre-populated) results are visible
        // immediately; selecting "no filter" just returns to the main screen.
        showScreen(hasFilter ? SEARCH_SCREEN : MAIN_SCREEN);
    });

    cmd("close_advanced", [=] {
        showScreen(lastScreen);
    });

    cmd("toggle_command", [=] {
        if (currentScreen != COMMAND_SCREEN) {
            lastScreen = currentScreen;
            showScreen(COMMAND_SCREEN);
        } else
            showScreen(lastScreen);
    });

    cmd("local_file_playback", [=] {
        std::string path = open_file_dialog();
        if (path != "") {
            SongInfo si;
            si.path = path;
            player.playSong(si);
            showScreen(MAIN_SCREEN);
        }
    });

    cmd("download_current", [=] {
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

    cmd("play_pause", [=] {
        // Nothing loaded -> F5 is a no-op (no pause state, no mute overlay).
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

    cmd("enque_song", [=] {
        if (haveSelection()) {
            auto song = getSelectedSong();
            // On a podcast SHOW row, drill into its episodes (nothing to enque).
            if (utils::startsWith(song.path, "podcastshow::")) {
                musicDatabase.setPodcastShow(std::stoi(song.path.substr(13)));
                songList.select(0);
                searchUpdated = true;
                return;
            }
            // On an Other-platforms GROUP row, drill into its songs.
            if (utils::startsWith(song.path, "otherplatform::")) {
                musicDatabase.setOtherPlatform(std::stoi(song.path.substr(15)));
                songList.select(0);
                searchUpdated = true;
                return;
            }
            player.addSong(song);
            songList.select(songList.selected() + 1);
        }
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

    cmd("next_screenshot", [=] { transitions.next(); });

    cmd("next_scroll_font", [=] {
        auto name = scrollEffect.nextFont();
        // Drop the .otf (or any) extension from the on-screen toast.
        auto dot = name.find_last_of('.');
        if (dot != std::string::npos) name = name.substr(0, dot);
        if (!name.empty()) toast("Font: " + name, NORMAL);
    });

    cmd("add_current_favorite", [=] {
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

    cmd("add_list_favorite", [=] {
        if (haveSelection())
            musicDatabase.addToPlaylist(currentPlaylistName, getSelectedSong());
    });

    cmd("clear_filter", [=] {
        filter = "";
        searchUpdated = true;
    });

    cmd("set_collection_filter", [=] {
        auto const& song = getSelectedSong();
        auto p = utils::split(song.path, "::");
        if (p.size() < 2) return;
        filter = p[0];
        searchUpdated = true;
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
            // An Other-platforms GROUP row: drill into its songs instead.
            if (utils::startsWith(song.path, "otherplatform::")) {
                musicDatabase.setOtherPlatform(std::stoi(song.path.substr(15)));
                songList.select(0);
                searchUpdated = true;
                return;
            }
            player.playSong(song);
            showScreen(MAIN_SCREEN);
        }
    });

    cmd("next_composer", [=] {
        std::string composer;
        int index = songList.selected();
        while (index < songList.size()) {
            auto res = iquery->getResult(index);
            auto parts = utils::split(res, "\t");
            if (composer == "") composer = parts[1];
            if (parts[1] != composer) break;
            index++;
        }
        songList.select(index);
    });

    cmd("next_song", [=] {
        showScreen(MAIN_SCREEN);
        player.nextSong();
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
        // Inside a drilled-in Other-platform: ESC pops back to the platform list.
        if (musicDatabase.otherPlatform() >= 0 && searchField.getText() == "") {
            musicDatabase.setOtherPlatform(-1);
            songList.select(0);
            searchUpdated = true;
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

    cmd("execute_selected_command", [=] {
        int i = commandList.selected();
        if (matchingCommands.size() == 0) return;
        commandList.select(-1);
        showScreen(lastScreen);
        auto it =
            std::find(commands.begin(), commands.end(), *matchingCommands[i]);
        if (it != commands.end()) it->fn();
    });

    cmd("next_subtune", [=] {
        if (currentInfo.numtunes == 0)
            player.seek(-1, player.getPosition() + 10);
        else if (currentTune < currentInfo.numtunes - 1)
            player.seek(currentTune + 1);
    });

    cmd("prev_subtune", [=] {
        if (currentInfo.numtunes == 0)
            player.seek(-1, player.getPosition() - 10);
        else if (currentTune > 0)
            player.seek(currentTune - 1);
    });

    cmd("clear_songs", [=] {
        player.clearSongs();
        toast("Playlist cleared");
    });

    cmd("volume_up", [=] {
        player.setVolume(player.getVolume() + 0.1);
        showVolume = 30;
    });

    cmd("volume_down", [=] {
        player.setVolume(player.getVolume() - 0.1);
        showVolume = 30;
    });

    cmd("layout_screen", [=] { layoutScreen(); });

    cmd("quit", [=] { grappix::screen.close(); });

    cmd("random_shuffle", [=] {
        toast("Random shuffle!");
        shuffleSongs(Shuffle::All, 100);
    });

    cmd("composer_shuffle", [=] {
        toast("Composer shuffle!");
        shuffleSongs(Shuffle::Composer, 1000);
    });

    cmd("format_shuffle", [=] {
        toast("Format shuffle!");
        shuffleSongs(Shuffle::Format, 100);
    });

    cmd("collection_shuffle", [=] {
        toast("Collection shuffle!");
        shuffleSongs(Shuffle::Collection, 100);
    });

    cmd("favorite_shuffle", [=]() {
        toast("Favorites shuffle!");
        shuffleFavorites();
    });

    cmd("result_shuffle", [=] {
        toast("Result shuffle!");
        player.clearSongs();
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
            player.addSong(song, true);
        }
        showScreen(MAIN_SCREEN);
        player.nextSong();
    });

    cmd("close_dialog", [=] {
        if (currentDialog) currentDialog->remove();
        currentDialog = nullptr;
    });
}

} // namespace chipmachine

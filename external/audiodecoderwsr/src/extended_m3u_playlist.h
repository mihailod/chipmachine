// M3U playlist file parser, with support for subtrack information

// Game_Music_Emu $vers

/* Sorry about poor english.
This source is modified in 2014-08 by another auther.
Because many codes is changed, please DON'T contact original auther about this source code.
Also, please DON'T replace this source code with Game_Music_Emu library's M3u_Playlist.
*/


#ifndef EXTENDED_M3U_PLAYLIST_H
#define EXTENDED_M3U_PLAYLIST_H

#include <vector>

using namespace std;

// NEZPlug extended m3u playlist file parser, with support for subtrack information
class extended_m3u_playlist
{
public:
  // Load playlist data
  int load(const void* p_file, int filesize, bool utf8_enc = false);

  // Line number of first parse error, 0 if no error. Any lines with parse
  // errors are ignored.
  int first_error() const { return first_error_; }

  // All string pointers point to valid string, or "" if not available
  struct info_t
  {
    const char* title;
    const char* artist;
    const char* albumartist;
    const char* date;
    const char* genre;
    const char* composer;
    const char* sequencer;
    const char* engineer;
    const char* ripping;
    const char* tagging;
    const char* copyright;
    const char* comment;
    const char* system;
  };
  info_t const& info() const { return info_; }

  struct entry_t
  {
    const char* file; // filename without stupid ::TYPE suffix
    const char* type; // if filename has ::TYPE suffix, this is "TYPE", otherwise ""
    const char* name;
    bool decimal_track; // true if track was specified in decimal
    // integers are -1 if not present
    int track;
    int length; // milliseconds
    int intro;
    int loop;
    int fade;
    int repeat; // count
  };
  entry_t const& operator[](int i) const { return entries[i]; }
  int size() const { return static_cast<int>(entries.size()); }

  //void clear();
  bool get_utf8_flag() const { return utf8_flag; }

private:
  vector<entry_t> entries;
  vector<char> data;
  unsigned int datasize;
  bool utf8_flag;
  int first_error_;
  struct info_t info_;

  int parse();
  void clear();
};

inline void extended_m3u_playlist::clear()
{
  utf8_flag = false;
  first_error_ = 0;
  info_.title = "";
  info_.artist = "";
  info_.albumartist = "";
  info_.date = "";
  info_.genre = "";
  info_.composer = "";
  info_.sequencer = "";
  info_.engineer = "";
  info_.ripping = "";
  info_.tagging = "";
  info_.copyright = "";
  info_.system = "";
  entries.clear();
  data.clear();
}

#endif

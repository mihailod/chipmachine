/*
 *  Copyright (C) 2014-2021 Arne Morten Kvarving
 *  Copyright (C) 2016-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "extended_m3u_playlist.h"

#include <kodi/addon-instance/AudioDecoder.h>
#include <kodi/tools/DllHelper.h>

extern "C"
{
#include "in_wsr/wsr_player.h"

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
}

class ATTR_DLL_LOCAL CWSRCodec : public kodi::addon::CInstanceAudioDecoder,
                                 private kodi::tools::CDllHelper
{
public:
  CWSRCodec(const kodi::addon::IInstanceInfo& instance);
  virtual ~CWSRCodec() = default;

  bool Init(const std::string& filename,
            unsigned int filecache,
            int& channels,
            int& samplerate,
            int& bitspersample,
            int64_t& totaltime,
            int& bitrate,
            AudioEngineDataFormat& format,
            std::vector<AudioEngineChannel>& channellist) override;
  int ReadPCM(uint8_t* buffer, size_t size, size_t& actualsize) override;
  int64_t Seek(int64_t time) override;
  bool ReadTag(const std::string& filename, kodi::addon::AudioDecoderInfoTag& tag) override;
  int TrackCount(const std::string& fileName) override;

private:
  static unsigned int m_usedLib;
  WSRPlayerApi* (*WSRPlayerSetUp)(void);
  WSRPlayerApi* m_wsrPlayerApi{nullptr};

  bool Open(const std::string& path);
  inline uint64_t time_to_samples(double p_time, uint32_t p_sample_rate)
  {
    return (uint64_t)floor((double)p_sample_rate * p_time + 0.5);
  }

  //buffer
  std::vector<uint8_t> m_file_buf; // in-memory copy of input file
  unsigned int m_file_buf_len; // buffer size for memory copy of input file
  std::vector<uint8_t> m_decode_buf; // decode buffer
  unsigned int m_decode_buf_len; // buffer size for decode buffer

  // about input file.
  std::string m_filePath;
  unsigned int m_file_len{0}; // input file size

  //song time
  int m_song_len;
  int m_fade_len;
  int m_default_song_len;
  int m_default_fade_len;
  int m_default_loop_count;
  uint64_t m_song_sample;
  uint64_t m_fade_sample;
  bool m_set_len;

  //playback
  int m_index_num;
  unsigned int m_first_subsong;
  unsigned int m_total_subsong; // max of subsong
  unsigned int m_sample_rate;
  static const unsigned int m_channels;
  static const unsigned int m_bps;
  double m_volume;
  bool m_play_infinitely;
  static const unsigned int m_update_cpuclock; // cpu cycle
  unsigned int m_update_sample; // buffer duration [sample]
  unsigned int m_update_done;
  uint64_t m_played_sample{0}; //played time [sample]

  //detect silence
  bool m_detect_silence{false};
  int m_silence_len;
  uint64_t m_silence_sample;
  static const int m_silence_level;
  uint64_t m_detected_silence{0};

  //misc.
  bool m_display_track_num;
  unsigned int m_decode_flags;

  //extended_m3u_playlist class
  extended_m3u_playlist m_exm3u;
  extended_m3u_playlist::info_t m_exm3u_album_info;
  bool m_exm3u_load{false};
  bool m_exm3u_ignore{false};
  int m_exm3u_line{0};
  bool m_exm3u_utf8_enc{false};
  bool m_exm3uSingleScan{false};
};

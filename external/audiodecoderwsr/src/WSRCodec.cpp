/*
 *  Copyright (C) 2014-2021 Arne Morten Kvarving
 *  Copyright (C) 2016-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

/*
 * Source code based upon foo_input_wsr version 0.29.
 */

#include "WSRCodec.h"

#include "Settings.h"

#include <algorithm>
#include <iostream>
#include <kodi/Filesystem.h>
#include <kodi/General.h>

unsigned int CWSRCodec::m_usedLib = 0;
static const unsigned int ws_cpuclock = 3072000; //WonderSwan's CPU Clock. 3072000 Hz.
const unsigned int CWSRCodec::m_update_cpuclock = 40704; //WonderSwan's VBlank. 159*256
// const unsigned int      CWSRCodec::m_update_sample = 584;
const unsigned int CWSRCodec::m_channels = 2;
const unsigned int CWSRCodec::m_bps = 16;
const int CWSRCodec::m_silence_level = 8; //approximately equal to 72dB on 16bps
static const double ws_vblank = 75.47; //WonderSwan's VBlank. 159*256/3072000 Hz
//static const double d_update_cycle = ws_vblank;  //number of times update(decode) per 1 second

typedef signed short wsr_sample_t;

CWSRCodec::CWSRCodec(const kodi::addon::IInstanceInfo& instance) : CInstanceAudioDecoder(instance)
{
}

bool CWSRCodec::Init(const std::string& filename,
                     unsigned int filecache,
                     int& channels,
                     int& samplerate,
                     int& bitspersample,
                     int64_t& totaltime,
                     int& bitrate,
                     AudioEngineDataFormat& format,
                     std::vector<AudioEngineChannel>& channellist)
{
  if (!m_wsrPlayerApi)
  {
    m_usedLib = !m_usedLib;
    std::string source = kodi::addon::GetAddonPath(LIBRARY_PREFIX + std::string("in_wsr_") +
                                                   std::to_string(m_usedLib) + LIBRARY_SUFFIX);
    if (!LoadDll(source))
      return -1;
    if (!REGISTER_DLL_SYMBOL(WSRPlayerSetUp))
      return -1;
    m_wsrPlayerApi = WSRPlayerSetUp();
    if (!m_wsrPlayerApi)
      return -1;
  }

  int track = 0;
  const std::string toLoad = kodi::addon::CInstanceAudioDecoder::GetTrack("wsr", filename, track);

  // Correct if packed sound file with several sounds
  if (track > 0)
    --track;

  if (!Open(toLoad))
    return false;

  //load wsr file
  if (!m_wsrPlayerApi->p_Load_WSR(m_file_buf.data(), m_file_len))
  {
    kodi::Log(ADDON_LOG_ERROR, "Invalid input file (%s)", toLoad.c_str());
    return false;
  }

  //get play time information from m_exm3u if it exist
  if (m_exm3u_load == 1 && track < static_cast<unsigned int>(m_exm3u_line))
  {
    int exm3u_intro = m_exm3u[track].intro;
    int exm3u_loop = m_exm3u[track].loop;
    int exm3u_length = m_exm3u[track].length;
    int exm3u_repeat = m_exm3u[track].repeat;
    int exm3u_fade = m_exm3u[track].fade;

    if (exm3u_intro != -1 && exm3u_loop != -1)
    {
      if (exm3u_repeat > 0)
      {
        m_song_len = exm3u_intro + exm3u_loop * exm3u_repeat;
      }
      else if (exm3u_repeat == 0)
      {
        m_song_len = exm3u_intro + exm3u_loop;
        m_play_infinitely = true;
      }
      else if (exm3u_repeat == -1)
      {
        if (m_default_loop_count > 0)
          m_song_len = exm3u_intro + exm3u_loop * m_default_loop_count;
        else if (m_default_loop_count == 0)
        {
          m_song_len = exm3u_intro + exm3u_loop;
          m_play_infinitely = true;
        }
      }
    }
    else if (exm3u_length != -1)
    {
      m_song_len = exm3u_length;
    }
    else
    {
      m_song_len = m_default_song_len;
    }

    if (exm3u_fade != -1)
      m_fade_len = exm3u_fade;
    else
      m_fade_len = m_default_fade_len;

    m_index_num = m_exm3u[track].track;
  }
  else
  {
    m_song_len = m_default_song_len;
    m_fade_len = m_default_fade_len;
    m_index_num = track;
  }

  if (m_default_loop_count < 0 || m_default_loop_count == 1)
    m_play_infinitely = false;


  m_wsrPlayerApi->p_Reset_WSR(m_index_num);
  m_sample_rate = m_wsrPlayerApi->p_Set_Frequency(m_sample_rate);
  m_wsrPlayerApi->p_Set_ChannelMuting(CWSRSettings::GetInstance().GetChannelMuting());

  m_song_sample = time_to_samples(double(m_song_len) * .001, m_sample_rate);
  m_fade_sample = time_to_samples(double(m_fade_len) * .001, m_sample_rate);
  m_silence_sample = time_to_samples(m_silence_len, m_sample_rate);
  m_set_len = true;

  // allocate buffer for decode.
  //m_update_cpuclock = static_cast<unsigned int>(double(ws_cpuclock) / d_update_cycle);    //cpu cycles
  //m_update_sample = static_cast<unsigned int>((double)m_sample_rate / ws_vblank);    // buffer duration [sample]
  m_update_sample = 640;
  m_decode_buf_len = m_update_sample * m_channels * (m_bps / 8) * 2; // [byte]
  m_decode_buf.resize(m_decode_buf_len + 16);

  format = AUDIOENGINE_FMT_S16NE;
  channellist = {AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR};
  channels = m_channels;
  bitspersample = m_bps;
  samplerate = m_sample_rate;
  totaltime = m_song_len + m_fade_len;
  bitrate = m_channels * m_bps * m_sample_rate;

  return true;
}

int CWSRCodec::CWSRCodec::ReadPCM(uint8_t* buffer, size_t size, size_t& actualsize)
{
  // decide how many samples we want from wsr_player
  uint64_t update_sample = m_update_sample;
  unsigned int update_cpuclock = m_update_cpuclock;

  if ((!m_play_infinitely && m_played_sample >= m_song_sample + m_fade_sample) ||
      (m_detect_silence && m_detected_silence >= m_silence_sample))
  {
    return AUDIODECODER_READ_EOF;
  }

  if (!m_play_infinitely && (m_played_sample + update_sample) > (m_song_sample + m_fade_sample))
  {
    update_sample = (m_song_sample + m_fade_sample) - m_played_sample;
    update_cpuclock = static_cast<unsigned int>(ws_cpuclock * update_sample / m_sample_rate);
  }

  //Channel Muting
  unsigned int muting = CWSRSettings::GetInstance().GetChannelMuting();
  if (m_wsrPlayerApi->p_Get_ChannelMuting() != muting)
  {
    m_wsrPlayerApi->p_Set_ChannelMuting(muting);
  }

  // Update samples. if we cannot allocate buffer enough, return 0.
  if (!m_wsrPlayerApi->p_Update_WSR(m_decode_buf.data(), m_decode_buf_len, update_sample))
  {
    return AUDIODECODER_READ_ERROR;
  }

  // Check Silence
  if (m_detect_silence)
  {
    const wsr_sample_t* p_decode_buf = (const wsr_sample_t*)m_decode_buf.data();
    uint64_t i;
    bool silence(true);
    for (i = 0; i < update_sample; i++)
    {
      if (m_detected_silence >= m_silence_sample)
        break;

      for (int j = 0; j < m_channels; j++)
      {
        if (p_decode_buf[j] > m_silence_level || p_decode_buf[j] < m_silence_level * (-1))
          silence = false;
      }

      if (silence)
        m_detected_silence++;
      else
      {
        m_detected_silence = 0;
        silence = true;
      }
      p_decode_buf += m_channels;
    }
  }

  // Volume or Fadeout
  if ((!m_play_infinitely && m_fade_sample && m_played_sample + update_sample >= m_song_sample) ||
      m_volume != 1.00)
  {
    int16_t* p_sample_in = reinterpret_cast<int16_t*>(m_decode_buf.data());
    int16_t* p_sample = reinterpret_cast<int16_t*>(buffer);
    uint64_t i;

    actualsize = 0;
    for (i = m_played_sample; i < (m_played_sample + update_sample); i++)
    {
      if (m_play_infinitely || i < m_song_sample)
      {
        p_sample[0] = static_cast<int16_t>(p_sample_in[0] * m_volume);
        p_sample[1] = static_cast<int16_t>(p_sample_in[1] * m_volume);
      }
      else if (i < (m_song_sample + m_fade_sample))
      {
        double scale = static_cast<double>(m_song_sample + m_fade_sample - i) /
                       static_cast<double>(m_fade_sample);
        scale = scale * scale * (3 - 2 * scale); // s curve fade out
        p_sample[0] = static_cast<int16_t>(p_sample_in[0] * m_volume * scale);
        p_sample[1] = static_cast<int16_t>(p_sample_in[1] * m_volume * scale);
      }
      else
      {
        p_sample[0] = 0;
        p_sample[1] = 0;
      }
      p_sample += m_channels;
      p_sample_in += m_channels;
      actualsize += m_channels * sizeof(int16_t);
    }
  }
  else
  {
    actualsize = update_sample * m_channels * sizeof(int16_t);
    memcpy(buffer, m_decode_buf.data(), actualsize);
  }

  if (!m_play_infinitely)
    m_played_sample += update_sample;
  m_update_done = update_sample;

  return AUDIODECODER_READ_SUCCESS;
}

int64_t CWSRCodec::Seek(int64_t time)
{
  if (m_play_infinitely)
    return -1;

  // compare desired seek pos with current pos
  uint64_t seek_sample = time_to_samples(double(time) / 1000.0, m_sample_rate);
  if (seek_sample < 0 || seek_sample > m_song_sample + m_fade_sample)
  {
    kodi::Log(ADDON_LOG_ERROR, "Seek is out of range");
    return -1;
  }

  m_update_done = 0;

  if (m_played_sample > seek_sample)
  {
    m_wsrPlayerApi->p_Reset_WSR(m_index_num);
    m_played_sample = 0;
  }

  while (seek_sample - m_played_sample > m_update_sample)
  {
    if (!m_wsrPlayerApi->p_Update_WSR(m_decode_buf.data(), m_decode_buf_len, m_update_sample))
    {
      kodi::Log(ADDON_LOG_ERROR, "Seek Error !");
      return -1;
    }

    m_played_sample += m_update_sample;
  }
  uint64_t remain_sample = seek_sample - m_played_sample;
  if (remain_sample > 0)
  {
    m_wsrPlayerApi->p_Update_WSR(m_decode_buf.data(), m_decode_buf_len, remain_sample);
  }
  m_played_sample = seek_sample;
  m_detected_silence = 0;

  return time;
}

bool CWSRCodec::ReadTag(const std::string& filename, kodi::addon::AudioDecoderInfoTag& tag)
{
  if (!m_wsrPlayerApi)
  {
    std::string source =
        kodi::addon::GetAddonPath(LIBRARY_PREFIX + std::string("in_wsr_track") + LIBRARY_SUFFIX);

    if (!LoadDll(source))
      return -1;
    if (!REGISTER_DLL_SYMBOL(WSRPlayerSetUp))
      return -1;
    m_wsrPlayerApi = WSRPlayerSetUp();
    if (!m_wsrPlayerApi)
      return -1;
  }

  int track = 0;
  const std::string toLoad = kodi::addon::CInstanceAudioDecoder::GetTrack("wsr", filename, track);
  if (track == 0)
    return true;
  if (track > 0)
    --track;

  if (!Open(toLoad))
    return false;

  if (!m_set_len)
  {
    //get play time information from m_exm3u if it exist
    if (m_exm3u_load == 1 && track < static_cast<unsigned int>(m_exm3u_line))
    {
      int exm3u_intro = m_exm3u[track].intro;
      int exm3u_loop = m_exm3u[track].loop;
      int exm3u_length = m_exm3u[track].length;
      int exm3u_repeat = m_exm3u[track].repeat;
      int exm3u_fade = m_exm3u[track].fade;

      if (exm3u_intro != -1 && exm3u_loop != -1)
      {
        if (exm3u_repeat > 0)
        {
          m_song_len = exm3u_intro + exm3u_loop * exm3u_repeat;
        }
        else if (exm3u_repeat == 0)
        {
          m_song_len = exm3u_intro + exm3u_loop;
          m_play_infinitely = true;
        }
        else if (exm3u_repeat == -1)
        {
          if (m_default_loop_count > 0)
            m_song_len = exm3u_intro + exm3u_loop * m_default_loop_count;
          else if (m_default_loop_count == 0)
          {
            m_song_len = exm3u_intro + exm3u_loop;
            m_play_infinitely = true;
          }
        }
      }
      else if (exm3u_length != -1)
      {
        m_song_len = exm3u_length;
      }
      else
      {
        m_song_len = m_default_song_len;
      }

      if (exm3u_fade != -1)
        m_fade_len = exm3u_fade;
      else
        m_fade_len = m_default_fade_len;

      m_index_num = m_exm3u[track].track;
    }
    else
    {
      m_song_len = m_default_song_len;
      m_fade_len = m_default_fade_len;
      m_index_num = track;
    }

    if (m_default_loop_count < 0)
      m_play_infinitely = false;
  }

  tag.SetDuration((m_song_len + m_fade_len) / 1000);
  tag.SetChannels(m_channels);
  tag.SetBitrate(m_bps * m_channels * m_sample_rate);
  tag.SetSamplerate(m_sample_rate);

  if (m_exm3u_load)
  {
    if (m_exm3uSingleScan)
    {
      if (track < static_cast<unsigned int>(m_exm3u_line))
      {
        std::vector<std::string> list = kodi::tools::StringUtils::Split(m_exm3u[track].name, " - ");
        if (list.size() >= 4)
        {
          tag.SetTitle(list[0]);
          tag.SetArtist(list[1]);
          tag.SetAlbum(list[list.size() - 2]);
          std::vector<std::string> listEnd =
              kodi::tools::StringUtils::Split(list[list.size() - 1], " ");
          if (list.size() >= 2)
          {
            if (isdigit(listEnd[0][0]))
              tag.SetReleaseDate(listEnd[0]);
            else if (isdigit(listEnd[0][1]))
              tag.SetReleaseDate(&listEnd[0][1]);
            tag.SetAlbumArtist(listEnd[1]);
          }
        }
        else
        {
          tag.SetTitle(m_exm3u[track].name);
          if (m_exm3u_album_info.albumartist)
            tag.SetAlbumArtist(m_exm3u_album_info.albumartist);
          if (m_exm3u_album_info.artist)
            tag.SetArtist(m_exm3u_album_info.artist);
          if (m_exm3u_album_info.title)
            tag.SetAlbum(m_exm3u_album_info.title);
          if (m_exm3u_album_info.genre)
            tag.SetGenre(m_exm3u_album_info.genre);
          if (m_exm3u_album_info.comment)
            tag.SetComment(m_exm3u_album_info.comment);
          if (m_exm3u_album_info.date)
            tag.SetReleaseDate(m_exm3u_album_info.date);
        }
      }
    }
    else
    {
      if (track < static_cast<unsigned int>(m_exm3u_line))
        tag.SetTitle(m_exm3u[track].name);
      if (m_exm3u_album_info.albumartist)
        tag.SetAlbumArtist(m_exm3u_album_info.albumartist);
      if (m_exm3u_album_info.artist)
        tag.SetArtist(m_exm3u_album_info.artist);
      if (m_exm3u_album_info.title)
        tag.SetAlbum(m_exm3u_album_info.title);
      if (m_exm3u_album_info.genre)
        tag.SetGenre(m_exm3u_album_info.genre);
      if (m_exm3u_album_info.comment)
        tag.SetComment(m_exm3u_album_info.comment);
      if (m_exm3u_album_info.date)
        tag.SetReleaseDate(m_exm3u_album_info.date);
    }

    if (m_display_track_num)
    {
      tag.SetTrack(track + 1);
    }
  }
  else
  {
    if (m_display_track_num && track >= m_first_subsong)
    {
      tag.SetTrack(track + 1 - m_first_subsong);
    }
  }

  return true;
}

int CWSRCodec::TrackCount(const std::string& fileName)
{
  if (!m_wsrPlayerApi)
  {
    std::string source =
        kodi::addon::GetAddonPath(LIBRARY_PREFIX + std::string("in_wsr_track") + LIBRARY_SUFFIX);

    if (!LoadDll(source))
      return -1;
    if (!REGISTER_DLL_SYMBOL(WSRPlayerSetUp))
      return -1;
    m_wsrPlayerApi = WSRPlayerSetUp();
    if (!m_wsrPlayerApi)
      return -1;
  }

  if (!Open(fileName))
    return 0;

  if (m_exm3u_load)
    return m_exm3u_line;
  else
  {
    if (m_total_subsong <= m_first_subsong)
      return 1;
    else
      return m_total_subsong - m_first_subsong;
  }
}

bool CWSRCodec::Open(const std::string& path)
{
  // Check already loaded
  if (m_filePath == path)
    return true;

  m_filePath = path;

  kodi::vfs::CFile file;
  if (!file.OpenFile(path))
  {
    kodi::Log(ADDON_LOG_ERROR, "Failed to open (%s)", path.c_str());
    return false;
  }

  m_file_len = static_cast<unsigned int>(file.GetLength());
  if (m_file_len <= 0x20)
  {
    kodi::Log(ADDON_LOG_ERROR, "Invalid input file size (%s)", path.c_str());
    return false;
  }

  m_file_buf_len = m_file_len + 16;
  m_file_buf.resize(m_file_buf_len);
  if (file.Read(m_file_buf.data(), m_file_len) < 0)
  {
    kodi::Log(ADDON_LOG_ERROR, "Read error on input file (%s)", path.c_str());
    return false;
  }

  // check wsr's footer
  if (m_file_buf[m_file_len - 0x20] != 'W' || m_file_buf[m_file_len - 0x20 + 1] != 'S' ||
      m_file_buf[m_file_len - 0x20 + 2] != 'R' || m_file_buf[m_file_len - 0x20 + 3] != 'F')
  {
    kodi::Log(ADDON_LOG_ERROR, "Invalid input file (%s)", path.c_str());
    return false;
  }

  m_first_subsong = static_cast<unsigned int>(m_file_buf[m_file_len - 0x20 + 5]);
  m_play_infinitely = CWSRSettings::GetInstance().GetPlayInfinitely();
  m_total_subsong = CWSRSettings::GetInstance().GetDefaultSubsongMax() + 1;
  m_sample_rate = CWSRSettings::GetInstance().GetSampleRate() > 0
                      ? CWSRSettings::GetInstance().GetSampleRate()
                      : 48000;
  m_default_song_len = CWSRSettings::GetInstance().GetDefaultSongLength();
  m_default_fade_len = CWSRSettings::GetInstance().GetDefaultFadeLength();
  m_default_loop_count = CWSRSettings::GetInstance().GetDefaultLoopCount();
  m_detect_silence = CWSRSettings::GetInstance().GetDetectSilence();
  m_silence_len = CWSRSettings::GetInstance().GetSilenceLength();
  m_display_track_num = CWSRSettings::GetInstance().GetDisplayTrackNum();
  m_volume = static_cast<double>(CWSRSettings::GetInstance().GetVolume()) * .01;
  m_exm3u_ignore = CWSRSettings::GetInstance().GetIgnorePlaylist();

  //load extended m3u if it exists
  if (!m_exm3u_ignore)
  {
    int exm3u_ret = 1;
    std::vector<uint8_t> exm3u_buf;
    size_t exm3u_len;

    std::string toLoadM3U = path.substr(0, path.find_last_of('.')) + ".m3u8";
    ;
    kodi::tools::StringUtils::Replace(toLoadM3U, ".wsr", ".m3u8");
    if (kodi::vfs::FileExists(toLoadM3U))
    {
      kodi::vfs::CFile file;
      if (file.OpenFile(toLoadM3U))
      {
        exm3u_len = file.GetLength();
        exm3u_buf.resize(exm3u_len);
        file.Read(&exm3u_buf[0], exm3u_len);
        exm3u_ret = m_exm3u.load(exm3u_buf.data(), exm3u_len, true);
      }
    }

    if (exm3u_ret != 0)
    {
      toLoadM3U = path.substr(0, path.find_last_of('.')) + ".m3u";
      ;
      if (kodi::vfs::FileExists(toLoadM3U))
      {
        kodi::vfs::CFile file;
        if (file.OpenFile(toLoadM3U))
        {
          exm3u_len = file.GetLength();
          exm3u_buf.resize(exm3u_len);
          file.Read(&exm3u_buf[0], exm3u_len);
          exm3u_ret = m_exm3u.load(exm3u_buf.data(), exm3u_len, false);
        }
      }
    }

    if (exm3u_ret != 0)
    {
      // Fallback for case if only single m3u files are present and combine it
      std::vector<kodi::vfs::CDirEntry> items;
      if (kodi::vfs::GetDirectory(kodi::vfs::GetDirectoryName(path), ".m3u", items))
      {
        const std::string baseFile = kodi::vfs::GetFileName(path);
        std::string m3uComplete;
        for (const auto& item : items)
        {
          kodi::vfs::CFile file;
          if (!file.OpenFile(item.Path()))
            continue;

          std::string line;
          if (file.ReadLine(line) && kodi::tools::StringUtils::StartsWith(line, baseFile))
            m3uComplete += line + "\n";
        }

        exm3u_ret = m_exm3u.load(m3uComplete.data(), m3uComplete.length(), false);
        m_exm3uSingleScan = true;
      }
    }

    if (exm3u_ret == 0)
    {
      kodi::Log(ADDON_LOG_DEBUG, "load m3u (%s)", toLoadM3U.c_str());
      m_exm3u_load = true;
      m_exm3u_utf8_enc = m_exm3u.get_utf8_flag();
      m_exm3u_album_info = m_exm3u.info();
      m_exm3u_line = m_exm3u.size();
      if (m_exm3u_line < 0)
        m_exm3u_line = 0;
    }
    else if (exm3u_ret == -1)
    {
      m_exm3u_load = false;
      kodi::Log(ADDON_LOG_ERROR, "m3u load error (%s)!", toLoadM3U.c_str());
    }
  }

  return true;
}

//------------------------------------------------------------------------------

class ATTR_DLL_LOCAL CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() { CWSRSettings::GetInstance().Load(); }
  ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance,
                              KODI_ADDON_INSTANCE_HDL& hdl) override
  {
    hdl = new CWSRCodec(instance);
    return ADDON_STATUS_OK;
  }
  virtual ~CMyAddon() = default;

  ADDON_STATUS SetSetting(const std::string& settingName,
                          const kodi::addon::CSettingValue& settingValue) override
  {
    return CWSRSettings::GetInstance().SetSetting(settingName, settingValue) ? ADDON_STATUS_OK
                                                                             : ADDON_STATUS_UNKNOWN;
  }
};

ADDONCREATOR(CMyAddon)

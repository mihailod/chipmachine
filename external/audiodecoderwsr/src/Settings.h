/*
 *  Copyright (C) 2020-2021 Team Kodi <https://kodi.tv>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <atomic>
#include <kodi/General.h>

class CWSRSettings
{
public:
  static CWSRSettings& GetInstance()
  {
    static CWSRSettings settings;
    return settings;
  }

  bool Load();
  bool SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue);

  bool GetPlayInfinitely() const { return m_playInfinitely; }
  int GetDefaultSongLength() const { return m_defaultSongLength; }
  int GetDefaultFadeLength() const { return m_defaultFadeLength; }
  int GetDefaultLoopCount() const { return m_defaultLoopCount; }
  bool GetDetectSilence() const { return m_detectSilence; }
  int GetSilenceLength() const { return m_silenceLength; }
  bool GetIgnorePlaylist() const { return m_ignorePlaylist; }
  int GetDefaultSubsongMax() const { return m_defaultSubsongMax; }
  int GetVolume() const { return m_volume; }
  bool GetDisplayTrackNum() const { return m_displayTrackNum; }
  unsigned int GetChannelMuting() const { return m_channelMuting; }
  int GetSampleRate() const { return m_sampleRate; }

private:
  CWSRSettings() = default;

  bool m_playInfinitely{false};
  int m_defaultSongLength{300000};
  int m_defaultFadeLength{5000};
  int m_defaultLoopCount{-1};
  bool m_detectSilence{true};
  int m_silenceLength{5};
  bool m_ignorePlaylist{false};
  int m_defaultSubsongMax{255};
  int m_volume{100};
  bool m_displayTrackNum{false};
  unsigned int m_channelMuting{0};
  int m_sampleRate{48000};
};

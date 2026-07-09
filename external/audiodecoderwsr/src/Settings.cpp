/*
 *  Copyright (C) 2020-2021 Team Kodi <https://kodi.tv>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Settings.h"

#include <math.h>

bool CWSRSettings::Load()
{
  m_playInfinitely = kodi::addon::GetSettingBoolean("playInfinitely", false);
  m_defaultSongLength = kodi::addon::GetSettingInt("defaultlength", 300) * 1000;
  m_defaultFadeLength = kodi::addon::GetSettingInt("defaultfade", 5000);
  m_defaultLoopCount = kodi::addon::GetSettingInt("loopcount", -1);
  m_detectSilence = kodi::addon::GetSettingBoolean("detectsilence", true);
  m_silenceLength = kodi::addon::GetSettingInt("silenceseconds", 5);
  m_ignorePlaylist = kodi::addon::GetSettingBoolean("ignoreplaylist", false);
  m_volume = kodi::addon::GetSettingInt("volume", 100);
  m_defaultSubsongMax = kodi::addon::GetSettingInt("subsongmax", 255);
  m_displayTrackNum = kodi::addon::GetSettingBoolean("displaytracknumber", true);
  m_channelMuting = 0;
  m_channelMuting |= kodi::addon::GetSettingBoolean("channelmuting1", false) ? (1 << 0) : 0;
  m_channelMuting |= kodi::addon::GetSettingBoolean("channelmuting2", false) ? (1 << 1) : 0;
  m_channelMuting |= kodi::addon::GetSettingBoolean("channelmuting3", false) ? (1 << 2) : 0;
  m_channelMuting |= kodi::addon::GetSettingBoolean("channelmuting4", false) ? (1 << 3) : 0;
  m_sampleRate = kodi::addon::GetSettingInt("samplerate", 5000);

  return true;
}

bool CWSRSettings::SetSetting(const std::string& settingName,
                              const kodi::addon::CSettingValue& settingValue)
{
  if (settingName == "playInfinitely")
  {
    if (settingValue.GetBoolean() != m_playInfinitely)
      m_playInfinitely = settingValue.GetBoolean();
  }
  else if (settingName == "defaultlength")
  {
    if (settingValue.GetInt() * 1000 != m_defaultSongLength)
      m_defaultSongLength = settingValue.GetInt() * 1000;
  }
  else if (settingName == "defaultfade")
  {
    if (settingValue.GetInt() != m_defaultFadeLength)
      m_defaultFadeLength = settingValue.GetInt();
  }
  else if (settingName == "loopcount")
  {
    if (settingValue.GetInt() != m_defaultLoopCount)
      m_defaultLoopCount = settingValue.GetInt();
  }
  else if (settingName == "detectsilence")
  {
    if (settingValue.GetBoolean() != m_detectSilence)
      m_detectSilence = settingValue.GetBoolean();
  }
  else if (settingName == "silenceseconds")
  {
    if (settingValue.GetInt() != m_silenceLength)
      m_silenceLength = settingValue.GetInt();
  }
  else if (settingName == "ignoreplaylist")
  {
    if (settingValue.GetBoolean() != m_ignorePlaylist)
      m_ignorePlaylist = settingValue.GetBoolean();
  }
  else if (settingName == "volume")
  {
    if (settingValue.GetInt() != m_volume)
      m_volume = settingValue.GetInt();
  }
  else if (settingName == "subsongmax")
  {
    if (settingValue.GetInt() != m_defaultSubsongMax)
      m_defaultSubsongMax = settingValue.GetInt();
  }
  else if (settingName == "displaytracknumber")
  {
    if (settingValue.GetBoolean() != m_displayTrackNum)
      m_displayTrackNum = settingValue.GetBoolean();
  }
  else if (settingName == "channelmuting1")
  {
    int flag = m_channelMuting;
    if (settingValue.GetBoolean())
      flag |= (1 << 0);
    else
      flag &= ~(1 << 0);
    if (flag != m_channelMuting)
      m_channelMuting = flag;
  }
  else if (settingName == "channelmuting2")
  {
    int flag = m_channelMuting;
    if (settingValue.GetBoolean())
      flag |= (1 << 1);
    else
      flag &= ~(1 << 1);
    if (flag != m_channelMuting)
      m_channelMuting = flag;
  }
  else if (settingName == "channelmuting3")
  {
    int flag = m_channelMuting;
    if (settingValue.GetBoolean())
      flag |= (1 << 2);
    else
      flag &= ~(1 << 2);
    if (flag != m_channelMuting)
      m_channelMuting = flag;
  }
  else if (settingName == "channelmuting4")
  {
    int flag = m_channelMuting;
    if (settingValue.GetBoolean())
      flag |= (1 << 3);
    else
      flag &= ~(1 << 3);
    if (flag != m_channelMuting)
      m_channelMuting = flag;
  }
  else if (settingName == "samplerate")
  {
    if (settingValue.GetInt() != m_sampleRate)
      m_sampleRate = settingValue.GetInt();
  }

  return true;
}

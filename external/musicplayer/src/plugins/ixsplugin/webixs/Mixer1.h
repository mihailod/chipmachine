/*
*   Copyright (C) 2022 Juergen Wothke
*   Copyright (C) original x86 code: Shortcut Software Development BV
*
* LICENSE
*
*   This software is licensed under a CC BY-NC-SA
*   (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#ifndef IXS_MIXER1_H
#define IXS_MIXER1_H

#include "MixerBase.h"

namespace IXS {

  /**
   * Seems to be some (unused) impl for 8-bit output..
   *
   * This might be useful as a simple example since the impl does not use the
   * low-level MMX optimizations.
   *
   * note: untested!
   */
  struct Mixer1 {
    IXS__MixerBase__VFTABLE* vftable;
  };

  void __thiscall IXS__Mixer1__virt4_1_0040a820(MixerBase *mixer, Buf0x40 *buf);
  void __thiscall IXS__Mixer1__virt3_1_0x40aa40(MixerBase *mixer, Buf0x40 *buf);

  MixerBase *IXS__Mixer1__ctor(byte *audioOutBuffer);

  void IXS__Mixer1__switchTo(MixerBase *);

}
#endif //IXS_MIXER1_H

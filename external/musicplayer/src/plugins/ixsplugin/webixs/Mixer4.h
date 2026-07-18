/*
*   Copyright (C) 2022 Juergen Wothke
*   Copyright (C) original x86 code: Shortcut Software Development BV
*
* LICENSE
*
*   This software is licensed under a CC BY-NC-SA
*   (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#ifndef IXS_MIXER4_H
#define IXS_MIXER4_H

#include "MixerBase.h"

namespace IXS {

  /**
   * This seems to a some (unused) impl for testing.
   */
  typedef struct Mixer4 Mixer4;
  struct Mixer4 {
    IXS__MixerBase__VFTABLE* vftable;
  };

  void __thiscall IXS__Mixer4__virt2_0040aca0(MixerBase *mixer, Buf0x40 *bufIdx);
  void __thiscall IXS__Mixer4__virt34_4_0040bfc0(MixerBase *mixer, Buf0x40 *bufIdx);

  MixerBase *IXS__Mixer4__ctor(byte *audioOutBuffer);

  void IXS__Mixer4__switchTo(MixerBase *mixer);
}
#endif //IXS_MIXER4_H

/*
*   Copyright (C) 2022 Juergen Wothke
*   Copyright (C) original x86 code: Shortcut Software Development BV
*
* LICENSE
*
*   This software is licensed under a CC BY-NC-SA
*   (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#include "Mixer4.h"
#include "asmEmu.h"

namespace IXS {
  void __thiscall IXS__Mixer4__virt2_0040aca0(MixerBase *mixer, uint bufIdx);
  void __thiscall IXS__Mixer4__virt34_4_0040bfc0(MixerBase *mixer, Buf0x40 *buf);

  static IXS__MixerBase__VFTABLE IXS_MIX4_VFTAB_0042ff94 = {
          IXS__MixerBase__virt0_00409ac0,
          IXS__MixerBase__clearSampleBuf_00409af0,
          IXS__Mixer4__virt2_0040aca0,
          IXS__Mixer4__virt34_4_0040bfc0,
          IXS__Mixer4__virt34_4_0040bfc0
  };

  MixerBase *IXS__Mixer4__ctor(byte *audioOutBuffer) {
    MixerBase *mixer = (MixerBase *) malloc(sizeof(MixerBase));
    if (mixer != (MixerBase *) nullptr) {
      IXS__MixerBase__ctor_004098b0(mixer);
      mixer->vftable = &IXS_MIX4_VFTAB_0042ff94;
      mixer->circAudioOutBuffer_0x1c = audioOutBuffer;
    }
    return mixer;
  }

  void IXS__Mixer4__switchTo(MixerBase *mixer) {
    if (mixer != (MixerBase *) nullptr) {
      mixer->vftable = &IXS_MIX4_VFTAB_0042ff94;
    }
  }


  void __thiscall IXS__Mixer4__virt2_0040aca0(MixerBase *mixer, uint bufIdx) {
    mixer->arrBuf20Ptr_0x38[bufIdx].totalBytesOut_0x4 =
            mixer->outBufOverflowCount_0x18 * 0x80000 + mixer->audioOutBufPos_0x10; // 524288 bytes buffer
    mixer->arrBuf20Ptr_0x38[bufIdx].int_0x8 = mixer->blockLen_0x14;
    mixer->arrBuf20Ptr_0x38[bufIdx].int_0x0 = 1;

    uint p = mixer->blockLen_0x14 + mixer->audioOutBufPos_0x10 & 0x7ffff;
    if ((int) p < mixer->audioOutBufPos_0x10) {
      mixer->outBufOverflowCount_0x18 += 1;
    }
    mixer->audioOutBufPos_0x10 = p;
  }

  void __thiscall IXS__Mixer4__virt34_4_0040bfc0(MixerBase *mixer, Buf0x40 *buf) {
  }
}
/*
*   Copyright (C) 2022 Juergen Wothke
*   Copyright (C) original x86 code: Shortcut Software Development BV
*
* LICENSE
*
*   This software is licensed under a CC BY-NC-SA
*   (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#include "Mixer1.h"
#include "asmEmu.h"

namespace IXS {
  void __thiscall IXS__Mixer1__virt4_1_0040a820(MixerBase *mixer, Buf0x40 *buf);

  void __thiscall IXS__Mixer1__virt3_1_0x40aa40(MixerBase *mixer, Buf0x40 *buf);

  static IXS__MixerBase__VFTABLE IXS_MIX1_VFTAB_0042ff58 = {
          IXS__MixerBase__virt0_00409ac0,
          IXS__MixerBase__clearSampleBuf_00409af0,
          IXS__MixerBase__copyToAudioBuf_0x409b30,
          IXS__Mixer1__virt3_1_0x40aa40,
          IXS__Mixer1__virt4_1_0040a820
  };

  MixerBase *IXS__Mixer1__ctor(byte *audioOutBuffer) {
    MixerBase *mixer = (MixerBase *) malloc(sizeof(MixerBase));
    if (mixer != (MixerBase *) nullptr) {
      IXS__MixerBase__ctor_004098b0(mixer);
      mixer->vftable = &IXS_MIX1_VFTAB_0042ff58;
      mixer->circAudioOutBuffer_0x1c = audioOutBuffer;
    }
    return mixer;
  }

  void IXS__Mixer1__switchTo(MixerBase *mixer) {
    if (mixer != (MixerBase *) nullptr) {
      mixer->vftable = &IXS_MIX1_VFTAB_0042ff58;
    }
  }

  // some class vars
  int		IXS_INT_0043bba0;
  int		IXS_INT_0043bba4;
  byte	IXS_BYTE_0043bba8;
  int		IXS_INT_0043bbac;
  byte	IXS_BYTE_0043bbb0;
  uint	IXS_UINT_0043bbb4;
  int		IXS_INT_0043bbb8;


  void __thiscall IXS__Mixer1__virt3_1_0x40aa40(MixerBase *mixer, Buf0x40 *buf)
  {
    Buf0x40 *v3 = buf;
    if ( buf->smplForwardLoopFlag_0x18 != 255 )
    {
      double b = (double)buf->uint_0x24 / (double)mixer->sampleRate_0x8 * 256.0;
      int numSmpls = (mixer->blockLen_0x14 / (int)mixer->bytesPerSample_0x2c);
      int b0 = (int64)b;
      byte *smplData = buf->smplDataPtr_0x4;
      int vol = (buf->volume_0x14 * (64 - buf->channelPan_0x10));
      short *smplBuf = mixer->smplBuf16Ptr_0x20;
      int b0_ = b0;
      double v0 = (double)(int)vol * mixer->outputVolume_0x24;
      IXS_INT_0043bbb8 = (int64)v0;
      vol = (buf->channelPan_0x10 * buf->volume_0x14);
      IXS_INT_0043bba4 = (int64)((double)(int)vol * mixer->outputVolume_0x24);

      if ((int)numSmpls > 0 ) {
        while ( true ) {
          if ( (IXS_BYTE_0043bba8 & 1) == 0 ) {
            IXS_INT_0043bba0 = b0;
            IXS_BYTE_0043bba8 |= 1;
          }
          int pos = v3->pos_0x0;
          if ( v3->int_0x8 == -1 ) {
            int loopStart = v3->smplLoopStart_0x1c;
            if ( pos < loopStart ) {
              v3->int_0x8 = 1;
              v3->pos_0x0 = 2 * loopStart - pos;
            }
          } else {
            int loopEnd = v3->smplLoopEnd_0x20;
            if ( pos > loopEnd ) {
              int forwardLoop = v3->smplForwardLoopFlag_0x18;
              if ( !forwardLoop ) {
                v3->smplForwardLoopFlag_0x18 = 255;
                return;
              }
              if (forwardLoop == 2 ) {
                v3->int_0x8 = 255;
                v3->pos_0x0 = 2 * loopEnd - pos;
              } else {
                v3->pos_0x0 = pos + v3->smplLoopStart_0x1c - loopEnd;
              }
            }
          }

          int p;
          if ( v3->int_0x8 == -1 ) {
            IXS_INT_0043bba0 = -b0;
            p = v3->pos_0x0 - v3->smplLoopStart_0x1c;
          } else {
            IXS_INT_0043bba0 = b0;
            p = v3->smplLoopEnd_0x20 - v3->pos_0x0;
          }
          int64 x = (int64)floor((double)p / (double)b0_);
          int x0 = x;

          if ( !(uint)x ) {
            GET_UINT(x, 0) = 1;
            x0 = 1;
          }
          if ((int)x > numSmpls ) {
            GET_UINT(x, 0) = numSmpls;
            x0 = numSmpls;
          }

          uint bitsPerSmpl = v3->bitsPerSmpl_0xc;
          numSmpls -= x;

          uint p0 = v3->pos_0x0;
          v3->pos_0x0 += x * IXS_INT_0043bba0;

          ushort *out = (ushort *)smplBuf;

          if ( bitsPerSmpl == 8 ) {
            int bn = IXS_INT_0043bba0;
            uint h0 = p0 >> 8;
            uint h = p0;

            for (int i = 0; i<x0; i++) {
              h += bn;
              out[0] += (IXS_INT_0043bba4 * *(char *)(smplData + h0 )) >> 8;
              out[1] += (IXS_INT_0043bbb8 * *(char *)(smplData + h0 )) >> 8;
              h0 = h >> 8;
//              bn = IXS_INT_0043bba0;  // redundant
              out += 2;
            }

          } else {
            int bn = IXS_INT_0043bba0;
            uint h0 = p0 >> 8;
            uint h = p0;

            for (int i = 0; i<x0; i++) {
              h += bn;
              out[0] += (uint)(IXS_INT_0043bba4 * *(short *)(smplData + 2 * h0)) >> 16;
              out[1] += (uint)(IXS_INT_0043bbb8 * *(short *)(smplData + 2 * h0)) >> 16;
              h0 = h >> 8;
//              bn = IXS_INT_0043bba0;  // redundant
              out += 2;
            }
         }
          if (numSmpls <= 0 )  {
            return;
          }
          b0 = b0_;
        }
      }
    }
  }

  void __thiscall IXS__Mixer1__virt4_1_0040a820(MixerBase *mixer, Buf0x40 *buf)
  {
    Buf0x40 *buf0 = buf;
    if ( buf->smplForwardLoopFlag_0x18 != 255 ) {

      double b = (double)buf->uint_0x24 / (double)mixer->sampleRate_0x8 * 256.0;
      int numSmpls = mixer->blockLen_0x14 / (int)mixer->bytesPerSample_0x2c;
      uint v = buf->volume_0x14;
      int b0 = (int64)b;
      byte *smplData = buf->smplDataPtr_0x4;
      int b0_ = b0;
      uint vol = v << 6;
      short *smplBuf = mixer->smplBuf16Ptr_0x20;

      if ( (IXS_BYTE_0043bbb0 & 1) == 0 ) {
        IXS_INT_0043bbac = (int64)((double)(int)(v << 6) * mixer->outputVolume_0x24);
        IXS_BYTE_0043bbb0 |= 1;
      }

      if (numSmpls > 0 ) {

        while ( true ) {
          if ( (IXS_BYTE_0043bbb0 & 2) == 0 ) {
            IXS_UINT_0043bbb4 = b0;
            IXS_BYTE_0043bbb0 |= 2;
          }
          int pos = buf0->pos_0x0;
          if ( buf0->int_0x8 == -1 ) {
            int loopStart = buf0->smplLoopStart_0x1c;
            if ( pos < loopStart ) {
              buf0->int_0x8 = 1;
              buf0->pos_0x0 = 2 * loopStart - pos;
            }

          } else {
            int loopEnd = buf0->smplLoopEnd_0x20;
            if ( pos > loopEnd ) {
              int forwardLoop = buf0->smplForwardLoopFlag_0x18;
              if ( !forwardLoop ) {
                buf0->smplForwardLoopFlag_0x18 = 255;
                return;
              }
              if (forwardLoop == 2 ) {
                buf0->int_0x8 = 255;
                buf0->pos_0x0 = 2 * loopEnd - pos;
              } else {
                buf0->pos_0x0 = pos + buf0->smplLoopStart_0x1c - loopEnd;
              }
            }
          }
          if ( buf0->int_0x8 == -1 ) {
            IXS_UINT_0043bbb4 = -b0;
            vol = buf0->pos_0x0 - buf0->smplLoopStart_0x1c;
          } else {
            IXS_UINT_0043bbb4 = b0;
            vol = buf0->smplLoopEnd_0x20 - buf0->pos_0x0;
          }
          int64 x = (int64)floor((double)vol / (double)b0_);

          int x0 = x;
          if ( !(uint)x ) {
            GET_UINT(x, 0) = 1;
            x0 = 1;
          }
          if ((int)x > numSmpls ) {
            GET_UINT(x, 0) = numSmpls;
            x0 = numSmpls;
          }
          uint bitsPerSmpl = buf0->bitsPerSmpl_0xc;
          numSmpls -= x;

          uint p = buf0->pos_0x0;
          buf0->pos_0x0 += x * IXS_UINT_0043bbb4;

          ushort *smplBuf0 = (ushort *)smplBuf;

          if (bitsPerSmpl == 8 ) {

            int bn = IXS_UINT_0043bbb4;
            int h0 = p >> 8;
            uint h = p;

            for (int i = 0; i<x0; i++) {
              h += bn;
              *smplBuf0 += (IXS_INT_0043bbac * *(char *)(smplData + h0)) >> 8;
              h0 = h >> 8;
//              bn = IXS_UINT_0043bbb4; // redundant
              ++smplBuf0;
            }

          } else {

            int bn = IXS_UINT_0043bbb4;
            int h0 = p >> 8;
            uint h = p;

            for (int i = 0; i<x0; i++) {
              h += bn;
              *smplBuf0 += (uint)(IXS_INT_0043bbac * *(short *)(smplData + 2 * h0)) >> 16;
              h0 = h >> 8;
//              bn = IXS_UINT_0043bbb4; // redundant
              ++smplBuf0;
            }
          }
          if (numSmpls <= 0 )  {
            return;
          }
          b0 = b0_;
        }
      }
    }
  }
}
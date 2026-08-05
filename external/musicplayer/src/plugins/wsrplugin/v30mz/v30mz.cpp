//chipmachine local patch: <ares/ares.hpp> replaced by the nall slice (pulled in
//by v30mz.hpp), and serialization.cpp / disassembler.cpp are not vendored.
#include "v30mz.hpp"

namespace ares {

enum : u32 { Byte = 1, Word = 2, Long = 4 };
#include "registers.cpp"
#include "memory.cpp"
#include "prefetch.cpp"
#include "modrm.cpp"
#include "algorithms.cpp"
#include "instruction.cpp"
#include "instructions-adjust.cpp"
#include "instructions-alu.cpp"
#include "instructions-exec.cpp"
#include "instructions-flag.cpp"
#include "instructions-group.cpp"
#include "instructions-misc.cpp"
#include "instructions-move.cpp"
#include "instructions-string.cpp"

auto V30MZ::power() -> void {
  static constexpr u16 Undefined = 0x0000;

  state.halt = 0;
  state.poll = 1;
  state.prefix = 0;
  state.interrupt = 0;
  state.brk = 0;
  state.nmi = 0;

  opcode = 0;
  prefixFlush();
  modrm.mod = 0;
  modrm.reg = 0;
  modrm.mem = 0;
  modrm.segment = 0;
  modrm.address = 0;

  AW  = Undefined;
  CW  = Undefined;
  DW  = Undefined;
  BW  = Undefined;
  SP  = Undefined;
  BP  = Undefined;
  IX  = Undefined;
  IY  = Undefined;
  DS1 = 0x0000;
  PS  = 0xffff;
  SS  = 0x0000;
  DS0 = 0x0000;
  PC  = 0x0000;
  PSW = 0x8000;
  flush();
}


}

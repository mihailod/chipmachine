# NEC V30MZ — vendored from ares

`*.cpp` / `v30mz.hpp` are ares' V30MZ core, taken from
`ares/component/processor/v30mz/` (https://github.com/ares-emulator/ares).

**Licence: ISC.** ares' core software is ISC-licensed (see the `LICENSE` file at
the root of that repository), which is a permissive BSD/MIT-style licence and is
fine for both chipmachine variants. It replaces the MAME/OSWAN-derived
`nec/` core that came with in_wsr, which was GPL-2-or-later.

## Why this core

The V30MZ is a WonderSwan-specific CPU and ares is the only permissively
licensed emulator that implements it in C++ (StoicGoose is MIT but C#). It is
also unusually easy to lift: the class talks to the outside world through nine
pure virtuals — `step`, `width`, `speed`, `read`, `write`, `in`, `out`,
`ioWidth`, `ioSpeed` — and has no dependency on ares' Node tree, its scheduler
or its threading.

## What is NOT vendored

- `disassembler.cpp` — needs nall's `string`.
- `serialization.cpp` — needs nall's `serializer`; there is no save state here.

## Local patches (re-apply on revendor)

`.orig` copies of both patched files sit next to them; `grep "chipmachine local
patch"` finds every edit.

- `v30mz.hpp` — includes `nall_compat.hpp` instead of getting nall through
  `<ares/ares.hpp>`, and drops the `serialize` / `disassemble*` declarations.
- `v30mz.cpp` — drops `#include <ares/ares.hpp>` and the two `#include`s of the
  files above.

Nothing else was touched: the instruction set, the cycle counts, the prefetch
queue and the flag handling are ares' code as published.

## nall_compat.hpp

Not ares code — written for this project. It supplies the four nall facilities
the core uses (`Natural`/`Integer` sized integers, `BitField`, `queue`, `maybe`)
without pulling in nall itself, whose `primitives.hpp` drags in its string,
serializer and traits headers.

The semantics are the part that matters. `Natural<N>` masks on every write and
`Integer<N>` sign-extends on every write; the core is written assuming both.
`(u4)x + (u4)y + (u4)c >= 16` is how it computes the auxiliary carry, every
relative jump is `(i8)fetch<Byte>()`, and addresses are `n20` so the 1 MB space
wraps by itself. A shim that used plain `int`s would compile and be wrong.

## Verifying it after a revendor

`v30mz_selftest.cpp` (built by cmtest) boots the core the way a WSR file does —
far JMP at `FFFF:0000` — and checks `REP MOVSB`, `PUSH`/`POP`, `DAA` (which
exercises the auxiliary-carry path and therefore the masking shim), `IN`/`OUT`,
`HLT`, and an `INT` vector fetch plus `IRET`. Note that `interrupt(vector)`
returns false unless `PSW.IE` **and** the pre-instruction `state.interrupt`
snapshot are both set, so a test that never issues `STI` will see no interrupt at
all — that is correct behaviour, not a fault.

Also note only **16 bytes** live at `FFFF:0000` before the 1 MB address space
wraps. Test programs must do what a real WSR footer does: put a far `JMP` there
and the code lower down.

# The WonderSwan, written from the WSdev Wiki

`ws_apu.c` is the sound chip, `ws_machine.cpp` everything around it. Both are
chipmachine's own code, written against the register documentation at
**https://ws.nesdev.org** (pages *Sound*, *Hyper Voice*, *DMA*, *Memory map*,
*Mapper*, *Interrupts*, *Timers*, *Display*) rather than derived from any
emulator. The CPU next door is ares' V30MZ under ISC.

Fetch a wiki page's source with
`curl https://ws.nesdev.org/wiki/Special:Export/Sound` — `?action=raw` 404s
there.

The `.wsr` container is documented by Mamiya's own readme, archived as
[`awesome-wsdev/archive/in_wsr.txt`](https://github.com/WonderfulToolchain/awesome-wsdev/blob/main/archive/in_wsr.txt)
(Japanese): the file is `64KB * n`, the last 32 bytes are the header (`WSRF`,
version, **first song number at +5**, a far `JMP` at +$10, then the copied cart
header bytes), and playback is nothing more than booting the machine with
`AX = song number` at `CS:IP = FFFF:0000`. There is no length, no song count and
no metadata of any kind.

## What is emulated

Everything that can make a sound, and nothing else: CPU, the memory map and its
bank registers, the horizontal/vertical blank timers, the interrupt controller,
the display's **line counter** (which is what the timers count and therefore what
sets the tempo), general DMA, sound DMA, Hyper Voice and the four sound channels.
There is no PPU, no sprite or tile rendering, no keypad, no serial, no EEPROM.

Output is rendered **straight to 44100 Hz** — the APU's phase accumulators are
derived from the requested rate — so there is no resampler in this plugin.

## Five things that cost real debugging time

1. **A cartridge is aligned to the TOP of the bank space.** Its last 64 KB bank
   is number `$FF` and bank numbers count *downwards*, which is what puts the
   WSRF footer under the reset vector at `FFFF:0000`, and why every bank register
   except `$C1` powers up holding `$FF`. Masking the address with `size - 1`
   instead only works for power-of-two images: a 192 KB rip (Final Fantasy) then
   boots into the middle of its own data and executes garbage — the CPU runs, the
   PC wanders, and the file renders **silence**.
2. **The APU's port range ends at `$9E`, not `$FF`.** Forwarding everything from
   `$80` up to the chip swallows the timers (`$A2-$AB`), the interrupt controller
   (`$B0-$B7`) and the bank registers (`$C0-$C3`). Every rip then boots, writes
   its whole setup — and plays nothing, because the interrupt it enabled in `$B2`
   never arrives and `$C3` never switches a bank.
3. **A one-shot timer must not clear its own enable bit.** Nothing in the
   documentation says it does, and clearing it breaks the commonest idiom there
   is: arm a one-shot HBlank timer, then re-arm it from the interrupt handler by
   rewriting the reload port. Do that and the timer fires exactly once (Glocal
   Hexcite: silence, while still looking alive — interrupts, register writes and
   all). The same bug made Guilty Gear Petit render **2.77x** too loud.
4. **Some rips never write `$A2` at all.** They enable the timer's interrupt in
   `$B2`, program a reload, and spin. in_wsr ignored `$A2` entirely, so such rips
   exist and shipped — "With You - Mitsumete Itai" is one. `tick()` therefore
   also counts when the interrupt is enabled and a non-zero reload is programmed.
   This is a documented compatibility relaxation, not hardware: it can only add
   sound where a strict reading gives silence.
5. **This path has to apply libvgm's chip volume itself.** The APU's scale was
   calibrated inside libvgm, which then multiplies `DEVID_WSWAN` by its
   `_CHIP_VOLUME` entry of `0x200` = exactly 2.0. Without the matching gain here
   every `.wsr` renders at a measured 0.50.

## Known deviations from in_wsr

- **Sound DMA rates.** The wiki documents 4000 / 6000 / 12000 / 24000 Hz;
  in_wsr's readme says outright that it *guessed* 12/16/20/24 kHz. Rips that
  stream through sound DMA will differ.
- **The channel-2 voice level.** The mixing diagram makes each channel an 8-bit
  bus into the unsigned 10-bit sum, so an 8-bit PCM sample at 100% spans 0-255
  against a wavetable channel's 0-225. in_wsr mixes its voice louder than that.
  The six rips that measure below 0.90 against it are all `$94 = $0F`
  voice-streamers — Klonoa (0.67) most of all.
- **Hyper Voice's `rrr` update-rate field is ignored**: the converted sample is
  simply held. Sound DMA already delivers samples at its own rate, which is how
  Hyper Voice is fed in practice.
- Port `$91`'s output-enable bits and `$95`/`$9E` are ignored — see the header
  comment in `ws_apu.c` for why.

## Measured

168 modland rips, 20 s each, against in_wsr: **median 0.983**, mean 0.976,
**96% within ±10%**, nothing newly silent, **no clipping in either player**.

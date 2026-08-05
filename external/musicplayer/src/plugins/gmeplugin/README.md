# gmeplugin — Game Music Emulator

8-bit console and home-computer chip logs, via **Game Music Emu**.

## Systems

ZX Spectrum, Amstrad CPC, Nintendo Game Boy, Sega Genesis / Mega Drive, NEC
TurboGrafx-16 / PC Engine, MSX and other Z80 systems, Nintendo NES / Famicom
(including VRC6, Namco 106 and FME-7 expansion sound), Atari systems using the
POKEY chip, Super Nintendo / Super Famicom, Sega Master System / Mark III, BBC
Micro.

## Extensions

`.spc` `.nsf` `.nsfe` `.gbs` `.gbr` `.ay` `.gym` `.sap` `.vgm` `.vgz` `.hes`
`.kss` `.sgc` `.emul`

## Notes

* **`.gbr`** is the older Game Boy rip format (predecessor of `.gbs`). GBR
  carries no "first song" field and many rips keep a silent stop-track at song
  0 — use the subsong controls (LEFT/RIGHT cursor keys) if a tune starts silent.
* **`.ay`** — the ZXAYEMUL container of raw Z80 rips — belongs to GME in both
  builds, which plays the Amstrad CPC rips that Ayfly renders silent. See
  [zxayplugin](../zxayplugin/README.md).
* **`.sap`** — the 6,617-song ASMA corpus plays here in both builds, which is
  what makes the Plus-only ASAP gating in
  [pokeynoiseplugin](../pokeynoiseplugin/README.md) cheap.
* **`.vgm`** — AY8910 (Vectrex) VGM logs are handled here; OPL2/OPL3 `.vgz`
  goes to [libvgmplugin](../libvgmplugin/README.md), which GME cannot do.
* **`.sgc`** — Sega Master System FM uses `emu2413` in place of GME's YM2413
  stub.
* GME's `Sms_Apu::run_until` carries an assert that fires on malformed rips;
  Release builds here do **not** define `NDEBUG`, so vendored asserts are live.

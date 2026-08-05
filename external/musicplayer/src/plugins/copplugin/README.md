# copplugin — Sam Coupé

**Sam Coupé** music (the modland "Sam Coupe COP" corpus) for the Philips
**SAA1099** sound chip.

Extensions: `.cop`

## How it works

Each `.cop` file is a SAM Coupé memory image whose Z80 replay routine is either
compiled into the song or is the shared **E-Tracker** player. That original Z80
routine runs on an embedded Z80 core (the GME core), with its SAA1099 port writes
driving Dave Hooper's **SAASound** emulator. The load and calling convention
follow Christopher O'Neill's SCPlayer.

## Routing

`.cop` is shared with the zxart **E-Tracker** variant decoded by ZXTune; routing
is by content.

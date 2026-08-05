# rsnplugin

RAR-packed music archives, primarily SNES `.spc` sets.

Extensions: `.rsn` `.rps` `.rdc` `.rds` `.rgs` `.r64`

The archive is unpacked in memory and the contained tunes handed to the plugin
that claims them (usually [gmeplugin](../gmeplugin/README.md)).

Note: solid RAR archives cannot be range-fetched, so remote sources that ship
solid RARs must be downloaded whole.

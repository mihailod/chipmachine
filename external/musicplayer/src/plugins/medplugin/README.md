# medplugin — old MED

Pre-OctaMED **MED** (Music EDitor) modules from the Amiga.

Extensions: `.med` (old MED only)

## Why a dedicated plugin

The old MED files crashed UADE, and libopenmpt's `.med` support targets the later
OctaMED `MMD*` formats. This plugin handles the early variant directly.

Later `.med` / `.mmd0` / `.mmd1` / `.mmd2` files still route to
[uadeplugin](../uadeplugin/README.md) (Plus) or
[openmptplugin](../openmptplugin/README.md).

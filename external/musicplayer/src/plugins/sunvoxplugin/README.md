# sunvoxplugin

**SunVox** music by Alexander Zolotov (NightRadio).

Extensions: `.sunvox`

## Engine

The closed-source SunVox library is loaded at runtime via `dlopen` of the arm64
dylib. There are **two** registration spots that must agree, and the packaged
`.app` needs the dylib in `Contents/Frameworks` (the packaging step's C++ probe
handles this).

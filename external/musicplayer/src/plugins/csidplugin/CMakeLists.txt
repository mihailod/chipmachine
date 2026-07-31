cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.csidplugin)

# Commodore 64 SID (.sid/.rsid) via Hermit's cSID -- WTFPL, no VICE/reSID code,
# which is the whole point: this is the SID engine the Mac App Store variant can
# ship where vicepluginbridge (GPLv2) cannot. See csid/csid_engine.c for the
# vendoring notes and the exact edits made to Hermit's original csid.c.

add_library(csidplugin
    CSIDPlugin.cpp
    csid/csid_engine.c
    CSIDPlugin.h
    csid/csid_engine.h)

# Hermit's code is C99 and leans on wrapping/implicit conversions throughout;
# it is vendored deliberately unchanged (the A/B fidelity result was measured
# against exactly this arithmetic), so do not let those warnings fail the build.
set_source_files_properties(csid/csid_engine.c PROPERTIES
    COMPILE_FLAGS "-Wno-conversion -Wno-sign-compare -Wno-unused-but-set-variable")

target_link_libraries(csidplugin PRIVATE coreutils)

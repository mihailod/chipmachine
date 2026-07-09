binary_name := mkhebios
dirs.root := ../../..

source_files += mkhebios.c mkhebios_main.cpp

libraries.common = binary_compression
libraries.3rdparty = zlib

defines += EMU_COMPILE EMU_LITTLE_ENDIAN

include $(dirs.root)/makefile.mak

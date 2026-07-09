// ZXTune archive/container plugin factory for the chipmachine zxtuneplugin.
//
// ZXTune ships three prebuilt variants of this factory under
// src/core/plugins/archives/{stub,full,lite}/ -- we provide our own instead so
// the build never pulls those directories. The full set (not just the raw
// container) is required: ZXTune's raw scanner takes a lookahead optimization
// over the other registered archive plugins, and registering raw alone leaves
// that table empty and crashes. The depack/packer/decompile plugins are what
// pull in lhasa + lzma.
#include "core/plugins/archives/plugins.h"
#include "core/plugins/archives/plugins_list.h"

namespace ZXTune
{
  void RegisterArchivePlugins(ArchivePluginsRegistrator& registrator)
  {
    RegisterRawContainer(registrator);
    RegisterArchiveContainers(registrator);
    RegisterZXArchiveContainers(registrator);
    RegisterMultitrackContainers(registrator);
    RegisterZdataContainer(registrator);
    RegisterDepackPlugins(registrator);
    RegisterChiptunePackerPlugins(registrator);
    RegisterDecompilePlugins(registrator);
  }
} // namespace ZXTune

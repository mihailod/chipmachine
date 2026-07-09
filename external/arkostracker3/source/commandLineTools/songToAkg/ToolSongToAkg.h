#pragma once

namespace arkostracker
{

/** Tool that loads any Song that AT can handle, and converts it to AKG. */
class ToolSongToAkg
{
public:
    static int execute(int argc, char* argv[]);     // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
};

}   // namespace arkostracker

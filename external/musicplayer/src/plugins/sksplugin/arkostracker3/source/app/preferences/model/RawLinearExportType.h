#pragma once

#include <cstdint>

namespace arkostracker
{

enum class RawLinearExportType : std::uint8_t
{
    all = 0,
    psgOnly,
    samplesOnly,
};

}   // namespace arkostracker

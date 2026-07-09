#pragma once

namespace arkostracker 
{

/** The type of the loaded music. */
enum class ImportedFormat
{
    native,
    at1,
    at2,
    midi,
    mod,
    soundtrakker128,
    starkos,
    wyz,
    chp,
    vt2,

    unknown
};

}   // namespace arkostracker


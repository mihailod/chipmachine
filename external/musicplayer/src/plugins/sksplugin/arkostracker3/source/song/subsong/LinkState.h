#pragma once

namespace arkostracker
{

/** Link to/from this Track? */
enum class LinkState : unsigned char
{
    none,
    /** The Track is linked to another Track. */
    link,
    /** Track(s) refers to this Track. */
    linkedTo,
};

}   // namespace arkostracker

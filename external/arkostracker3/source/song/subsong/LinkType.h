#pragma once

namespace arkostracker
{

/** Useful to declare what kind of Track link we want to find. */
enum class LinkType : unsigned char
{
    /** If unlinked, gets the unlinked Track. Else, the linked Track, is present, as it has priority. */
    unlinkedIfLinkedNotPresent,
    linkedOnly,
    unlinkedOnly,
};

}   // namespace arkostracker

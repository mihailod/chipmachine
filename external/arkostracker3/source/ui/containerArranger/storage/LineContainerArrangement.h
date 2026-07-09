#pragma once

#include <vector>

#include "ContainerArrangement.h"

namespace arkostracker 
{

/** Represents several Containers in an horizontal line. */
class LineContainerArrangement
{
public:
    /**
     * Constructor.
     * @param desiredHeight the desired height of the line, if any.
     */
    explicit LineContainerArrangement(OptionalInt desiredHeight) noexcept;

    /** Equality operator. */
    friend bool operator==(const LineContainerArrangement& left, const LineContainerArrangement& right);

    /**
     * Adds a ContainerArrangement.
     * @param ContainerArrangement the container arrangement to add.
     */
    void addContainerArrangement(const ContainerArrangement& containerArrangement) noexcept;

    /** @return the desired height of the line, if any. */
    OptionalInt getDesiredHeight() const;

    /** @return the container arrangements in this Line Container. */
    const std::vector<ContainerArrangement>& getContainerArrangements() const noexcept;

private:
    OptionalInt desiredHeight;                                  // The desired height of the line, if any.
    std::vector<ContainerArrangement> containerArrangements;    // The data of the Containers for this line.
};

}   // namespace arkostracker

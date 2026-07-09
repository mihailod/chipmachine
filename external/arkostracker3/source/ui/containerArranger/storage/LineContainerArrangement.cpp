#include "LineContainerArrangement.h"

namespace arkostracker 
{

LineContainerArrangement::LineContainerArrangement(OptionalInt pDesiredHeight) noexcept :
        desiredHeight(pDesiredHeight)
{
}

void LineContainerArrangement::addContainerArrangement(const ContainerArrangement& containerArrangement) noexcept
{
    containerArrangements.push_back(containerArrangement);
}

const std::vector<ContainerArrangement>& LineContainerArrangement::getContainerArrangements() const noexcept
{
    return containerArrangements;
}

OptionalInt LineContainerArrangement::getDesiredHeight() const
{
    return desiredHeight;
}

bool operator==(const LineContainerArrangement& left, const LineContainerArrangement& right)
{
    return (left.desiredHeight == right.desiredHeight)
           && (left.containerArrangements == right.containerArrangements);
}

}   // namespace arkostracker

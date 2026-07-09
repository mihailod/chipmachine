#include "ToolboxControllerNoOp.h"

namespace arkostracker
{
void ToolboxControllerNoOp::show(juce::Component& /*parent*/, int /*toolBoxButtonX*/, int /*toolBoxButtonY*/) noexcept
{
}

void ToolboxControllerNoOp::hide() noexcept
{
}

void ToolboxControllerNoOp::locate(int /*toolBoxButtonX*/, int /*toolBoxButtonY*/) noexcept
{
}

void ToolboxControllerNoOp::onUserSelectedTarget(Target /*target*/) noexcept
{
}

void ToolboxControllerNoOp::onUserWantsToChangeFirstChannelToSwap(int /*channelIndex*/) noexcept
{
}

void ToolboxControllerNoOp::onUserWantsToChangeSecondChannelToSwap(int /*channelIndex*/) noexcept
{
}

void ToolboxControllerNoOp::onUserChangedToolType(ToolType /*toolType*/) noexcept
{
}

void ToolboxControllerNoOp::onUserWantsToChangeTranspositionRate(int /*newTranspositionRate*/) noexcept
{
}

void ToolboxControllerNoOp::perform() noexcept
{
}

void ToolboxControllerNoOp::onSongMetadataChanged(const Id& /*subsongId*/) noexcept
{
}

void ToolboxControllerNoOp::onSubsongMetadataChanged(const Id& /*subsongId*/) noexcept
{
}

}   // namespace arkostracker

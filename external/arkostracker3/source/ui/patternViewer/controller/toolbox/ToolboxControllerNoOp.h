#pragma once

#include "ToolboxController.h"

namespace arkostracker
{

class ToolboxControllerNoOp final : public ToolboxController
{
public:
    void show(juce::Component& parent, int toolBoxButtonX, int toolBoxButtonY) noexcept override;
    void hide() noexcept override;
    void locate(int toolBoxButtonX, int toolBoxButtonY) noexcept override;
    void onUserSelectedTarget(Target target) noexcept override;
    void onUserWantsToChangeFirstChannelToSwap(int channelIndex) noexcept override;
    void onUserWantsToChangeSecondChannelToSwap(int channelIndex) noexcept override;
    void onUserChangedToolType(ToolType toolType) noexcept override;
    void onUserWantsToChangeTranspositionRate(int newTranspositionRate) noexcept override;
    void perform() noexcept override;
    void onSongMetadataChanged(const Id& subsongId) noexcept override;
    void onSubsongMetadataChanged(const Id& subsongId) noexcept override;
};

}   // namespace arkostracker

#include "PsgView.h"

#include <BinaryData.h>

#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

PsgView::PsgView(Listener& pListener) noexcept :
        listener(pListener),
        group(juce::String(), true),
        typeLabel(),
        psgFrequencyLabel(),
        referenceFrequencyLabel(),
        sampleReplayFrequencyLabel(),
        editButton(BinaryData::IconToolboxOn_png, BinaryData::IconToolboxOn_pngSize, juce::translate("Edit the PSG data"),
                   [&] (int, bool, bool) { onEditClicked(); }),
        deleteButton(BinaryData::IconDeleteOn_png, BinaryData::IconDeleteOn_pngSize,
                     juce::translate("Delete the PSG"),
                     [&] (int, bool, bool) { onDeleteClicked(); }),
        addButton(BinaryData::IconNew_png, BinaryData::IconNew_pngSize, juce::translate("Add a new PSG after"),
                  [&](int, bool, bool) { onAddButtonClicked(); }),
        psgIndex(0)
{
    addAndMakeVisible(group);

    referenceFrequencyLabel.setMinimumHorizontalScale(0.5F);

    group.addComponentToGroup(typeLabel);
    group.addComponentToGroup(psgFrequencyLabel);
    group.addComponentToGroup(referenceFrequencyLabel);
    group.addComponentToGroup(sampleReplayFrequencyLabel);
    group.addComponentToGroup(editButton);
    group.addComponentToGroup(deleteButton);
    group.addComponentToGroup(addButton);
}

void PsgView::setDisplayedData(const int currentPsgIndex, const int psgCount, const Psg& psg, bool canDelete) noexcept
{
    psgIndex = currentPsgIndex;
    group.setGroupTitle(juce::translate("Psg ") + juce::String(currentPsgIndex + 1) + "/" + juce::String(psgCount));

    const auto psgTypeText = juce::translate(PsgTypeUtil::psgTypeToDisplayedText(psg.getType())) + " (" +
            PsgMixingOutputUtil::psgMixingOutputToDisplayedText(psg.getPsgMixingOutput()) + ")";
    typeLabel.setText(psgTypeText, juce::NotificationType::dontSendNotification);

    psgFrequencyLabel.setText(juce::translate("Frequency: ") + juce::String(psg.getPsgFrequency()) + " Hz", juce::NotificationType::dontSendNotification);
    referenceFrequencyLabel.setText(juce::translate("Reference frequency: ") + juce::String(psg.getReferenceFrequency()) + " Hz",
                                    juce::NotificationType::dontSendNotification);
    sampleReplayFrequencyLabel.setText(juce::translate("Sample player freq.: ") + juce::String(psg.getSamplePlayerFrequency()) + " Hz",
                                    juce::NotificationType::dontSendNotification);

    deleteButton.setVisible(canDelete);
}

void PsgView::onEditClicked() const noexcept
{
    listener.onEditPsgButtonClicked(psgIndex);
}

void PsgView::onDeleteClicked() const noexcept
{
    listener.onDeletePsgButtonClicked(psgIndex);
}

void PsgView::onAddButtonClicked() const noexcept
{
    listener.onAddPsgButtonClicked(psgIndex);
}

// juce::Component method implementations.
// ==========================================

void PsgView::resized()
{
    group.setBounds(0, 0, getWidth(), getHeight());

    const auto innerArea = group.getGroupInnerArea();
    const auto width = innerArea.getWidth();

    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    constexpr auto left = 0;
    constexpr auto y = 0;
    const auto y2 = y + labelsHeight;
    const auto buttonsWidth = LookAndFeelConstants::buttonImagesWidth;
    const auto margins = LookAndFeelConstants::margins;
    constexpr auto firstColumnWidth = 200;
    const auto secondColumnX = firstColumnWidth + margins;
    constexpr auto secondColumnWidth = 210;

    typeLabel.setBounds(left, y, 90, labelsHeight);
    psgFrequencyLabel.setBounds(secondColumnX, y, secondColumnWidth, labelsHeight);
    referenceFrequencyLabel.setBounds(left, y2, firstColumnWidth, labelsHeight);
    sampleReplayFrequencyLabel.setBounds(secondColumnX, referenceFrequencyLabel.getY(), secondColumnWidth, labelsHeight);

    editButton.setBounds(width - buttonsWidth - 20, y, buttonsWidth, labelsHeight);
    deleteButton.setBounds(editButton.getX() - buttonsWidth - 2, y, buttonsWidth, labelsHeight);
    addButton.setBounds(editButton.getX(), y2, buttonsWidth, labelsHeight);

    group.setViewportHeight(referenceFrequencyLabel.getBottom());
}

}   // namespace arkostracker

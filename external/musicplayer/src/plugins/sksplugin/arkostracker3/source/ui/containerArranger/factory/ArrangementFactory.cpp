#include "ArrangementFactory.h"

#include "../PanelSizes.h"

namespace arkostracker
{

// What matters is the proportions, not the exact sizes.

const int ArrangementFactory::screenWidth = 500;
const int ArrangementFactory::mixerWidth = 100;
const int ArrangementFactory::mixerBesidesPvWidth = 200;

std::unique_ptr<DocumentArrangement> ArrangementFactory::createStoredArrangementMain() noexcept
{
    // The mixer + the Linker.
    // The instrument list + PatternViewer/Instrument Editor.
    // The TestArea.

    auto focusOrder = 0;

    // Mixer and Linker.
    LineContainerArrangement lineContainerArrangementTop(PanelSizes::linkerDesiredHeight);
    {
        constexpr auto linkerWidth = screenWidth - mixerWidth;
        const ContainerArrangement containerArrangementMeters(Dimension(mixerWidth), true, { PanelType::meters });
        const ContainerArrangement containerArrangementLinker(Dimension(linkerWidth), true, { PanelType::linker },
            { }, focusOrder++);
        lineContainerArrangementTop.addContainerArrangement(containerArrangementMeters);
        lineContainerArrangementTop.addContainerArrangement(containerArrangementLinker);
    }

    LineContainerArrangement lineContainerArrangementMiddle((OptionalInt()));
    {
        constexpr auto listWidth = PanelSizes::genericListDesiredWidth;
        constexpr auto rightViewersWidth = screenWidth - listWidth;

        const auto middlePanelTypes = { PanelType::patternViewer, PanelType::instrumentEditor,
                                        PanelType::arpeggioTable, PanelType::pitchTable };

        const ContainerArrangement containerArrangementLeft(Dimension(listWidth, PanelSizes::genericListMinimumWidth,
                                                                      PanelSizes::genericListMaximumWidth),
                                                            false, { PanelType::instrumentList, PanelType::arpeggioList, PanelType::pitchList },
                                                            middlePanelTypes, focusOrder++
        );
        const ContainerArrangement containerArrangementMiddle(Dimension(rightViewersWidth), true, middlePanelTypes,
                                                              { }, focusOrder++);
        lineContainerArrangementMiddle.addContainerArrangement(containerArrangementLeft);
        lineContainerArrangementMiddle.addContainerArrangement(containerArrangementMiddle);
    }

    LineContainerArrangement lineContainerArrangementBottom(PanelSizes::testAreaDesiredHeight);
    {
        const ContainerArrangement containerArrangement(Dimension(screenWidth), true, { PanelType::testArea },
            { }, focusOrder);
        lineContainerArrangementBottom.addContainerArrangement(containerArrangement);
    }

    auto documentArrangement = std::make_unique<DocumentArrangement>();
    documentArrangement->addLineContainerArrangement(lineContainerArrangementTop);
    documentArrangement->addLineContainerArrangement(lineContainerArrangementMiddle);
    documentArrangement->addLineContainerArrangement(lineContainerArrangementBottom);

    documentArrangement->setSelectedPanels(PanelType::patternViewer,
                                           {
                                                   PanelType::meters, PanelType::linker,
                                                   PanelType::instrumentList, PanelType::patternViewer,
                                                   PanelType::testArea
                                           });

    return documentArrangement;
}

std::unique_ptr<DocumentArrangement> ArrangementFactory::createStoredArrangementFocusOnPatternViewer() noexcept
{
    // The instrument list + PatternViewer/Instrument Editor.
    // The TestArea.

    auto focusOrder = 0;

    LineContainerArrangement lineContainerArrangementMiddle((OptionalInt()));
    {
        constexpr auto listWidth = PanelSizes::genericListDesiredWidth;
        constexpr auto rightViewersWidth = screenWidth - listWidth;
        const ContainerArrangement containerArrangementLeft(Dimension(listWidth, PanelSizes::genericListMinimumWidth,
                                                                      PanelSizes::genericListMaximumWidth), true,
                                                            { PanelType::instrumentList }, { }, focusOrder++);
        const ContainerArrangement containerArrangementMiddle(Dimension(rightViewersWidth), false,
                                                              { PanelType::patternViewer, PanelType::instrumentEditor }, { }, focusOrder++);
        lineContainerArrangementMiddle.addContainerArrangement(containerArrangementLeft);
        lineContainerArrangementMiddle.addContainerArrangement(containerArrangementMiddle);
    }
    LineContainerArrangement lineContainerArrangementBottom(PanelSizes::testAreaDesiredHeight);
    {
        const ContainerArrangement containerArrangement(Dimension(screenWidth), true, { PanelType::testArea },
            { }, focusOrder);
        lineContainerArrangementBottom.addContainerArrangement(containerArrangement);
    }

    auto documentArrangement = std::make_unique<DocumentArrangement>();
    documentArrangement->addLineContainerArrangement(lineContainerArrangementMiddle);
    documentArrangement->addLineContainerArrangement(lineContainerArrangementBottom);

    documentArrangement->setSelectedPanels(PanelType::patternViewer,
                                           {
                                                   PanelType::instrumentList, PanelType::patternViewer,
                                                   PanelType::testArea
                                           });
    return documentArrangement;
}

std::unique_ptr<DocumentArrangement> ArrangementFactory::createStoredArrangementFocusOnExpressions() noexcept
{
    // Arpeggio list + arpeggio editor.
    // Pitch list + pitch editor.
    // The TestArea.

    auto focusOrder = 0;

    LineContainerArrangement lineContainerArrangementArpeggio((OptionalInt()));
    {
        constexpr auto listWidth = PanelSizes::genericListDesiredWidth;
        constexpr auto rightViewersWidth = screenWidth - listWidth;

        const ContainerArrangement containerArrangementLeft(Dimension(listWidth, PanelSizes::genericListMinimumWidth,
                                                                      PanelSizes::genericListMaximumWidth),
                                                            true, { PanelType::arpeggioList },
                                                            { }, focusOrder++
        );
        const ContainerArrangement containerArrangementMiddle(Dimension(rightViewersWidth), true, { PanelType::arpeggioTable },
            { }, focusOrder++);
        lineContainerArrangementArpeggio.addContainerArrangement(containerArrangementLeft);
        lineContainerArrangementArpeggio.addContainerArrangement(containerArrangementMiddle);
    }

    LineContainerArrangement lineContainerArrangementPitch((OptionalInt()));
    {
        constexpr auto listWidth = PanelSizes::genericListDesiredWidth;
        constexpr auto rightViewersWidth = screenWidth - listWidth;

        const ContainerArrangement containerArrangementLeft(Dimension(listWidth, PanelSizes::genericListMinimumWidth,
                                                                      PanelSizes::genericListMaximumWidth),
                                                            true, { PanelType::pitchList }, { }, focusOrder++
        );
        const ContainerArrangement containerArrangementMiddle(Dimension(rightViewersWidth), true, { PanelType::pitchTable },
             { }, focusOrder++);
        lineContainerArrangementPitch.addContainerArrangement(containerArrangementLeft);
        lineContainerArrangementPitch.addContainerArrangement(containerArrangementMiddle);
    }

    LineContainerArrangement lineContainerArrangementTestArea(PanelSizes::testAreaDesiredHeight);
    {
        const ContainerArrangement containerArrangement(Dimension(screenWidth), true, { PanelType::testArea },
            { }, focusOrder);
        lineContainerArrangementTestArea.addContainerArrangement(containerArrangement);
    }

    auto documentArrangement = std::make_unique<DocumentArrangement>();
    documentArrangement->addLineContainerArrangement(lineContainerArrangementArpeggio);
    documentArrangement->addLineContainerArrangement(lineContainerArrangementPitch);
    documentArrangement->addLineContainerArrangement(lineContainerArrangementTestArea);

    documentArrangement->setSelectedPanels(PanelType::arpeggioTable,
                                           {
                                                   PanelType::arpeggioList, PanelType::pitchList, PanelType::pitchTable, PanelType::testArea
                                           });
    return documentArrangement;
}

std::unique_ptr<DocumentArrangement> ArrangementFactory::createStoredArrangementFocusOnListening() noexcept
{
    // The Linker.
    // The PatternViewer/mixer.

    /*LineContainerArrangement lineContainerArrangementMixer(PanelSizes::linkerDesiredHeight);
    {
        const ContainerArrangement containerArrangementMeters(Dimension({ }), true, { PanelType::meters });
        lineContainerArrangementMixer.addContainerArrangement(containerArrangementMeters);
    }*/

    auto focusOrder = 0;

    LineContainerArrangement lineContainerArrangementLinker(PanelSizes::linkerDesiredHeight);
    {
        const ContainerArrangement containerArrangementLinker(Dimension({ }), true, { PanelType::linker },
            { }, focusOrder++);
        lineContainerArrangementLinker.addContainerArrangement(containerArrangementLinker);
    }

    LineContainerArrangement lineContainerArrangementPatternViewerAndMixer({ });
    {
        const ContainerArrangement containerArrangementPv(Dimension({ }), true, { PanelType::patternViewer },
            { }, focusOrder);
        const ContainerArrangement containerArrangementMixer(Dimension(mixerBesidesPvWidth), true, { PanelType::meters });
        lineContainerArrangementPatternViewerAndMixer.addContainerArrangement(containerArrangementMixer);
        lineContainerArrangementPatternViewerAndMixer.addContainerArrangement(containerArrangementPv);
    }

    auto documentArrangement = std::make_unique<DocumentArrangement>();
    documentArrangement->addLineContainerArrangement(lineContainerArrangementLinker);
    documentArrangement->addLineContainerArrangement(lineContainerArrangementPatternViewerAndMixer);

    documentArrangement->setSelectedPanels(PanelType::patternViewer,
                                       {
                                               PanelType::meters, PanelType::linker, PanelType::patternViewer
                                       });
    return documentArrangement;
}

}   // namespace arkostracker

#include "PanelFactory.h"

#include "../../controllers/MainController.h"
#include "../arpeggioTableEditor/ArpeggioTableEditorPanel.h"
#include "../arpeggioTableList/ArpeggioTableListPanel.h"
#include "../containerArranger/DummyPanel.h"
#include "../instrumentEditor/InstrumentEditorPanel.h"
#include "../instrumentList/InstrumentListPanel.h"
#include "../linker/LinkerPanel.h"
#include "../meters/MetersPanel.h"
#include "../patternViewer/PatternViewerPanel.h"
#include "../pitchTableEditor/PitchTableEditorPanel.h"
#include "../pitchTableList/PitchTableListPanel.h"
#include "../testArea/TestAreaPanel.h"

namespace arkostracker 
{

PanelFactory::PanelFactory(MainController& pMainController, Panel::Listener& pListener) noexcept:
        mainController(pMainController),
        panelListener(pListener)
{
}

std::unique_ptr<Panel> PanelFactory::createPanel(PanelType panelType) const noexcept
{
    switch (panelType) {
        default:
            jassertfalse;       // Should never happen!!
        case PanelType::dummy:
            return std::make_unique<DummyPanel>(panelListener);
        case PanelType::meters:
            return std::make_unique<MetersPanel>(mainController, panelListener);
        case PanelType::patternViewer:
            return std::make_unique<PatternViewerPanel>(mainController, panelListener);
        case PanelType::linker:
            return std::make_unique<LinkerPanel>(mainController, panelListener);
        case PanelType::instrumentEditor:
            return std::make_unique<InstrumentEditorPanel>(mainController, panelListener);
        case PanelType::instrumentList:
            return std::make_unique<InstrumentListPanel>(mainController, panelListener);
        case PanelType::arpeggioTable:
            return std::make_unique<ArpeggioTableEditorPanel>(mainController, panelListener);
        case PanelType::arpeggioList:
            return std::make_unique<ArpeggioTableListPanel>(mainController, panelListener);
        case PanelType::pitchTable:
            return std::make_unique<PitchTableEditorPanel>(mainController, panelListener);
        case PanelType::pitchList:
            return std::make_unique<PitchTableListPanel>(mainController, panelListener);
        case PanelType::testArea:
            return std::make_unique<TestAreaPanel>(mainController, panelListener);
    }
}

}   // namespace arkostracker

#include "PitchTableEditorPanel.h"

#include "../../controllers/MainController.h"
#include "controller/PitchTableEditorControllerImpl.h"

namespace arkostracker 
{

PitchTableEditorPanel::PitchTableEditorPanel(MainController& mainController, Panel::Listener& pListener) noexcept :
        Panel(pListener),
        pitchTableEditorController(mainController.getPitchTableEditorController())
{
    pitchTableEditorController.onParentViewCreated(*this);
}

PitchTableEditorPanel::~PitchTableEditorPanel()
{
    pitchTableEditorController.onParentViewDeleted();
}

PanelType PitchTableEditorPanel::getType() const noexcept
{
    return PanelType::pitchTable;
}

void PitchTableEditorPanel::resized()
{
    pitchTableEditorController.onParentViewResized(getXInsideComponent(), getYInsideComponent(), getAvailableWidthInComponent(), getAvailableHeightInComponent());
}

void PitchTableEditorPanel::getKeyboardFocus() noexcept
{
    pitchTableEditorController.getKeyboardFocus();
}


}   // namespace arkostracker


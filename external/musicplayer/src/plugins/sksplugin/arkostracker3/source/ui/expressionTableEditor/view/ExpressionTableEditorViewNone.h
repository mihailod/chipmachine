#pragma once

#include "../../editorWithBars/view/EditorWithBarsViewNoOp.h"

namespace arkostracker 
{

/** View in the Expression Table Editor, when none is selected (or 0 is selected). Only shows a label. */
class ExpressionTableEditorViewNone : public EditorWithBarsViewNoOp
{
public:
    /**
     * Constructor.
     * @param isArpeggio true if arpeggio, false if pitch.
     */
    explicit ExpressionTableEditorViewNone(bool isArpeggio) noexcept;

    void resized() override;

private:
    bool isArpeggio;
    juce::Label label;
};

}   // namespace arkostracker

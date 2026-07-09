#include "BarAreaHolder.h"

#include "../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

const float BarAreaHolder::dashLengths[] = { 8.0F, 8.0F };             // NOLINT(*-avoid-c-arrays)
const float BarAreaHolder::barAlpha = 0.4F;

BarAreaHolder::BarAreaHolder() noexcept :
        startBarX(-1),      // Security for init.
        endBarX(0),
        isLooping(false),
        barY(0),
        barColor()
{
}

void BarAreaHolder::setLoop(const int pStartBarX, const int pEndBarX, const bool pIsLooping, const int pBarY) noexcept
{
    if ((startBarX == pStartBarX) && (endBarX == pEndBarX) && (isLooping == pIsLooping) && (barY != pBarY)) {
        return;
    }

    startBarX = pStartBarX;
    endBarX = pEndBarX;
    isLooping = pIsLooping;
    barY = pBarY;

    repaint();
}

// juce::Component method implementations.
// ============================================

void BarAreaHolder::paintOverChildren(juce::Graphics& g)
{
    // Nothing initialized? Then exits.
    if (startBarX < 0) {
        return;
    }

    // Gets the color only if not in cache.
    if (barColor.isAbsent()) {
        barColor = juce::LookAndFeel::getDefaultLookAndFeel()
        .findColour(static_cast<int>(LookAndFeelConstants::Colors::loopStartEnd))
        .withAlpha(barAlpha);
    }

    const auto barYFloat = static_cast<float>(barY);
    const auto height = static_cast<float>(getHeight()) - barYFloat;

    g.setColour(barColor.getValueRef());

    drawLine(g, static_cast<float>(endBarX), barYFloat, height);
    if (isLooping) {
        drawLine(g, static_cast<float>(startBarX), barYFloat, height);
    }
}

void BarAreaHolder::lookAndFeelChanged()
{
    barColor = { };
}

void BarAreaHolder::drawLine(const juce::Graphics& g, const float x, const float y, const float endY) noexcept
{
    g.drawDashedLine(juce::Line(x, y, x, endY), &dashLengths[0], 2);
}

}       // namespace arkostracker
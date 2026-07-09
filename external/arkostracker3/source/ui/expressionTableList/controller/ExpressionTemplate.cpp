#include "ExpressionTemplate.h"

namespace arkostracker
{

std::unique_ptr<Expression> ExpressionTemplate::getTemplate(const Template desiredTemplate) noexcept
{
    switch (desiredTemplate) {
        default:
            jassertfalse;           // Shouldn't happen.
        case Template::emptyArpeggio: [[fallthrough]];
        case Template::emptyPitch:
            return std::make_unique<Expression>((desiredTemplate == Template::emptyArpeggio), juce::String(), true);

        case Template::major: {
            auto expression = std::make_unique<Expression>();
            expression->addValue(4);
            expression->addValue(7);
            expression->setEnd(2);
            return expression;
        }
        case Template::minor: {
            auto expression = std::make_unique<Expression>();
            expression->addValue(3);
            expression->addValue(7);
            expression->setEnd(2);
            return expression;
        }
        case Template::octaveAttack: {
            auto expression = std::make_unique<Expression>(true, juce::String(), false);
            expression->addValue(12);
            expression->addValue(0);
            expression->setLoopStartAndEnd(1, 1);
            return expression;
        }

        case Template::vibrato: {
            auto expression = std::make_unique<Expression>(false, juce::String(), false);
            expression->addValue(0);
            expression->addValue(1);
            expression->addValue(2);
            expression->addValue(1);
            expression->addValue(0);
            expression->addValue(-1);
            expression->addValue(-2);
            expression->addValue(-1);
            expression->setEnd(7);
            return expression;
        }
        case Template::strongVibrato: {
            auto expression = std::make_unique<Expression>(false, juce::String(), false);
            expression->addValue(0);
            expression->addValue(1);
            expression->addValue(2);
            expression->addValue(3);
            expression->addValue(2);
            expression->addValue(1);
            expression->addValue(0);
            expression->addValue(-1);
            expression->addValue(-2);
            expression->addValue(-3);
            expression->addValue(-2);
            expression->addValue(-1);
            expression->setEnd(11);
            return expression;
        }
        case Template::lowAttack: {
            auto expression = std::make_unique<Expression>(false, juce::String(), false);
            expression->addValue(-40);
            expression->addValue(-30);
            expression->addValue(-20);
            expression->addValue(-10);
            expression->addValue(0);
            expression->setLoopStartAndEnd(4, 4);
            return expression;
        }
    }
}

std::map<ExpressionTemplate::Template, juce::String> ExpressionTemplate::getTemplateNames(const bool isArpeggio) noexcept
{
    static const std::map<Template, juce::String> templateToArpeggioNames = {
        { Template::emptyArpeggio, juce::translate("Empty") },
        { Template::major, juce::translate("Major chord") },
        { Template::minor, juce::translate("Minor chord") },
        { Template::octaveAttack, juce::translate("Octave attack") },
    };

    static const std::map<Template, juce::String> templateToPitchNames = {
        { Template::emptyPitch, juce::translate("Empty") },
        { Template::vibrato, juce::translate("Vibrato") },
        { Template::strongVibrato, juce::translate("Strong vibrato") },
        { Template::lowAttack, juce::translate("Low attack") },
    };

    return isArpeggio ? templateToArpeggioNames : templateToPitchNames;
}

}   // namespace arkostracker

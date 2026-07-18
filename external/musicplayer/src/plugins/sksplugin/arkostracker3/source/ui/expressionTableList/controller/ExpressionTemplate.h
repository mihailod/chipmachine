#pragma once

#include <memory>

#include "../../../song/Expression.h"

namespace arkostracker
{

/** Helper to provide templates of Expressions. */
class ExpressionTemplate
{
public:
    enum class Template : unsigned char
    {
        // Arpeggios.
        emptyArpeggio,
        major,
        minor,
        octaveAttack,

        // Pitchs.
        emptyPitch,
        vibrato,
        strongVibrato,
        lowAttack,
    };

    /** Prevents instantiation. */
    ExpressionTemplate() = delete;

    /**
     * @return a template expression.
     * \param desiredTemplate the template.
     */
    static std::unique_ptr<Expression> getTemplate(Template desiredTemplate) noexcept;

    /**
     * @return the template linked with their name.
     * \param isArpeggio true to get the arpeggios, false the pitch.
     */
    static std::map<Template, juce::String> getTemplateNames(bool isArpeggio) noexcept;
};

}   // namespace arkostracker

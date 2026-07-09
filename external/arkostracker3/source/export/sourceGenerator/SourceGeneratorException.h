#pragma once

#include <stdexcept>

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** An exception that can be thrown during the source generation. */
class SourceGeneratorException : public std::runtime_error
{
public:
    explicit SourceGeneratorException(const juce::String& message) :
        std::runtime_error(message.toStdString())
    {
    }
};


}   // namespace arkostracker


#include "Expression.h"

namespace arkostracker 
{

const int Expression::emptyValue = 0;

Expression::Expression(const bool pIsArpeggio, juce::String pName, const bool pCreateOneElement) noexcept :
        id(),
        arpeggio(pIsArpeggio),
        name(std::move(pName)),
        loopStartIndex(0),
        endIndex(0),
        speed(0),
        shift(0),
        items()
{
    // Creates a default value if wanted.
    if (pCreateOneElement) {
        items.push_back(emptyValue);
    }
}

Expression Expression::buildArpeggio(const juce::String& name, const bool createOneElement) noexcept
{
    return Expression(true, name, createOneElement);
}

Expression Expression::buildPitch(const juce::String& name, const bool createOneElement) noexcept
{
    return Expression(false, name, createOneElement);
}

Expression Expression::build(const juce::String& name, const bool isArpeggio, const std::vector<int>& items, const int speed) noexcept
{
    Expression expression(isArpeggio, name, false);
    expression.items = items;
    expression.speed = speed;
    expression.endIndex = static_cast<int>(items.size() - 1);

    return expression;
}

Id Expression::getId() const noexcept
{
    return id;
}

const juce::String& Expression::getName() const noexcept
{
    return name;
}

void Expression::setName(const juce::String& newName) noexcept
{
    name = newName;
}

int Expression::getLength(const bool withShift) const noexcept
{
    return static_cast<int>(items.size()) + (withShift ? shift : 0);
}

int Expression::getLoopStart(const bool withShift) const noexcept
{
    return loopStartIndex + (withShift ? shift : 0);
}

int Expression::getEnd(const bool withShift) const noexcept
{
    return endIndex + (withShift ? shift : 0);
}

int Expression::getSpeed() const noexcept
{
    return speed;
}

int Expression::getShift() const noexcept
{
    return shift;
}

int Expression::getValue(const int index, const bool withShift) const noexcept
{
    auto finalIndex = index;

    if (withShift) {
        if (index < shift) {
            return emptyValue;
        }
        finalIndex = index - shift;
    }

    // Still within index? If not, returns 0.
    return (finalIndex < getLength()) ? items.at(static_cast<size_t>(finalIndex)) : emptyValue;
}

void Expression::setValue(const int index, const int value) noexcept
{
    jassert(index < getLength());
    items.at(static_cast<size_t>(index)) = value;
}

void Expression::addValue(const int value) noexcept
{
    items.push_back(value);
}

void Expression::insertValue(const int index, const int value) noexcept
{
    items.insert(items.begin() + index, value);
}

void Expression::removeValues(const int startIndex, const int valueCount) noexcept
{
    items.erase(items.begin() + startIndex, items.begin() + startIndex + valueCount);
}

void Expression::setLoopStartAndEnd(const int loopStart, const int end) noexcept
{
    jassert(loopStart <= end);
    jassert(end < getLength());

    loopStartIndex = loopStart;
    endIndex = end;
}

void Expression::setEnd(const int end) noexcept
{
    jassert(end < getLength());

    endIndex = end;
}

void Expression::setSpeed(const int newSpeed) noexcept
{
    jassert((newSpeed >= 0) && (newSpeed < 256));
    speed = newSpeed;
}

void Expression::setShift(const int newShift) noexcept
{
    shift = newShift;
}

bool Expression::isArpeggio() const noexcept
{
    return arpeggio;
}

Loop Expression::getLoop(const bool withShift) const noexcept
{
    return Loop(getLoopStart(withShift), getEnd(withShift), true);     // An expression is always looping.
}

void Expression::setLoop(const Loop& newLoop) noexcept
{
    jassert(newLoop.isLooping());           // Expression always loop!

    setLoopStartAndEnd(newLoop.getStartIndex(), newLoop.getEndIndex());
}

void Expression::clear(const bool createOneElement) noexcept
{
    items.clear();
    loopStartIndex = 0;
    endIndex = 0;
    speed = 0;
    name = juce::String();

    if (createOneElement) {
        items.push_back(emptyValue);
    }
}

bool Expression::operator==(const Expression& other) const
{
    return (name == other.name) && isMusicallyEqualTo(other);
}

bool Expression::operator!=(const Expression& rhs) const
{
    return !(rhs == *this);
}

bool Expression::isMusicallyEqualTo(const Expression& other) const noexcept
{
    return (arpeggio == other.arpeggio)
           && (loopStartIndex == other.loopStartIndex)
           && (endIndex == other.endIndex)
           && (speed == other.speed)
           && (shift == other.shift)
           && (items == other.items);
}

bool Expression::hasOnlyZeros() const noexcept
{
    return std::all_of(items.cbegin(), items.cend(), [](const int item) {
        return (item == emptyValue);
    });
}

}   // namespace arkostracker

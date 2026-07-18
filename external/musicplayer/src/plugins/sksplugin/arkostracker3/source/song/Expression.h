#pragma once

#include <vector>

#include <juce_core/juce_core.h>

#include "../business/model/Loop.h"
#include "../utils/Id.h"

namespace arkostracker 
{

/** Class for an Expression (Arpeggio, Pitch). A 'shift'... shifts all the data. Internally, the data never take this in account. */
class Expression            // TODO TU this.
{
public:
    /**
     * Constructor. Default parameters are provided for map insertions.
     * @param isArpeggio true if Arpeggio, false if Pitch. Only a convenience for clients (for checks, etc.).
     * @param name the name, displayed to the user.
     * @param createOneElement true to create one default item. If false, the expression is invalid because it must hold one item!
     */
    explicit Expression(bool isArpeggio = true, juce::String name = juce::String(), bool createOneElement = true) noexcept;

    /**
     * Builds an Arpeggio expression.
     * @param name the name, displayed to the user.
     * @param createOneElement true to create one default item. If false, the expression is invalid because it must hold one item!
     * @return the Expression.
     */
    static Expression buildArpeggio(const juce::String& name, bool createOneElement) noexcept;

    /**
     * Conveniently builds an expression from the given values. The loop is set on all the values.
     * @param name the name, displayed to the user.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param items the items.
     * @param speed the speed.
     * @return the Expression.
     */
    static Expression build(const juce::String& name, bool isArpeggio, const std::vector<int>& items, int speed = 0) noexcept;

    /**
     * Builds a Pitch expression.
     * @param name the name, displayed to the user.
     * @param createOneElement true to create one default item. If false, the expression is invalid because it must hold one item!
     * @return the Expression.
     */
    static Expression buildPitch(const juce::String& name, bool createOneElement) noexcept;

    /** @return the unique ID of the Expression. Do not serialize it. */
    Id getId() const noexcept;

    /** @return the name of the expression. */
    const juce::String& getName() const noexcept;

    /**
     * Sets the name.
     * @param newName the new name.
     */
    void setName(const juce::String& newName) noexcept;

    /**
     * Sets the loop start and end.
     * @param loopStart the loop start. Must be valid.
     * @param end the end. Must be valid.
     */
    void setLoopStartAndEnd(int loopStart, int end) noexcept;

    /**
     * Sets the end (not the same as the length!).
     * @param end the end. Must be valid.
     */
    void setEnd(int end) noexcept;

    /**
     * Sets the speed.
     * @param speed the speed (>=0, <256). Must be valid.
     */
    void setSpeed(int speed) noexcept;

    /**
     * Sets the shift.
     * @param shift the shift (>=0). Must be valid.
     */
    void setShift(int shift) noexcept;

    /**
     * @return how many items there are.
     * @param withShift true to include the shift in the result.
     */
    int getLength(bool withShift = false) const noexcept;
    /**
     * @return the loop start index.
     * @param withShift true to include the shift in the result.
     */
    int getLoopStart(bool withShift = false) const noexcept;
    /**
     * @return the end index. This may be smaller to the length.
     * @param withShift true to include the shift in the result.
     */
    int getEnd(bool withShift = false) const noexcept;
    /** @return the speed, from 0 (fastest) to 255 (slowest). */
    int getSpeed() const noexcept;
    /** @return the shift (>=0). */
    int getShift() const noexcept;

    /** @return the loop, built as a convenience. */
    Loop getLoop(bool withShift = false) const noexcept;
    /**
     * Sets the loop. If non-looping, asserts but sets a loop anyway.
     * This does NOT take the shift in account, only the real backing data.
     * @param loop the new loop.
     */
    void setLoop(const Loop& loop) noexcept;

    /**
     * @return a value. This call is safe: if out of bounds, 0 is returned. This is convenient to the UI and the player.
     * @param index the index. Must be valid.
     * @param withShift true to include the shift in the result.
     */
    int getValue(int index, bool withShift = false) const noexcept;

    /**
     * Sets a value. This does NOT take the shift in account, only the real backing data.
     * @param index the index. Must be valid.
     * @param value the value.
     */
    void setValue(int index, int value) noexcept;

    /**
     * Adds a value at the end of the items.
     * @param value the value.
     */
    void addValue(int value) noexcept;

    /**
     * Inserts a value. This does NOT take the shift in account, only the real backing data.
     * This does not update the loop.
     * @param index where to insert. Must be valid.
     * @param value the value.
     */
    void insertValue(int index, int value) noexcept;

    /**
     * Removes values. This does NOT take the shift in account, only the real backing data.
     * @param startIndex the first index where to start removing. Must be valid.
     * @param valueCount how many values to remove. Must be valid.
     */
    void removeValues(int startIndex, int valueCount) noexcept;

    /** @return true if Arpeggio, false if Pitch. */
    bool isArpeggio() const noexcept;

    /**
     * Clears all the values, including the name and speed. Warning, if no element is then created, the arpeggio is unusable!
     * @param createOneElement if true, an empty element is created, making the object usable.
     */
    void clear(bool createOneElement = false) noexcept;

    /** Compares everything, including the name. */
    bool operator==(const Expression& rhs) const;
    bool operator!=(const Expression& rhs) const;

    /** @return true if musically equal (name not taken in account). */
    bool isMusicallyEqualTo(const Expression& other) const noexcept;

    /** @return true if the expression only consists of 'zero' values. */
    bool hasOnlyZeros() const noexcept;

private:
    static const int emptyValue;

    Id id;                                  // Uniquely identify the Expression for this session. Not serialized.

    bool arpeggio;                          // True if Arpeggio, false if Pitch.
    juce::String name;                      // The name of the item.
    int loopStartIndex;                     // Index of the start of the loop, WITHOUT the shift taken in account.
    int endIndex;                           // Index of the end, WITHOUT the shift taken in account.

    int speed;                              // The speed, from 0 (fastest) to 255 (slowest).

    int shift;                              // 0s to add at the beginning.

    std::vector<int> items;                 // The stored elements.

    JUCE_LEAK_DETECTOR (Expression)
};

}   // namespace arkostracker

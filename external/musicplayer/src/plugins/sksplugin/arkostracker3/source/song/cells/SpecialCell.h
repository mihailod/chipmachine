#pragma once

namespace arkostracker 
{

/**
 * A SpecialCell. It can be empty (0), or not.
 * Use the static constructor. However, there is a default constructor, required for various templates.
 */
class SpecialCell
{
public:
    /** Constructor of an empty SpecialCell. */
    SpecialCell() noexcept;

    /** Builds an empty Special Cell. */
    static SpecialCell buildEmptySpecialCell() noexcept;

    /** Builds a Special Cell with a value, 0 being empty. */
    static SpecialCell buildSpecialCell(int value) noexcept;

    /** Indicates whether the SpecialCell is empty or not. */
    bool isEmpty() const noexcept;

    /** Returns the value, from 0 to 255, or 0 if empty. */
    int getValue() const noexcept;

    /**
     * Mixes the current SpecialCell with a new one, which overwrites only the non-empty fields it has.
     * @param specialCell the cell which overwrites the current one.
     * @return the mixed SpecialCell.
     */
    SpecialCell mix(const SpecialCell& specialCell) const noexcept;

    /** Clears the data of this SpecialCell. */
    void clear() noexcept;

    /** Equality operator. */
    bool operator==(const SpecialCell&) const;

    /** Non-equality operator. */
    bool operator!=(const SpecialCell&) const;

private:
    /**
        Builds a SpecialCell.
        @param value the value, valid or not.
    */
    explicit SpecialCell(int value) noexcept;

    int value;            // The value.
};

}   // namespace arkostracker

#include "FpFloat.h"

namespace arkostracker 
{

FpFloat::FpFloat(const uint16_t pIntegerPart, const int pDecimalPart) :
    integerPart(pIntegerPart),
    decimalPart(static_cast<int>(static_cast<unsigned int>(pDecimalPart) & 0xffU))
{
}

void FpFloat::setValues(const uint16_t newIntegerPart, const int newDecimalPart) noexcept
{
    integerPart = newIntegerPart;
    decimalPart = static_cast<int>(static_cast<unsigned int>(newDecimalPart) & 0xffU);
}

void FpFloat::setFromDigits(const int digits) noexcept
{
    const auto localIntegerPart = static_cast<uint16_t>((static_cast<unsigned int>(digits) >> 8U) & 0xffU);
    const auto localDecimalPart = static_cast<int>(static_cast<unsigned int>(digits) & 0xffU);
    setValues(localIntegerPart, localDecimalPart);
}

uint16_t FpFloat::getIntegerPart() const noexcept
{
    return integerPart;
}

int FpFloat::getDecimalPart() const noexcept
{
    return decimalPart;
}

void FpFloat::setDecimalPart(const int newDecimalPart) noexcept
{
    decimalPart = static_cast<int>(static_cast<unsigned int>(newDecimalPart) & 0xffU);
}

void FpFloat::reset() noexcept
{
    integerPart = 0U;
    decimalPart = 0;
}

bool FpFloat::isNegative() const noexcept
{
    return ((integerPart & 0x8000U) != 0U);       // Is bit 15 to 1? If yes, negative.
}

FpFloat& FpFloat::operator=(const uint16_t& other) noexcept
{
    integerPart = other;
    decimalPart = 0;
    return *this;
}

bool FpFloat::operator==(const FpFloat& other) const noexcept
{
    return ((static_cast<unsigned int>(integerPart) == other.integerPart) && (decimalPart == other.decimalPart));
}

bool FpFloat::operator!=(const FpFloat& other) const noexcept
{
    return !operator==(other);
}

FpFloat FpFloat::operator+(const FpFloat& other) const noexcept
{
    auto firstInteger = integerPart;
    auto firstDecimal = decimalPart;

    addition(firstInteger, firstDecimal, other.integerPart, other.decimalPart);
    return FpFloat(firstInteger, firstDecimal);
}

FpFloat FpFloat::operator-(const FpFloat& other) const noexcept
{
    auto firstInteger = integerPart;
    auto firstDecimal = decimalPart;

    subtraction(firstInteger, firstDecimal, other.integerPart, other.decimalPart);
    return FpFloat(firstInteger, firstDecimal);
}

FpFloat& FpFloat::operator+=(const FpFloat& other) noexcept
{
    addition(integerPart, decimalPart, other.integerPart, other.decimalPart);
    return *this;
}

FpFloat& FpFloat::operator-=(const FpFloat& other) noexcept
{
    subtraction(integerPart, decimalPart, other.integerPart, other.decimalPart);
    return *this;
}

void FpFloat::addition(uint16_t& firstInteger, int& firstDecimal, const uint16_t secondInteger, const int secondDecimal) noexcept
{
    // Raw additions first.
    auto newIntegerPart = static_cast<uint16_t>(static_cast<unsigned int>(firstInteger) + secondInteger);   // Will loop automatically, this is what we want.
    auto newDecimalPart = firstDecimal + secondDecimal;

    // Corrects the decimal, and returns the number to add the integer part in case of overflow.
    const auto offset = correctDecimalPartReturnOffset(newDecimalPart);
    newIntegerPart += static_cast<unsigned int>(offset);

    firstInteger = newIntegerPart;
    firstDecimal = newDecimalPart;
}

void FpFloat::subtraction(uint16_t& firstInteger, int& firstDecimal, const uint16_t secondInteger, const int secondDecimal) noexcept
{
    // Raw subtraction first.
    auto newIntegerPart = static_cast<uint16_t>(static_cast<unsigned int>(firstInteger) - secondInteger);   // Will loop automatically, this is what we want.
    auto newDecimalPart = firstDecimal - secondDecimal;

    // Corrects the decimal, and returns the number to add the integer part in case of overflow.
    const auto offset = correctDecimalPartReturnOffset(newDecimalPart);
    newIntegerPart += static_cast<unsigned int>(offset);

    firstInteger = newIntegerPart;
    firstDecimal = newDecimalPart;
}

void FpFloat::negate() noexcept
{
    uint16_t newIntegerPart = 0U;
    auto newDecimalPart = 0;
    // 0 - number.
    subtraction(newIntegerPart, newDecimalPart, integerPart, decimalPart);

    integerPart = newIntegerPart;
    decimalPart = newDecimalPart;
}

uint16_t FpFloat::correctDecimalPartReturnOffset(int& decimalPartToModify) noexcept
{
    const auto newDecimalPart = static_cast<unsigned int>(decimalPartToModify) & 0xffU;
    const auto offset = static_cast<unsigned int>(decimalPartToModify) / 0x100U;

    decimalPartToModify = static_cast<int>(newDecimalPart);

    return static_cast<uint16_t>(offset);
}

void FpFloat::correctFrom0To(const unsigned int maximumIntegerPart) noexcept
{
    if (isNegative()) {
        setValues(0U);
    } else if (const auto volume = getIntegerPart(); volume > maximumIntegerPart) {
        setValues(static_cast<uint16_t>(maximumIntegerPart));
    }
}

FpFloat operator+(const uint16_t lhs, const FpFloat& rhs) noexcept
{
    return FpFloat(lhs) + rhs;
}

FpFloat operator+(const FpFloat& lhs, const uint16_t rhs) noexcept
{
    return FpFloat(rhs) + lhs;
}

FpFloat operator-(const uint16_t lhs, const FpFloat& rhs) noexcept
{
    return FpFloat(lhs).operator-(rhs);
}

FpFloat operator-(const FpFloat& lhs, const uint16_t rhs) noexcept
{
    return lhs.operator-(FpFloat(rhs));
}

FpFloat FpFloat::operator*(const int multiplier) const noexcept
{
    const auto number = ((integerPart * 256U) + (static_cast<unsigned int>(decimalPart) & 0xffU)) & 0xffffffU;
    const auto multipliedNumber = static_cast<unsigned int>(static_cast<int>(number) * multiplier);
    const FpFloat result(static_cast<uint16_t>(multipliedNumber >> 8U), static_cast<int>(multipliedNumber & 0xffU));
    return result;
}

}   // namespace arkostracker

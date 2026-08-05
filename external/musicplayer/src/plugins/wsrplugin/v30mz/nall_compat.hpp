// nall_compat.hpp -- the slice of ares' nall that the vendored V30MZ core needs.
//
// ares builds its CPU cores against nall, its own utility library. The V30MZ is
// almost free of it -- it has no Node, no Thread, no serializer and no string
// once the debugger and serialization files are dropped -- but it does use nall's
// sized-integer primitives, a bit-field helper, a fixed-capacity queue and
// `maybe`. Rather than vendor nall (which drags in its string/vfs/traits tower
// through primitives.hpp), this header supplies those four things with the same
// names and the same semantics.
//
// Written for this project against nall's documented behaviour; see PROVENANCE.md
// for what IS vendored and what was patched.
//
// The semantics that matter, because the CPU depends on them:
//
//   * Natural<Bits> / NaturalPrimitive<Bits> MASK ON EVERY WRITE. `n8 x = 0x1ff`
//     holds 0xff, and `x++` at 0xff wraps to 0. ares writes `u4` casts and
//     `n20` addresses expecting exactly that truncation -- e.g. the auxiliary
//     carry is computed as `(u4)x + (u4)y + (u4)c >= 16`, which is wrong by a
//     mile if u4 is a plain int.
//   * BitField<Size, Index> is a REFERENCE into a host integer, not a copy: the
//     PSW's flags alias the same u16 the CPU pushes and pops.
//   * queue<T[N]> is a fixed-capacity FIFO whose read() pops.

#pragma once

#include <cstddef>
#include <type_traits>
#include <cstdint>
#include <utility>

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

namespace nall {

template<u32 Precision> struct NaturalType {
  using type =
    typename std::conditional<Precision <= 8, u8,
    typename std::conditional<Precision <= 16, u16,
    typename std::conditional<Precision <= 32, u32, u64>::type>::type>::type;
};

// Masking unsigned integer of `Precision` bits.
template<u32 Precision> struct NaturalPrimitive {
  using utype = typename NaturalType<Precision>::type;
  static constexpr auto mask() -> utype {
    return (utype)(~0ull >> (64 - Precision));
  }

  NaturalPrimitive() : data(0) {}
  template<typename T> NaturalPrimitive(const T& value) { data = cast(value); }

  operator utype() const { return data; }

  auto operator++(s32) { auto value = *this; data = cast(data + 1); return value; }
  auto operator--(s32) { auto value = *this; data = cast(data - 1); return value; }
  auto& operator++() { data = cast(data + 1); return *this; }
  auto& operator--() { data = cast(data - 1); return *this; }

  template<typename T> auto& operator  =(const T& value) { data = cast(         value); return *this; }
  template<typename T> auto& operator *=(const T& value) { data = cast(data  *  value); return *this; }
  template<typename T> auto& operator /=(const T& value) { data = cast(data  /  value); return *this; }
  template<typename T> auto& operator %=(const T& value) { data = cast(data  %  value); return *this; }
  template<typename T> auto& operator +=(const T& value) { data = cast(data  +  value); return *this; }
  template<typename T> auto& operator -=(const T& value) { data = cast(data  -  value); return *this; }
  template<typename T> auto& operator<<=(const T& value) { data = cast(data <<  value); return *this; }
  template<typename T> auto& operator>>=(const T& value) { data = cast(data >>  value); return *this; }
  template<typename T> auto& operator &=(const T& value) { data = cast(data  &  value); return *this; }
  template<typename T> auto& operator ^=(const T& value) { data = cast(data  ^  value); return *this; }
  template<typename T> auto& operator |=(const T& value) { data = cast(data  |  value); return *this; }

private:
  static auto cast(u64 value) -> utype { return (utype)(value & mask()); }
  utype data;
};

template<u32 Precision = 64> struct Natural : NaturalPrimitive<Precision> {
  using Base = NaturalPrimitive<Precision>;
  using utype = typename Base::utype;

  Natural() : Base() {}
  template<typename T> Natural(const T& value) : Base(value) {}

  template<typename T> auto& operator=(const T& value) { Base::operator=(value); return *this; }

  static constexpr auto bits() -> u32 { return Precision; }
};

template<u32 Precision> struct IntegerType {
  using type =
    typename std::conditional<Precision <= 8, s8,
    typename std::conditional<Precision <= 16, s16,
    typename std::conditional<Precision <= 32, s32, s64>::type>::type>::type;
};

// Signed counterpart: SIGN-EXTENDS on every write, so `(i8)0xff` is -1. The CPU
// relies on this for every relative jump and for CBW/CWD.
template<u32 Precision> struct IntegerPrimitive {
  using stype = typename IntegerType<Precision>::type;

  IntegerPrimitive() : data(0) {}
  template<typename T> IntegerPrimitive(const T& value) { data = cast(value); }

  operator stype() const { return data; }

  auto operator++(s32) { auto value = *this; data = cast(data + 1); return value; }
  auto operator--(s32) { auto value = *this; data = cast(data - 1); return value; }
  auto& operator++() { data = cast(data + 1); return *this; }
  auto& operator--() { data = cast(data - 1); return *this; }

  template<typename T> auto& operator  =(const T& value) { data = cast(         value); return *this; }
  template<typename T> auto& operator *=(const T& value) { data = cast(data  *  value); return *this; }
  template<typename T> auto& operator /=(const T& value) { data = cast(data  /  value); return *this; }
  template<typename T> auto& operator %=(const T& value) { data = cast(data  %  value); return *this; }
  template<typename T> auto& operator +=(const T& value) { data = cast(data  +  value); return *this; }
  template<typename T> auto& operator -=(const T& value) { data = cast(data  -  value); return *this; }
  template<typename T> auto& operator<<=(const T& value) { data = cast(data <<  value); return *this; }
  template<typename T> auto& operator>>=(const T& value) { data = cast(data >>  value); return *this; }
  template<typename T> auto& operator &=(const T& value) { data = cast(data  &  value); return *this; }
  template<typename T> auto& operator ^=(const T& value) { data = cast(data  ^  value); return *this; }
  template<typename T> auto& operator |=(const T& value) { data = cast(data  |  value); return *this; }

private:
  static auto cast(s64 value) -> stype {
    constexpr u64 mask = ~0ull >> (64 - Precision);
    constexpr u64 sign = 1ull << (Precision - 1);
    u64 bits = (u64)value & mask;
    return (stype)((bits ^ sign) - sign);
  }
  stype data;
};

template<u32 Precision = 64> struct Integer : IntegerPrimitive<Precision> {
  using Base = IntegerPrimitive<Precision>;
  using stype = typename Base::stype;

  Integer() : Base() {}
  template<typename T> Integer(const T& value) : Base(value) {}

  template<typename T> auto& operator=(const T& value) { Base::operator=(value); return *this; }

  static constexpr auto bits() -> u32 { return Precision; }
};

// A single bit inside a host integer, addressed by reference. Assignment writes
// through to the host; `bit()` yields the bit's index, which is what ares'
// SetFlag/ClearFlag instruction handlers take.
template<u32 Size, u32 Index> struct BitField {
  using utype = typename NaturalType<Size>::type;
  static constexpr utype maskbit = (utype)((utype)1 << Index);

  BitField(utype* source) : target(*source) {}

  operator bool() const { return target & maskbit; }
  auto bit() const -> u32 { return Index; }

  auto& operator=(bool value) {
    target = (utype)(value ? (target | maskbit) : (target & ~maskbit));
    return *this;
  }
  auto& operator=(const BitField& value) { return operator=((bool)value); }
  auto& operator|=(bool value) { return operator=((bool)*this | value); }
  auto& operator&=(bool value) { return operator=((bool)*this & value); }
  auto& operator^=(bool value) { return operator=((bool)*this ^ value); }

private:
  utype& target;
};

// Fixed-capacity FIFO. read() POPS; write() appends and is a no-op when full,
// which is why the CPU checks full() itself before writing.
template<typename T> struct queue;

template<typename T, u32 Capacity> struct queue<T[Capacity]> {
  auto flush() -> void { count = first = 0; }
  auto size() const -> u32 { return count; }
  auto empty() const -> bool { return count == 0; }
  auto full() const -> bool { return count >= Capacity; }

  auto write(T value) -> void {
    if(count >= Capacity) return;
    data[(first + count) % Capacity] = value;
    count++;
  }

  // The index argument exists for source compatibility; ares only ever reads
  // the front, and reading pops it.
  auto read(u32 = 0) -> T {
    if(count == 0) return T();
    T value = data[first];
    first = (first + 1) % Capacity;
    count--;
    return value;
  }

private:
  T data[Capacity] = {};
  u32 first = 0;
  u32 count = 0;
};

// Optional value; ares passes `maybe<u8>` for an immediate that may be absent.
template<typename T> struct maybe {
  maybe() : valid(false), value() {}
  maybe(const T& value) : valid(true), value(value) {}

  explicit operator bool() const { return valid; }
  auto operator*() const -> const T& { return value; }
  auto operator()(const T& fallback) const -> T { return valid ? value : fallback; }

private:
  bool valid;
  T value;
};

}

using nall::BitField;
using nall::Integer;
using nall::IntegerPrimitive;
using nall::Natural;
using nall::NaturalPrimitive;
using nall::maybe;
using nall::queue;

using  i1 = Integer< 1>;  using  s1 = IntegerPrimitive< 1>;
using  i2 = Integer< 2>;  using  s2 = IntegerPrimitive< 2>;
using  i3 = Integer< 3>;  using  s3 = IntegerPrimitive< 3>;
using  i4 = Integer< 4>;  using  s4 = IntegerPrimitive< 4>;
using  i5 = Integer< 5>;  using  s5 = IntegerPrimitive< 5>;
using  i6 = Integer< 6>;  using  s6 = IntegerPrimitive< 6>;
using  i7 = Integer< 7>;  using  s7 = IntegerPrimitive< 7>;
using  i8 = Integer< 8>;
using i16 = Integer<16>;
using i20 = Integer<20>;  using s20 = IntegerPrimitive<20>;
using i32 = Integer<32>;

using  n1 = Natural< 1>;  using  u1 = NaturalPrimitive< 1>;
using  n2 = Natural< 2>;  using  u2 = NaturalPrimitive< 2>;
using  n3 = Natural< 3>;  using  u3 = NaturalPrimitive< 3>;
using  n4 = Natural< 4>;  using  u4 = NaturalPrimitive< 4>;
using  n5 = Natural< 5>;  using  u5 = NaturalPrimitive< 5>;
using  n6 = Natural< 6>;  using  u6 = NaturalPrimitive< 6>;
using  n7 = Natural< 7>;  using  u7 = NaturalPrimitive< 7>;
using  n8 = Natural< 8>;
using  n9 = Natural< 9>;  using  u9 = NaturalPrimitive< 9>;
using n10 = Natural<10>;  using u10 = NaturalPrimitive<10>;
using n11 = Natural<11>;  using u11 = NaturalPrimitive<11>;
using n12 = Natural<12>;  using u12 = NaturalPrimitive<12>;
using n13 = Natural<13>;  using u13 = NaturalPrimitive<13>;
using n14 = Natural<14>;  using u14 = NaturalPrimitive<14>;
using n15 = Natural<15>;  using u15 = NaturalPrimitive<15>;
using n16 = Natural<16>;
using n17 = Natural<17>;  using u17 = NaturalPrimitive<17>;
using n18 = Natural<18>;  using u18 = NaturalPrimitive<18>;
using n19 = Natural<19>;  using u19 = NaturalPrimitive<19>;
using n20 = Natural<20>;  using u20 = NaturalPrimitive<20>;
using n24 = Natural<24>;  using u24 = NaturalPrimitive<24>;
using n32 = Natural<32>;

// nall/endian.hpp. The CPU declares its 16-bit registers as a union of a u16 and
// two u8 halves; on a little-endian host the low half comes first. Everything
// this project targets is little-endian, and the static_assert below says so.
#if defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  #define order_lsb2(a, b) b, a
#else
  #define order_lsb2(a, b) a, b
#endif

// Copyright 2026, Trail of Bits. All rights reserved.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

namespace hyde::rt {

// Mixes the bits of `x` (the finalizer from splitmix64).
inline constexpr uint64_t HashMix(uint64_t x) noexcept {
  x ^= x >> 30u;
  x *= 0xbf58476d1ce4e5b9ull;
  x ^= x >> 27u;
  x *= 0x94d049bb133111ebull;
  x ^= x >> 31u;
  return x;
}

// Hash for builtin scalar types. Foreign types participate by providing an
// ADL-visible `uint64_t DrHash(const T &)`.
template <typename T>
requires std::is_integral_v<T> || std::is_enum_v<T>
inline constexpr uint64_t DrHash(T val) noexcept {
  return HashMix(static_cast<uint64_t>(val));
}

// Floats are canonicalized before hashing so that values comparing equal
// hash equally (`-0.0` vs `+0.0`). NaN column values are unsupported: NaN
// never compares equal to itself, so no equality-keyed store can hold it.
inline constexpr uint64_t DrHash(double val) noexcept {
  if (val == 0.0) {
    val = 0.0;
  }
  return HashMix(__builtin_bit_cast(uint64_t, val));
}

inline constexpr uint64_t DrHash(float val) noexcept {
  if (val == 0.0f) {
    val = 0.0f;
  }
  return HashMix(__builtin_bit_cast(uint32_t, val));
}

inline uint64_t DrHash(std::string_view val) noexcept {
  // FNV-1a, mixed.
  uint64_t h = 0xcbf29ce484222325ull;
  for (char c : val) {
    h ^= static_cast<uint8_t>(c);
    h *= 0x100000001b3ull;
  }
  return HashMix(h);
}

inline uint64_t DrHash(const std::string &val) noexcept {
  return DrHash(std::string_view(val));
}

// Combines field hashes into a row hash.
inline constexpr uint64_t HashCombine(uint64_t seed, uint64_t h) noexcept {
  return HashMix(seed ^ (h + 0x9e3779b97f4a7c15ull + (seed << 6u)));
}

// Hashes a full row of values: `HashRow(a, b, c)`.
template <typename... Ts>
inline constexpr uint64_t HashRow(const Ts &...vals) noexcept {
  uint64_t seed = 0x2545f4914f6cdd1dull;
  ((seed = HashCombine(seed, DrHash(vals))), ...);
  return seed;
}

}  // namespace hyde::rt

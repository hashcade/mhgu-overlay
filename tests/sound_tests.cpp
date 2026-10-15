// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Jing Haihan

#include "mhgu/core/sound.hpp"

#include <cassert>
#include <cstring>
#include <iostream>

namespace {

template <typename T>
void put(std::vector<std::byte>& wav, std::size_t offset, T value) {
  std::memcpy(wav.data() + offset, &value, sizeof(value));
}

std::vector<std::byte> sound(std::uint16_t channels, std::uint32_t rate) {
  std::vector<std::byte> wav(44 + channels * 4);
  std::memcpy(wav.data(), "RIFF", 4);
  put(wav, 4, static_cast<std::uint32_t>(wav.size() - 8));
  std::memcpy(wav.data() + 8, "WAVEfmt ", 8);
  put(wav, 16, std::uint32_t{16});
  put(wav, 20, std::uint16_t{1});
  put(wav, 22, channels);
  put(wav, 24, rate);
  put(wav, 32, static_cast<std::uint16_t>(channels * 2));
  put(wav, 34, std::uint16_t{16});
  std::memcpy(wav.data() + 36, "data", 4);
  put(wav, 40, static_cast<std::uint32_t>(channels * 4));
  for (unsigned index = 0; index < channels * 2; ++index)
    put(wav, 44 + index * 2, static_cast<std::int16_t>(100 + index));
  return wav;
}

}  // namespace

int main() {
  using mhgu::core::decode_confirmation_sound;
  const auto mono = sound(1, 48000);
  assert(
    (decode_confirmation_sound(mono) ==
     std::vector<std::int16_t>{100, 100, 101, 101})
  );
  assert(
    (decode_confirmation_sound(sound(2, 48000)) ==
     std::vector<std::int16_t>{100, 101, 102, 103})
  );
  for (const auto rate : {8000, 11025, 16000, 22050, 32000, 44100, 48000}) {
    const auto result = decode_confirmation_sound(sound(1, rate));
    const auto expected =
      static_cast<std::size_t>(((2 * 48000 + rate - 1) / rate) * 2);
    assert(result.size() == expected);
    assert(result.front() == 100 && result.back() == 101);
    for (std::size_t index = 0; index < result.size(); index += 2)
      assert(result[index] == result[index + 1]);
  }
  // Odd-size metadata chunks must not shift PCM parsing by a byte.
  auto metadata = mono;
  metadata.insert(metadata.begin() + 36, 10, std::byte{});
  std::memcpy(metadata.data() + 36, "JUNK", 4);
  put(metadata, 40, std::uint32_t{1});
  put(metadata, 4, static_cast<std::uint32_t>(metadata.size() - 8));
  assert(
    decode_confirmation_sound(metadata) == decode_confirmation_sound(mono)
  );
  // Truncated headers/chunks, unsupported formats, and excessive lengths fail
  // safely.
  for (std::size_t size = 0; size < mono.size(); ++size)
    assert(decode_confirmation_sound(std::span(mono).first(size)).empty());
  for (const auto& [offset, value] :
       {std::pair{20, 3}, {22, 3}, {34, 24}, {32, 4}}) {
    auto invalid = mono;
    put(invalid, offset, static_cast<std::uint16_t>(value));
    assert(decode_confirmation_sound(invalid).empty());
  }
  for (const auto rate : {0, 7999, 48001})
    assert(decode_confirmation_sound(sound(1, rate)).empty());
  auto invalid = mono;
  put(invalid, 40, std::uint32_t{0xFFFFFFFF});
  assert(decode_confirmation_sound(invalid).empty());
  invalid.resize(256 * 1024 + 1);
  assert(decode_confirmation_sound(invalid).empty());
  std::cout << "Confirmation sound checks passed\n";
}

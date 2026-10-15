// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Jing Haihan

#include "mhgu/core/sound.hpp"

#include <cstring>

namespace mhgu::core {
namespace {

template <typename T>
T value(std::span<const std::byte> data, std::size_t offset) {
  T result{};
  std::memcpy(&result, data.data() + offset, sizeof(result));
  return result;
}

}  // namespace

std::vector<std::int16_t>
decode_confirmation_sound(std::span<const std::byte> wav) {
  if (
    wav.size() < 12 || wav.size() > 256 * 1024 ||
    std::memcmp(wav.data(), "RIFF", 4) != 0 ||
    std::memcmp(wav.data() + 8, "WAVE", 4) != 0
  )
    return {};
  const auto declared_size =
    static_cast<std::uint64_t>(value<std::uint32_t>(wav, 4)) + 8;
  if (declared_size < 12 || declared_size > wav.size())
    return {};
  wav = wav.first(declared_size);
  std::uint16_t format{}, channels{}, bits{}, alignment{};
  std::uint32_t rate{};
  std::span<const std::byte> samples;
  for (std::size_t offset = 12; offset + 8 <= wav.size();) {
    const auto size = value<std::uint32_t>(wav, offset + 4);
    const auto start = offset + 8;
    if (size > wav.size() - start)
      return {};
    if (std::memcmp(wav.data() + offset, "fmt ", 4) == 0) {
      if (size < 16)
        return {};
      format = value<std::uint16_t>(wav, start);
      channels = value<std::uint16_t>(wav, start + 2);
      rate = value<std::uint32_t>(wav, start + 4);
      alignment = value<std::uint16_t>(wav, start + 12);
      bits = value<std::uint16_t>(wav, start + 14);
    } else if (std::memcmp(wav.data() + offset, "data", 4) == 0) {
      samples = wav.subspan(start, size);
    }
    offset = start + size + (size & 1);
  }
  if (
    format != 1 || channels < 1 || channels > 2 || (bits != 8 && bits != 16) ||
    rate < 8000 || rate > 48000 || alignment != channels * (bits / 8) ||
    samples.empty() || samples.size() % alignment != 0
  )
    return {};
  const auto frames = samples.size() / alignment;
  // A confirmation is short. Reject malformed or unexpectedly large packs.
  if (frames > rate)
    return {};
  const auto output_frames = (frames * 48000 + rate - 1) / rate;
  std::vector<std::int16_t> output(output_frames * 2);
  for (std::size_t frame = 0; frame < output_frames; ++frame) {
    const auto source = frame * rate / 48000;
    for (std::size_t channel = 0; channel < 2; ++channel) {
      const auto index =
        source * alignment + (channels == 1 ? 0 : channel * (bits / 8));
      if (bits == 16)
        output[frame * 2 + channel] = value<std::int16_t>(samples, index);
      else
        output[frame * 2 + channel] =
          (std::to_integer<int>(samples[index]) - 128) * 256;
    }
  }
  return output;
}

}  // namespace mhgu::core

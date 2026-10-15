// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Jing Haihan

#include "mhgu/platform/switch/feedback_audio.hpp"
#include "mhgu/core/sound.hpp"

#include <switch.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

namespace mhgu::platform::switch_adapter {
namespace {

bool initialized{};
bool pending{};
AudioOutBuffer buffer{};
std::unique_ptr<void, decltype(&std::free)> samples(nullptr, std::free);

}  // namespace

void stop_feedback_audio() {
  if (initialized) {
    audoutStopAudioOut();
    audoutExit();
  }
  initialized = false;
  pending = false;
  samples.reset();
  buffer = {};
}

bool play_confirmation_sound(bool enabled) {
  if (!enabled) {
    stop_feedback_audio();
    return false;
  }
  if (pending) {
    AudioOutBuffer* released{};
    u32 count{};
    if (
      R_FAILED(audoutGetReleasedAudioOutBuffer(&released, &count)) || count == 0
    )
      return false;
    pending = false;
  }
  // Read the active sound pack in-place, including user-selected packs.
  const std::unique_ptr<FILE, decltype(&std::fclose)> file(
    std::fopen("sdmc:/config/ultrahand/sounds/enter.wav", "rb"), std::fclose
  );
  if (!file || std::fseek(file.get(), 0, SEEK_END) != 0)
    return false;
  const auto size = std::ftell(file.get());
  if (
    size < 12 || size > 256 * 1024 || std::fseek(file.get(), 0, SEEK_SET) != 0
  )
    return false;
  std::vector<std::byte> wav(size);
  if (std::fread(wav.data(), 1, wav.size(), file.get()) != wav.size())
    return false;
  const auto pcm = core::decode_confirmation_sound(wav);
  if (pcm.empty())
    return false;
  if (!initialized) {
    if (R_FAILED(smInitialize()))
      return false;
    auto result = audoutInitialize();
    if (R_SUCCEEDED(result))
      result = audoutStartAudioOut();
    smExit();
    if (R_FAILED(result)) {
      audoutExit();
      return false;
    }
    initialized = true;
    audoutSetAudioOutVolume(0.6f);
  }
  const auto bytes = pcm.size() * sizeof(pcm[0]);
  const auto capacity = (bytes + 0xFFF) & ~std::size_t{0xFFF};
  samples.reset(std::aligned_alloc(0x1000, capacity));
  if (!samples)
    return false;
  std::memset(samples.get(), 0, capacity);
  std::memcpy(samples.get(), pcm.data(), bytes);
  buffer = {};
  buffer.buffer = samples.get();
  buffer.buffer_size = capacity;
  buffer.data_size = bytes;
  if (R_FAILED(audoutAppendAudioOutBuffer(&buffer))) {
    stop_feedback_audio();
    return false;
  }
  pending = true;
  return true;
}

}  // namespace mhgu::platform::switch_adapter

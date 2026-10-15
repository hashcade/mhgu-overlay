#pragma once

#include <cstdint>

namespace mhgu::core {

enum class ActionStatus : std::uint8_t {
  Idle,
  Pending,
  Completed,
  NoCharacterData,
  NoActiveQuest,
  Failed,
};

// Consume each asynchronous result once; only confirmed success requests sound.
class ActionFeedback {
public:
  void begin() {
    pending_ = true;
    status_ = ActionStatus::Pending;
    done_until_ = 0;
  }

  bool observe(ActionStatus status) {
    if (
      !pending_ || status == ActionStatus::Idle ||
      status == ActionStatus::Pending
    )
      return false;
    pending_ = false;
    status_ = status;
    return status == ActionStatus::Completed;
  }

  void confirmation(bool played, std::uint64_t now) {
    done_until_ = played ? 0 : now + 2000;
  }

  bool pending() const {
    return pending_;
  }
  ActionStatus status() const {
    return status_;
  }
  bool show_done(std::uint64_t now) const {
    return status_ == ActionStatus::Completed && now < done_until_;
  }

private:
  bool pending_{};
  ActionStatus status_{ActionStatus::Idle};
  std::uint64_t done_until_{};
};

}  // namespace mhgu::core

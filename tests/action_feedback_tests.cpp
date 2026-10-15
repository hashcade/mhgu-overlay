#include "mhgu/core/action_feedback.hpp"

#include <cassert>
#include <iostream>

int main() {
  using mhgu::core::ActionFeedback;
  using mhgu::core::ActionStatus;
  ActionFeedback feedback;
  // Opening a menu must not replay a previous operation's confirmation.
  assert(!feedback.observe(ActionStatus::Completed));
  feedback.begin();
  assert(feedback.pending());
  assert(!feedback.observe(ActionStatus::Pending));
  assert(!feedback.observe(ActionStatus::Idle));
  assert(feedback.observe(ActionStatus::Completed));
  assert(!feedback.pending());
  assert(!feedback.observe(ActionStatus::Completed));
  feedback.confirmation(false, 100);
  assert(feedback.show_done(100));
  assert(feedback.show_done(2099));
  assert(!feedback.show_done(2100));
  feedback.begin();
  assert(feedback.observe(ActionStatus::Completed));
  feedback.confirmation(true, 3000);
  assert(!feedback.show_done(3000));
  for (const auto status :
       {ActionStatus::Failed,
        ActionStatus::NoActiveQuest,
        ActionStatus::NoCharacterData}) {
    feedback.begin();
    assert(!feedback.observe(status));
    assert(!feedback.pending());
    assert(feedback.status() == status);
    assert(!feedback.show_done(4000));
    assert(!feedback.observe(status));
  }
  std::cout << "Action feedback checks passed\n";
}

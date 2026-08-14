#pragma once

#include <cstdint>

namespace rig {

enum class RawTouchState : uint8_t { Unknown, Released, Pressed };
enum class StableTouchTransition : uint8_t { None, Down, Up };
enum class TouchOwner : uint8_t {
  None,
  CategoryPager,
  CategoryOpenControl,
  MenuScroller,
  MenuRow,
  Keyboard,
  Confirmation
};
enum class TouchPhase : uint8_t {
  Idle,
  Pressed,
  DraggingHorizontal,
  DraggingVertical,
  HoldFired,
  Cancelled,
  WaitForRelease
};

// Debounces the physical contact without treating an unreadable I2C sample as
// a release. A sequence begins only after two pressed samples and ends only
// after three released samples.
class StableTouchFilter {
 public:
  StableTouchTransition update(RawTouchState raw) {
    if (raw == RawTouchState::Unknown) return StableTouchTransition::None;
    if (raw == RawTouchState::Pressed) {
      releasedSamples_ = 0;
      if (pressedSamples_ < 2) ++pressedSamples_;
      if (!pressed_ && pressedSamples_ == 2) {
        pressed_ = true;
        ++sequence_;
        holdFired_ = false;
        return StableTouchTransition::Down;
      }
      return StableTouchTransition::None;
    }
    pressedSamples_ = 0;
    if (releasedSamples_ < 3) ++releasedSamples_;
    if (pressed_ && releasedSamples_ == 3) {
      pressed_ = false;
      holdFired_ = false;
      return StableTouchTransition::Up;
    }
    return StableTouchTransition::None;
  }

  bool pressed() const { return pressed_; }
  uint32_t sequence() const { return sequence_; }
  bool fireHoldOnce() {
    if (!pressed_ || holdFired_) return false;
    holdFired_ = true;
    return true;
  }

 private:
  uint32_t sequence_ = 0;
  uint8_t pressedSamples_ = 0;
  uint8_t releasedSamples_ = 0;
  bool pressed_ = false;
  bool holdFired_ = false;
};

}  // namespace rig

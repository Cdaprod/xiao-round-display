#pragma once

#include <cstdint>

namespace rig {

enum class RawTouchState : uint8_t {
  Unknown,
  Released,
  Pressed
};

enum class StableTouchTransition : uint8_t {
  None,
  Down,
  Up
};

enum class TouchOwner : uint8_t {
  None,
  CategoryPager,
  CategoryOpenControl,
  MenuScroller,
  MenuRow,
  MenuHeader,
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

class StableTouchFilter {
 public:
  static constexpr uint32_t kReleaseDebounceMs = 60;
  static constexpr uint32_t kUnknownGraceMs = 40;

  StableTouchTransition update(
      RawTouchState raw,
      uint32_t nowMs) {

    // A valid CHSC6X coordinate starts contact immediately.
    if (raw == RawTouchState::Pressed) {
      unknownActive_ = false;
      releaseCandidate_ = false;

      if (!pressed_) {
        pressed_ = true;
        ++sequence_;
        holdFired_ = false;

        return StableTouchTransition::Down;
      }

      return StableTouchTransition::None;
    }

    // Failed read while already touching:
    // tolerate it instead of immediately releasing.
    if (raw == RawTouchState::Unknown) {
      if (!pressed_) {
        return StableTouchTransition::None;
      }

      if (!unknownActive_) {
        unknownActive_ = true;
        unknownAtMs_ = nowMs;
        return StableTouchTransition::None;
      }

      if (elapsed(nowMs, unknownAtMs_) < kUnknownGraceMs) {
        return StableTouchTransition::None;
      }

      // Unknown has persisted long enough to become a release candidate.
      raw = RawTouchState::Released;
    } else {
      unknownActive_ = false;
    }

    if (raw == RawTouchState::Released) {
      if (!pressed_) {
        releaseCandidate_ = false;
        return StableTouchTransition::None;
      }

      if (!releaseCandidate_) {
        releaseCandidate_ = true;
        releaseAtMs_ = nowMs;
        return StableTouchTransition::None;
      }

      if (elapsed(nowMs, releaseAtMs_) >= kReleaseDebounceMs) {
        pressed_ = false;
        releaseCandidate_ = false;
        unknownActive_ = false;
        holdFired_ = false;

        return StableTouchTransition::Up;
      }
    }

    return StableTouchTransition::None;
  }

  bool pressed() const {
    return pressed_;
  }

  uint32_t sequence() const {
    return sequence_;
  }

  bool fireHoldOnce() {
    if (!pressed_ || holdFired_) {
      return false;
    }

    holdFired_ = true;
    return true;
  }

 private:
  static uint32_t elapsed(
      uint32_t now,
      uint32_t then) {
    return now - then;
  }

  uint32_t sequence_ = 0;
  uint32_t releaseAtMs_ = 0;
  uint32_t unknownAtMs_ = 0;

  bool pressed_ = false;
  bool holdFired_ = false;
  bool releaseCandidate_ = false;
  bool unknownActive_ = false;
};

}  // namespace rig
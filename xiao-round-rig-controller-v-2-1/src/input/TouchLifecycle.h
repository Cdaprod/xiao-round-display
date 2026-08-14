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

// Debounces by elapsed time so behavior is independent of polling frequency.
class StableTouchFilter {
 public:
  static constexpr uint32_t kPressDebounceMs = 20;
  static constexpr uint32_t kReleaseDebounceMs = 70;
  static constexpr uint32_t kUnknownGraceMs = 40;

  StableTouchTransition update(RawTouchState raw, uint32_t nowMs) {
    if (raw == RawTouchState::Unknown) {
      if (!unknownActive_) { unknownActive_ = true; unknownAtMs_ = nowMs; }
      if (pressed_ && elapsed(nowMs, unknownAtMs_) >= kUnknownGraceMs)
        raw = RawTouchState::Released;
      else
        return StableTouchTransition::None;
    } else {
      unknownActive_ = false;
    }
    if (raw == RawTouchState::Pressed) {
      releaseCandidate_ = false;
      if (!pressCandidate_) { pressCandidate_ = true; pressAtMs_ = nowMs; }
      if (!pressed_ && elapsed(nowMs, pressAtMs_) >= kPressDebounceMs) {
        pressed_ = true;
        pressCandidate_ = false;
        ++sequence_;
        holdFired_ = false;
        return StableTouchTransition::Down;
      }
      return StableTouchTransition::None;
    }
    pressCandidate_ = false;
    if (!releaseCandidate_) { releaseCandidate_ = true; releaseAtMs_ = nowMs; }
    if (pressed_ && elapsed(nowMs, releaseAtMs_) >= kReleaseDebounceMs) {
      pressed_ = false;
      releaseCandidate_ = false;
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
  static uint32_t elapsed(uint32_t now, uint32_t then) { return now - then; }
  uint32_t sequence_ = 0;
  uint32_t pressAtMs_ = 0;
  uint32_t releaseAtMs_ = 0;
  uint32_t unknownAtMs_ = 0;
  bool pressed_ = false;
  bool holdFired_ = false;
  bool pressCandidate_ = false;
  bool releaseCandidate_ = false;
  bool unknownActive_ = false;
};

}  // namespace rig

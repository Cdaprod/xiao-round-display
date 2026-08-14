#pragma once

#include <cstdint>

namespace rig {

class SelectionModel {
 public:
  static constexpr uint16_t kHoldMs = 600;
  static constexpr uint16_t kSettleMs = 150;

  void setHighlighted(int row) { highlightedRow_ = row; }
  int highlighted() const { return highlightedRow_; }
  int pressed() const { return pressedRow_; }
  int activated() const { return activatedRow_; }

  void begin(int row, uint32_t nowMs, bool settled) {
    pressedRow_ = row;
    activatedRow_ = -1;
    holdStartedAtMs_ = nowMs;
    holdCandidate_ = settled && row >= 0 && row == highlightedRow_;
    consumed_ = false;
  }

  void moved(int totalDx, int totalDy) {
    if (absolute(totalDx) > 8 || absolute(totalDy) > 8) cancelHold();
  }

  int update(uint32_t nowMs, bool sameRow, bool actionStillValid) {
    if (!holdCandidate_ || consumed_ || !sameRow || !actionStillValid) {
      if (!sameRow || !actionStillValid) cancelHold();
      return -1;
    }
    if (nowMs - holdStartedAtMs_ < kHoldMs) return -1;
    activatedRow_ = pressedRow_;
    consumed_ = true;
    holdCandidate_ = false;
    return activatedRow_;
  }

  void release(int row, uint32_t nowMs, bool wasScroll) {
    if (wasScroll) {
      settleUntilMs_ = nowMs + kSettleMs;
    } else if (!consumed_ && row >= 0) {
      highlightedRow_ = row;
    }
    cancelHold();
    pressedRow_ = -1;
  }

  void cancel() {
    cancelHold();
    pressedRow_ = -1;
    activatedRow_ = -1;
  }
  bool settled(uint32_t nowMs) const { return nowMs >= settleUntilMs_; }
  bool holding() const { return holdCandidate_; }
  bool consumed() const { return consumed_; }
  uint16_t holdProgress(uint32_t nowMs) const {
    if (!holdCandidate_) return 0;
    const uint32_t elapsed = nowMs - holdStartedAtMs_;
    return static_cast<uint16_t>(elapsed >= kHoldMs ? 1000 : elapsed * 1000 / kHoldMs);
  }

 private:
  static int absolute(int value) { return value < 0 ? -value : value; }
  void cancelHold() { holdCandidate_ = false; }
  int highlightedRow_ = 0;
  int pressedRow_ = -1;
  int activatedRow_ = -1;
  uint32_t holdStartedAtMs_ = 0;
  uint32_t settleUntilMs_ = 0;
  bool holdCandidate_ = false;
  bool consumed_ = false;
};

class InputBarrier {
 public:
  void open(uint32_t generation) {
    generation_ = generation;
    awaitingRelease_ = true;
    releasedAtMs_ = 0;
  }
  void released(uint32_t nowMs) {
    if (awaitingRelease_) {
      awaitingRelease_ = false;
      releasedAtMs_ = nowMs;
    }
  }
  bool acceptsPress(uint32_t nowMs) const {
    return !awaitingRelease_ && releasedAtMs_ != 0 && nowMs - releasedAtMs_ >= 40;
  }
  bool blocking() const { return awaitingRelease_ || releasedAtMs_ != 0; }
  void consumeFreshPress() { releasedAtMs_ = 0; }
  uint32_t generation() const { return generation_; }

 private:
  uint32_t generation_ = 0;
  uint32_t releasedAtMs_ = 0;
  bool awaitingRelease_ = false;
};

enum class NavigationLayer : uint8_t {
  CategorySummary,
  CategoryMenu,
  Keyboard,
  Confirmation,
  ErrorModal
};
struct NavigationEntry {
  NavigationLayer layer = NavigationLayer::CategorySummary;
  uint8_t category = 0;
  int8_t highlightedRow = -1;
  float scrollOffset = 0;
};
class NavigationStack {
 public:
  static constexpr uint8_t kCapacity = 5;
  bool push(const NavigationEntry &entry) {
    if (size_ >= kCapacity) return false;
    entries_[size_++] = entry;
    return true;
  }
  bool pop(NavigationEntry &entry) {
    if (size_ == 0) return false;
    entry = entries_[--size_];
    return true;
  }
  uint8_t size() const { return size_; }
 private:
  NavigationEntry entries_[kCapacity];
  uint8_t size_ = 0;
};

}  // namespace rig

#pragma once

#include <cstdint>

namespace rig {

enum class StableLayer : uint8_t { Category, Menu };
enum class GestureOwner : uint8_t {
  None,
  CategoryHorizontal,
  MenuScroll,
  ActionTap,
  RowHoldCandidate,
  RowHoldArmed,
  KeyboardInput
};
enum class GestureResolution : uint8_t {
  None,
  CategoryPrevious,
  CategoryNext,
  OpenMenu,
  KeepMenu,
  ActivateAction,
  Cancelled
};

struct TouchSession {
  StableLayer layerAtDown = StableLayer::Category;
  uint8_t categoryAtDown = 0;
  int16_t startX = 0, startY = 0, filteredX = 0, filteredY = 0;
  int16_t totalDx = 0, totalDy = 0, velocityX = 0, velocityY = 0;
  GestureOwner gestureOwner = GestureOwner::None;
  int8_t actionCandidate = -1;
  float menuScrollOffsetAtDown = 0.0f;
  bool startedInMenuHeader = false;
  uint32_t touchStartedAt = 0, lastValidSampleAt = 0;
  bool committed = false, cancelled = false, resolved = false;
};

class GestureArbitrator {
 public:
  static constexpr int kTouchSlop = 12;
  static constexpr int kCenterOpenRadius = 72;
  static constexpr int kTapTolerance = 14;

  static bool categoryOpenControl(int16_t x, int16_t y) {
    const int dx = x - 120, dy = y - 120;
    return dx * dx + dy * dy <= kCenterOpenRadius * kCenterOpenRadius;
  }

  void begin(StableLayer layer, uint8_t category, int16_t x, int16_t y,
             uint32_t now, float scrollOffset, bool inHeader, int action) {
    session_ = TouchSession();
    session_.layerAtDown = layer;
    session_.categoryAtDown = category;
    session_.startX = session_.filteredX = x;
    session_.startY = session_.filteredY = y;
    session_.touchStartedAt = session_.lastValidSampleAt = now;
    session_.menuScrollOffsetAtDown = scrollOffset;
    session_.startedInMenuHeader = inHeader;
    session_.actionCandidate = static_cast<int8_t>(action);
    active_ = true;
  }

  GestureOwner move(int16_t x, int16_t y, int16_t vx, int16_t vy,
                    uint32_t now) {
    if (!active_ || session_.resolved) return GestureOwner::None;
    session_.filteredX = x;
    session_.filteredY = y;
    session_.totalDx = static_cast<int16_t>(x - session_.startX);
    session_.totalDy = static_cast<int16_t>(y - session_.startY);
    session_.velocityX = vx;
    session_.velocityY = vy;
    session_.lastValidSampleAt = now;
    const int ax = absolute(session_.totalDx), ay = absolute(session_.totalDy);
    if (ax > 8 || ay > 8) session_.actionCandidate = -1;
    if (session_.gestureOwner != GestureOwner::None) return session_.gestureOwner;
    if (ax <= kTouchSlop && ay <= kTouchSlop) return GestureOwner::None;
    const bool horizontal = ax >= 18 && ax * 100 >= ay * 115;
    const bool vertical = ay >= 12 && ay * 100 >= ax * 115;
    if (session_.layerAtDown == StableLayer::Category) {
      if (horizontal) session_.gestureOwner = GestureOwner::CategoryHorizontal;
    } else if (vertical && !session_.startedInMenuHeader) {
      session_.gestureOwner = GestureOwner::MenuScroll;
    }
    return session_.gestureOwner;
  }

  GestureResolution release(int16_t x, int16_t y, int16_t vx, int16_t vy,
                            int actionAtRelease) {
    if (!active_ || session_.resolved) return GestureResolution::None;
    move(x, y, vx, vy, session_.lastValidSampleAt);
    active_ = false;
    session_.resolved = true;
    if (session_.cancelled) return GestureResolution::Cancelled;
    switch (session_.gestureOwner) {
      case GestureOwner::CategoryHorizontal: {
        const int distance = absolute(session_.totalDx);
        const bool commit = distance >= 30 ||
            (distance >= 20 && absolute(session_.velocityX) >= 300);
        session_.committed = commit;
        if (!commit) return GestureResolution::None;
        return session_.totalDx < 0 ? GestureResolution::CategoryNext
                                    : GestureResolution::CategoryPrevious;
      }
      case GestureOwner::MenuScroll:
        return GestureResolution::KeepMenu;
      case GestureOwner::ActionTap:
      case GestureOwner::None:
        if (session_.layerAtDown == StableLayer::Category &&
            session_.startedInMenuHeader &&
            absolute(session_.totalDx) <= kTapTolerance &&
            absolute(session_.totalDy) <= kTapTolerance) {
          session_.committed = true;
          return GestureResolution::OpenMenu;
        }
        (void)actionAtRelease;
        return session_.layerAtDown == StableLayer::Menu
            ? GestureResolution::KeepMenu : GestureResolution::None;
      case GestureOwner::RowHoldCandidate:
      case GestureOwner::RowHoldArmed:
      case GestureOwner::KeyboardInput:
        return session_.layerAtDown == StableLayer::Menu
            ? GestureResolution::KeepMenu : GestureResolution::None;
    }
    return GestureResolution::None;
  }

  GestureResolution cancel() {
    if (!active_ || session_.resolved) return GestureResolution::None;
    active_ = false;
    session_.cancelled = session_.resolved = true;
    return GestureResolution::Cancelled;
  }

  bool active() const { return active_; }
  const TouchSession &session() const { return session_; }

 private:
  static int absolute(int value) { return value < 0 ? -value : value; }
  TouchSession session_{};
  bool active_ = false;
};

}  // namespace rig

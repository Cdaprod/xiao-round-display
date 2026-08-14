#pragma once

#include <cstdint>

namespace rig {

enum class StableLayer : uint8_t { Category, Menu };
enum class GestureOwner : uint8_t {
  None,
  CategoryHorizontal,
  CategoryMenuOpen,
  MenuScroll,
  MenuClose,
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
  CloseMenu,
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
    if (ax > 9 || ay > 9) session_.actionCandidate = -1;
    if (session_.gestureOwner != GestureOwner::None) return session_.gestureOwner;
    if (ax <= kTouchSlop && ay <= kTouchSlop) return GestureOwner::None;
    const bool horizontal = ax >= 16 && ax * 100 >= ay * 145;
    const bool vertical = ay >= 16 && ay * 100 >= ax * 145;
    if (session_.layerAtDown == StableLayer::Category) {
      if (horizontal) session_.gestureOwner = GestureOwner::CategoryHorizontal;
    } else if (vertical) {
      if (session_.totalDy > 0 &&
          (session_.startedInMenuHeader ||
           (session_.menuScrollOffsetAtDown <= 0.0f && session_.totalDy > 20)))
        session_.gestureOwner = GestureOwner::MenuClose;
      else
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
        const bool commit = distance >= 48 ||
            (distance >= 24 && absolute(session_.velocityX) >= 420);
        session_.committed = commit;
        if (!commit) return GestureResolution::None;
        return session_.totalDx < 0 ? GestureResolution::CategoryNext
                                    : GestureResolution::CategoryPrevious;
      }
      case GestureOwner::CategoryMenuOpen: {
        const float progress = openProgress();
        session_.committed = progress >= 0.42f ||
            (session_.totalDy <= -26 && session_.velocityY <= -380);
        return session_.committed ? GestureResolution::OpenMenu
                                  : GestureResolution::None;
      }
      case GestureOwner::MenuScroll:
        return GestureResolution::KeepMenu;
      case GestureOwner::MenuClose: {
        const float progress = closeProgress();
        session_.committed = progress >= 0.55f ||
            (session_.totalDy >= 32 && session_.velocityY >= 420);
        return session_.committed ? GestureResolution::CloseMenu
                                  : GestureResolution::KeepMenu;
      }
      case GestureOwner::ActionTap:
      case GestureOwner::None:
        if (session_.layerAtDown == StableLayer::Category &&
            session_.startedInMenuHeader &&
            absolute(session_.totalDx) <= 9 && absolute(session_.totalDy) <= 9) {
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

  float openProgress() const {
    return clamp((-session_.totalDy - 16) / 64.0f);
  }
  float closeProgress() const {
    const float displacement = session_.startedInMenuHeader
        ? session_.totalDy - 12.0f : session_.totalDy - 20.0f;
    return clamp(displacement / 64.0f);
  }
  bool active() const { return active_; }
  const TouchSession &session() const { return session_; }

 private:
  static int absolute(int value) { return value < 0 ? -value : value; }
  static float clamp(float value) {
    return value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value;
  }
  TouchSession session_{};
  bool active_ = false;
};

}  // namespace rig

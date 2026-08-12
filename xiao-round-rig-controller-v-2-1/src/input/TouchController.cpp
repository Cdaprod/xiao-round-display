#include "input/TouchController.h"

#include "RigBuildConfig.h"

namespace rig {

void TouchController::begin() {
  pinMode(build::kPinTouchInterrupt, INPUT_PULLUP);
  Wire.begin();
}

TouchEvent TouchController::poll(uint32_t nowMs) {
  TouchEvent event{};
  const bool active = digitalRead(build::kPinTouchInterrupt) == LOW;

  if (active && !pressed_) {
    uint16_t x = 0;
    uint16_t y = 0;
    if (!readPoint(x, y)) return event;
    pressed_ = true;
    pressX_ = x;
    pressY_ = y;
    pressedAt_ = nowMs;
    return event;
  }

  if (!active && pressed_) {
    pressed_ = false;
    if (nowMs - lastEventAt_ < build::kTouchDebounceMs) return event;
    if (!withinControlArea(pressX_, pressY_)) return event;

    event.x = pressX_;
    event.y = pressY_;
    event.durationMs = nowMs - pressedAt_;
    event.kind = event.durationMs >= build::kTouchLongPressMs
        ? TouchKind::LongPress
        : TouchKind::Tap;
    lastEventAt_ = nowMs;
  }

  return event;
}

bool TouchController::readPoint(uint16_t &x, uint16_t &y) {
  const uint8_t requested = Wire.requestFrom(
      build::kTouchAddress,
      static_cast<uint8_t>(5));
  if (requested != 5) return false;

  uint8_t data[5] = {0};
  for (uint8_t &value : data) value = Wire.read();
  if (data[0] != 0x01) return false;

  x = data[2];
  y = data[4];
  return x < build::kScreenSize && y < build::kScreenSize;
}

bool TouchController::withinControlArea(uint16_t x, uint16_t y) const {
  const int dx = static_cast<int>(x) - build::kScreenCenter;
  const int dy = static_cast<int>(y) - build::kScreenCenter;
  return (dx * dx) + (dy * dy) <= (96 * 96);
}

}  // namespace rig

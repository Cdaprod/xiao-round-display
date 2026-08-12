#pragma once

#include <Wire.h>

#include "model/RigTypes.h"

namespace rig {

class TouchController {
 public:
  void begin();
  TouchEvent poll(uint32_t nowMs);

 private:
  bool readPoint(uint16_t &x, uint16_t &y);
  bool withinControlArea(uint16_t x, uint16_t y) const;

  bool pressed_ = false;
  uint16_t pressX_ = 0;
  uint16_t pressY_ = 0;
  uint32_t pressedAt_ = 0;
  uint32_t lastEventAt_ = 0;
};

}  // namespace rig

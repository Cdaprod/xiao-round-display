#pragma once

#include <Arduino.h>

#include "display/DisplayDevice.h"
#include "model/RigTypes.h"

namespace rig {

class StatusHalo {
 public:
  explicit StatusHalo(DisplayDevice &display) : display_(display) {}

  void begin(RigState state);
  void setState(RigState state);
  void tick(uint32_t nowUs);
  void setInteractive(bool value) { interactive_ = value; }
  void setTouchFeedback(
      bool active,
      uint16_t x,
      uint16_t y,
      uint16_t progress,
      bool dragging);
  void setOutcomeFeedback(bool success);

  uint32_t renderedFrames() const { return renderedFrames_; }
  uint32_t droppedFrames() const { return droppedFrames_; }

 private:
  static constexpr size_t kPaletteSize = 256;
  static constexpr int kSegmentCount = 48;
  static constexpr int kOuterRadius = 117;
  static constexpr int kInnerRadius = 110;

  void buildPalette(RigState state, uint16_t *destination);
  void render(uint64_t elapsedUs);
  float cubicBezier(float x, float x1, float y1, float x2, float y2) const;
  uint16_t paletteColor(uint8_t index, float transition) const;
  uint16_t blend565(uint16_t from, uint16_t to, uint8_t amount) const;
  uint16_t scale565(uint16_t color, uint8_t brightness) const;
  void fillWrappedArc(
      Arduino_GFX &gfx,
      float start,
      float end,
      uint16_t color) const;

  DisplayDevice &display_;
  RigState state_ = RigState::Booting;
  uint16_t fromPalette_[kPaletteSize] = {0};
  uint16_t targetPalette_[kPaletteSize] = {0};
  bool transitioning_ = false;
  uint64_t transitionStartedUs_ = 0;

  bool clockStarted_ = false;
  uint32_t lastTickUs_ = 0;
  uint64_t elapsedUs_ = 0;
  uint32_t frameAccumulatorUs_ = 0;
  uint32_t renderedFrames_ = 0;
  uint32_t droppedFrames_ = 0;
  uint64_t lastStatsUs_ = 0;
  uint32_t statsFrameStart_ = 0;
  bool interactive_ = false;
  bool touchActive_ = false;
  bool touchDragging_ = false;
  uint16_t touchX_ = 120;
  uint16_t touchY_ = 120;
  uint16_t touchProgress_ = 0;
  int8_t outcome_ = 0;
  uint64_t outcomeStartedUs_ = 0;
};

}  // namespace rig

#pragma once

#include <Arduino.h>

namespace rig::theme {

constexpr uint16_t kBlack = 0x0000;
constexpr uint16_t kWhite = 0xFFFF;
constexpr uint16_t kRed = 0xF800;
constexpr uint16_t kGreen = 0x07E0;
constexpr uint16_t kAmber = 0xFD20;
constexpr uint16_t kGold = 0xFEA0;
constexpr uint16_t kBlue = 0x04FF;
constexpr uint16_t kCyan = 0x07FF;
constexpr uint16_t kViolet = 0xA81F;
constexpr uint16_t kMagenta = 0xF81F;
constexpr uint16_t kMuted = 0x7BEF;
constexpr uint16_t kPanel = 0x1082;
constexpr uint16_t kDimGreen = 0x0340;

inline uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue) {
  return static_cast<uint16_t>(
      ((red & 0xF8) << 8) |
      ((green & 0xFC) << 3) |
      (blue >> 3));
}

}  // namespace rig::theme

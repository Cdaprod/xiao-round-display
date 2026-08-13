#pragma once

#include <cmath>
#include <cstdint>

namespace rig {

struct HorizontalSpan {
  int16_t left;
  int16_t right;

  constexpr HorizontalSpan(int16_t leftValue = 0, int16_t rightValue = -1)
      : left(leftValue), right(rightValue) {}

  bool valid() const { return right >= left; }

  int16_t width() const {
    return valid() ? static_cast<int16_t>(right - left + 1) : 0;
  }
};

class CircularViewport {
 public:
  static constexpr int16_t kCenter = 120;
  static constexpr int16_t kSummaryRadius = 103;
  static constexpr int16_t kExpandedRadius = 108;

  explicit CircularViewport(int16_t radius = kExpandedRadius)
      : radius_(radius) {
    for (int y = 0; y < 240; ++y) {
      const int dy = y - kCenter;
      if (dy < -radius_ || dy > radius_) {
        spans_[y] = HorizontalSpan();
        continue;
      }

      const int dx = static_cast<int>(std::sqrt(static_cast<float>(
          radius_ * radius_ - dy * dy)));
      int left = kCenter - dx;
      int right = kCenter + dx;
      if (left < 0) left = 0;
      if (right > 239) right = 239;
      spans_[y] = HorizontalSpan(
          static_cast<int16_t>(left),
          static_cast<int16_t>(right));
    }
  }

  HorizontalSpan span(int y) const {
    return y >= 0 && y < 240 ? spans_[y] : HorizontalSpan();
  }

  bool contains(int x, int y) const {
    const HorizontalSpan current = span(y);
    return current.valid() && x >= current.left && x <= current.right;
  }

  int16_t radius() const { return radius_; }

 private:
  int16_t radius_;
  HorizontalSpan spans_[240];
};

}  // namespace rig

#include "ui/StatusHalo.h"

#include <cmath>

#include "RigBuildConfig.h"
#include "ui/UiTheme.h"

namespace rig {
namespace {

constexpr uint64_t kOrbitCycleUs = 8000000ULL;
constexpr uint64_t kTransitionUs = 650000ULL;

}  // namespace

void StatusHalo::begin(RigState state) {
  state_ = state;
  buildPalette(state, targetPalette_);
  memcpy(fromPalette_, targetPalette_, sizeof(targetPalette_));
  transitioning_ = false;
}

void StatusHalo::setState(RigState state) {
  if (state == state_) return;
  memcpy(fromPalette_, targetPalette_, sizeof(targetPalette_));
  buildPalette(state, targetPalette_);
  state_ = state;
  transitioning_ = true;
  transitionStartedUs_ = elapsedUs_;
}

void StatusHalo::setTouchFeedback(
    bool active,
    uint16_t x,
    uint16_t y,
    uint16_t progress,
    bool dragging) {
  touchActive_ = active;
  touchX_ = x;
  touchY_ = y;
  touchProgress_ = progress;
  touchDragging_ = dragging;
}

void StatusHalo::setOutcomeFeedback(bool success) {
  outcome_ = success ? 1 : -1;
  outcomeStartedUs_ = elapsedUs_;
}

void StatusHalo::tick(uint32_t nowUs) {
  const uint32_t frameIntervalUs = 1000000UL / RIG_HALO_TARGET_FPS;

  if (!clockStarted_) {
    clockStarted_ = true;
    lastTickUs_ = nowUs;
    frameAccumulatorUs_ = frameIntervalUs;
  } else {
    const uint32_t deltaUs = nowUs - lastTickUs_;
    lastTickUs_ = nowUs;
    elapsedUs_ += deltaUs;
    frameAccumulatorUs_ += deltaUs;
  }

  if (frameAccumulatorUs_ < frameIntervalUs) return;

  const uint32_t dueFrames = frameAccumulatorUs_ / frameIntervalUs;
  if (dueFrames > 1) droppedFrames_ += dueFrames - 1;
  // Coalesce expired deadlines. The elapsed-time phase already points at the
  // newest frame, so retaining queued intervals only causes bursty catch-up.
  frameAccumulatorUs_ = 0;
  const uint32_t renderStartedUs = micros();
  render(elapsedUs_);
  renderUs_ = micros() - renderStartedUs;
  if (renderUs_ > maxRenderUs_) maxRenderUs_ = renderUs_;
  ++renderedFrames_;

  if (elapsedUs_ - lastStatsUs_ >= 5000000ULL) {
    const uint32_t frames = renderedFrames_ - statsFrameStart_;
    const float seconds = static_cast<float>(elapsedUs_ - lastStatsUs_) / 1000000.0f;
    Serial.printf(
        "[halo] %.1f fps, %lu coalesced, raster=%lu/%luus transfer=%lu/%luus bytes/s=%lu dma=%s\n",
        frames / seconds,
        static_cast<unsigned long>(droppedFrames_),
        static_cast<unsigned long>(renderUs_ > transferUs_ ? renderUs_ - transferUs_ : 0),
        static_cast<unsigned long>(maxRenderUs_),
        static_cast<unsigned long>(transferUs_),
        static_cast<unsigned long>(maxTransferUs_),
        static_cast<unsigned long>((bytesTransferred_ - statsBytesStart_) / seconds),
        display_.dmaEnabled() ? "on" : "off");
    statsFrameStart_ = renderedFrames_;
    statsBytesStart_ = bytesTransferred_;
    lastStatsUs_ = elapsedUs_;
  }
}

void StatusHalo::buildPalette(RigState state, uint16_t *destination) {
  uint16_t keys[5] = {
      theme::kBlue,
      theme::kCyan,
      theme::kWhite,
      theme::kViolet,
      theme::kBlue,
  };

  switch (state) {
    case RigState::WifiConnecting:
      keys[0] = theme::rgb565(255, 118, 16);
      keys[1] = theme::kAmber;
      keys[2] = theme::kGold;
      keys[3] = theme::rgb565(255, 70, 8);
      keys[4] = keys[0];
      break;
    case RigState::WifiOffline:
    case RigState::ApiOffline:
      keys[0] = theme::rgb565(110, 32, 0);
      keys[1] = theme::kAmber;
      keys[2] = theme::kGold;
      keys[3] = theme::rgb565(210, 66, 0);
      keys[4] = keys[0];
      break;
    case RigState::NoSession:
    case RigState::Previewing:
      keys[0] = theme::rgb565(0, 80, 36);
      keys[1] = theme::kGreen;
      keys[2] = theme::kCyan;
      keys[3] = theme::rgb565(0, 190, 100);
      keys[4] = keys[0];
      break;
    case RigState::Recording:
      keys[0] = theme::rgb565(100, 0, 12);
      keys[1] = theme::kRed;
      keys[2] = theme::rgb565(255, 112, 16);
      keys[3] = theme::kMagenta;
      keys[4] = keys[0];
      break;
    case RigState::Sending:
      keys[0] = theme::rgb565(12, 24, 120);
      keys[1] = theme::kBlue;
      keys[2] = theme::kCyan;
      keys[3] = theme::kViolet;
      keys[4] = keys[0];
      break;
    case RigState::Error:
      keys[0] = theme::rgb565(80, 0, 0);
      keys[1] = theme::kRed;
      keys[2] = theme::kAmber;
      keys[3] = theme::kMagenta;
      keys[4] = keys[0];
      break;
    case RigState::Booting:
      break;
  }

  for (size_t index = 0; index < kPaletteSize; ++index) {
    const size_t section = index / 64;
    const uint8_t amount = static_cast<uint8_t>((index % 64) * 4);
    destination[index] = blend565(keys[section], keys[section + 1], amount);
  }
}

void StatusHalo::render(uint64_t elapsedUs) {
  const float seconds = static_cast<float>(elapsedUs) / 1000000.0f;
  const float cycle = static_cast<float>(elapsedUs % kOrbitCycleUs) /
      static_cast<float>(kOrbitCycleUs);
  const float eased = cubicBezier(cycle, 0.65f, 0.0f, 0.35f, 1.0f);
  float phaseDegrees = eased * 360.0f + seconds * 12.0f;
  phaseDegrees += 8.0f * sinf(seconds * 0.71f);
  const uint8_t phase = static_cast<uint8_t>(static_cast<int>(phaseDegrees * 256.0f / 360.0f) & 255);
  const float breath = 0.79f + 0.21f * (0.5f + 0.5f * sinf(seconds * 1.61f));
  const uint8_t brightness = static_cast<uint8_t>(breath * (interactive_ ? 130.0f : 255.0f));
  float transition = 1.0f;
  if (transitioning_) {
    const uint64_t age = elapsedUs - transitionStartedUs_;
    transition = constrain(static_cast<float>(age) / static_cast<float>(kTransitionUs), 0.0f, 1.0f);
    transition = cubicBezier(transition, 0.25f, 0.1f, 0.25f, 1.0f);
    if (age >= kTransitionUs) transitioning_ = false;
  }
  for (int index = 0; index < 256; ++index) {
    framePalette_[index] = scale565(paletteColor(static_cast<uint8_t>(index), transition), brightness);
  }
  for (uint16_t index = 0; index < geometry_.count(); ++index) {
    const HaloPixel &pixel = geometry_.pixel(index);
    framePixels_[index] = framePalette_[wrapHaloPhase(pixel.angle, phase)];
  }
  // Touch and outcome feedback modify the compact annular framebuffer only.
  if (touchActive_ || outcome_ != 0) {
    const uint8_t touchAngle = geometry_.angleAt(touchX_, touchY_);
    const uint8_t touchWidth = touchDragging_ ? 18 : static_cast<uint8_t>(8 + touchProgress_ / 80);
    const uint8_t outcomeEnd = static_cast<uint8_t>(((elapsedUs - outcomeStartedUs_) * 256ULL) / 600000ULL);
    for (uint16_t index = 0; index < geometry_.count(); ++index) {
      const uint8_t angle = geometry_.pixel(index).angle;
      const uint8_t distance = static_cast<uint8_t>(angle - touchAngle);
      const uint8_t circularDistance = distance > 128 ? static_cast<uint8_t>(256-distance) : distance;
      if (touchActive_ && circularDistance <= touchWidth) framePixels_[index] = theme::kWhite;
      if (touchActive_ && static_cast<uint8_t>(angle-touchAngle-128) < 5) framePixels_[index] = theme::kCyan;
      if (outcome_ != 0 && angle <= outcomeEnd) framePixels_[index] = outcome_ > 0 ? theme::kGreen : theme::kAmber;
    }
    if (outcome_ != 0 && elapsedUs - outcomeStartedUs_ >= 600000ULL) outcome_ = 0;
  }
  const uint32_t transferStarted = micros();
  bytesTransferred_ += presentHalo();
  transferUs_ = micros() - transferStarted;
  if (transferUs_ > maxTransferUs_) maxTransferUs_ = transferUs_;
}

uint32_t StatusHalo::presentHalo() {
  Arduino_GFX &gfx = display_.gfx();
  uint32_t bytes = 0;
  uint16_t pixelIndex = 0;
  for (int y = 0; y < HaloGeometry::kSize; ++y) {
    const HaloRow &row = geometry_.row(y);
    const HaloSpan spans[2] = {row.left, row.right};
    for (const HaloSpan &span : spans) {
      if (!span.valid()) continue;
      const uint16_t width = span.width();
      for (uint16_t x = 0; x < width; ++x) spanBuffer_[x] = framePixels_[pixelIndex++];
      gfx.draw16bitRGBBitmap(span.left, y, spanBuffer_, width, 1);
      bytes += width * 2;
    }
  }
  return bytes;
}

float StatusHalo::cubicBezier(
    float x,
    float x1,
    float y1,
    float x2,
    float y2) const {
  auto sample = [](float t, float p1, float p2) {
    const float inverse = 1.0f - t;
    return 3.0f * inverse * inverse * t * p1 +
        3.0f * inverse * t * t * p2 +
        t * t * t;
  };
  auto derivative = [](float t, float p1, float p2) {
    return 3.0f * (1.0f - t) * (1.0f - t) * p1 +
        6.0f * (1.0f - t) * t * (p2 - p1) +
        3.0f * t * t * (1.0f - p2);
  };

  float t = x;
  for (int iteration = 0; iteration < 5; ++iteration) {
    const float slope = derivative(t, x1, x2);
    if (fabsf(slope) < 0.0001f) break;
    t -= (sample(t, x1, x2) - x) / slope;
    t = constrain(t, 0.0f, 1.0f);
  }
  return sample(t, y1, y2);
}

uint16_t StatusHalo::paletteColor(uint8_t index, float transition) const {
  if (!transitioning_ && transition >= 1.0f) return targetPalette_[index];
  return blend565(
      fromPalette_[index],
      targetPalette_[index],
      static_cast<uint8_t>(transition * 255.0f));
}

uint16_t StatusHalo::blend565(uint16_t from, uint16_t to, uint8_t amount) const {
  const uint16_t inverse = 255 - amount;
  const uint8_t fromR = (from >> 11) & 0x1F;
  const uint8_t fromG = (from >> 5) & 0x3F;
  const uint8_t fromB = from & 0x1F;
  const uint8_t toR = (to >> 11) & 0x1F;
  const uint8_t toG = (to >> 5) & 0x3F;
  const uint8_t toB = to & 0x1F;

  const uint8_t red = (fromR * inverse + toR * amount) / 255;
  const uint8_t green = (fromG * inverse + toG * amount) / 255;
  const uint8_t blue = (fromB * inverse + toB * amount) / 255;
  return static_cast<uint16_t>((red << 11) | (green << 5) | blue);
}

uint16_t StatusHalo::scale565(uint16_t color, uint8_t brightness) const {
  const uint8_t red = (((color >> 11) & 0x1F) * brightness) / 255;
  const uint8_t green = (((color >> 5) & 0x3F) * brightness) / 255;
  const uint8_t blue = ((color & 0x1F) * brightness) / 255;
  return static_cast<uint16_t>((red << 11) | (green << 5) | blue);
}

}  // namespace rig

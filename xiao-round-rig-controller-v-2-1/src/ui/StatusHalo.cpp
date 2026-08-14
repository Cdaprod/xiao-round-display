#include "ui/StatusHalo.h"

#include <cmath>

#include "RigBuildConfig.h"
#include "ui/UiTheme.h"

namespace rig {
namespace {

constexpr uint64_t kOrbitCycleUs = 8000000ULL;
}  // namespace

void StatusHalo::begin(RigState state) {
  state_ = state;
  buildPalette(HaloCategory::Status, targetPalette_);
  memcpy(fromPalette_, targetPalette_, sizeof(targetPalette_));
  presentation_.begin(0);
}

void StatusHalo::setState(RigState state) {
  if (state == state_) return;
  state_ = state;
  presentation_.requestFrame(elapsedUs_);
}

void StatusHalo::setCategory(UiPage category, uint64_t nowUs) {
  (void)nowUs;
  const HaloCategory next = static_cast<HaloCategory>(static_cast<uint8_t>(category));
  if (next == presentation_.category()) return;
  memcpy(fromPalette_, targetPalette_, sizeof(targetPalette_));
  buildPalette(next, targetPalette_);
  presentation_.setCategory(next, elapsedUs_);
}

void StatusHalo::setViewMode(UiMode mode, uint64_t nowUs) {
  (void)nowUs;
  const bool expanded = mode != UiMode::Summary &&
      mode != UiMode::TabHolding && mode != UiMode::Collapsing;
  presentation_.setExpanded(expanded, elapsedUs_);
}

void StatusHalo::setTouchFeedback(
    bool active,
    uint16_t x,
    uint16_t y,
    uint16_t progress,
    bool dragging) {
  if (!active) {
    if (touchActive_) presentation_.touchRelease(elapsedUs_);
    touchActive_ = false;
    touchSuppressed_ = false;
    return;
  }
  if (dragging) {
    if (touchActive_) presentation_.touchCancel(elapsedUs_);
    touchActive_ = false;
    touchSuppressed_ = true;
    return;
  }
  if (touchSuppressed_) return;
  if (!touchActive_) presentation_.touchDown(elapsedUs_);
  if (active && progress > 0) presentation_.hold(elapsedUs_);
  touchActive_ = true;
  touchX_ = x;
  touchY_ = y;
  touchProgress_ = progress;
  touchDragging_ = dragging;
}

void StatusHalo::setOutcomeFeedback(bool success) {
  if (presentation_.presentation() == HaloPresentation::Hidden ||
      presentation_.presentation() == HaloPresentation::Exiting) return;
  outcome_ = success ? 1 : -1;
  outcomeStartedUs_ = elapsedUs_;
  presentation_.confirm(success, elapsedUs_);
}

void StatusHalo::tick(uint32_t nowUs) {
  if (!clockStarted_) {
    clockStarted_ = true;
    lastTickUs_ = nowUs;
  } else {
    const uint32_t deltaUs = nowUs - lastTickUs_;
    lastTickUs_ = nowUs;
    elapsedUs_ += deltaUs;
  }
  presentation_.update(elapsedUs_);
  const bool semanticPulse = state_ == RigState::WifiConnecting ||
      state_ == RigState::Error || state_ == RigState::Recording;
  if (!presentation_.needsFrame(elapsedUs_, semanticPulse)) return;
  const uint32_t renderStartedUs = micros();
  render(elapsedUs_);
  renderUs_ = micros() - renderStartedUs;
  if (renderUs_ > maxRenderUs_) maxRenderUs_ = renderUs_;
  ++renderedFrames_;
  presentation_.framePresented(elapsedUs_, semanticPulse);

  if (elapsedUs_ - lastStatsUs_ >= 5000000ULL) {
    const uint32_t frames = renderedFrames_ - statsFrameStart_;
    const float seconds = static_cast<float>(elapsedUs_ - lastStatsUs_) / 1000000.0f;
    Serial.printf(
        "[halo] state=%s category=%s fps=%.1f scheduled=%lu rendered=%lu skipped_idle=%lu raster=%lu/%luus transfer=%lu/%luus bytes/s=%lu dma=%s\n",
        haloPresentationName(presentation_.presentation()),
        haloCategoryName(presentation_.category()),
        frames / seconds,
        static_cast<unsigned long>(presentation_.scheduledFrames()),
        static_cast<unsigned long>(renderedFrames_),
        static_cast<unsigned long>(skippedIdleFrames_),
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

void StatusHalo::buildPalette(HaloCategory category, uint16_t *destination) {
  uint16_t categoryKeys[4];
  categoryPalette(category, categoryKeys);
  uint16_t keys[5] = {categoryKeys[0],categoryKeys[1],categoryKeys[2],
                      categoryKeys[3],categoryKeys[0]};

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
  float visibility = presentation_.visibility(elapsedUs);
  if (presentation_.presentation() == HaloPresentation::Entering)
    visibility = cubicBezier(visibility, 0.16f, 1.0f, 0.3f, 1.0f);
  else if (presentation_.presentation() == HaloPresentation::Exiting) {
    const float exitProgress = 1.0f - visibility;
    visibility = 1.0f - cubicBezier(exitProgress, 0.7f, 0.0f, 0.84f, 0.0f);
  }
  const uint8_t brightness = static_cast<uint8_t>(breath * visibility * 255.0f);
  float transition = presentation_.categoryBlend(elapsedUs);
  transition = cubicBezier(transition, 0.22f, 1.0f, 0.36f, 1.0f);
  for (int index = 0; index < 256; ++index) {
    framePalette_[index] = scale565(paletteColor(static_cast<uint8_t>(index), transition), brightness);
  }
  for (uint16_t index = 0; index < geometry_.count(); ++index) {
    const HaloPixel &pixel = geometry_.pixel(index);
    framePixels_[index] = framePalette_[wrapHaloPhase(pixel.angle, phase)];
  }
  // Touch and outcome feedback modify the compact annular framebuffer only.
  if (touchActive_ || outcome_ != 0 || state_ == RigState::WifiConnecting ||
      state_ == RigState::Error || state_ == RigState::Recording) {
    const uint8_t touchAngle = geometry_.angleAt(touchX_, touchY_);
    const uint8_t touchWidth = touchDragging_ ? 18 : static_cast<uint8_t>(8 + touchProgress_ / 80);
    const uint8_t outcomeEnd = static_cast<uint8_t>(((elapsedUs - outcomeStartedUs_) * 256ULL) / 600000ULL);
    for (uint16_t index = 0; index < geometry_.count(); ++index) {
      const uint8_t angle = geometry_.pixel(index).angle;
      const uint8_t distance = static_cast<uint8_t>(angle - touchAngle);
      const uint8_t circularDistance = distance > 128 ? static_cast<uint8_t>(256-distance) : distance;
      if (touchActive_ && circularDistance <= touchWidth) framePixels_[index] = scale565(theme::kWhite, brightness);
      if (touchActive_ && static_cast<uint8_t>(angle-touchAngle-128) < 5) framePixels_[index] = scale565(theme::kCyan, brightness);
      if (outcome_ != 0 && angle <= outcomeEnd) framePixels_[index] = scale565(outcome_ > 0 ? theme::kGreen : theme::kAmber, brightness);
      const uint8_t semanticDistance = static_cast<uint8_t>(angle - phase);
      const uint8_t semanticCircularDistance = semanticDistance > 128
          ? static_cast<uint8_t>(256 - semanticDistance) : semanticDistance;
      if (state_ == RigState::WifiConnecting && semanticCircularDistance <= 13)
        framePixels_[index] = scale565(theme::kCyan, brightness);
      if (state_ == RigState::Recording && semanticCircularDistance <= 18)
        framePixels_[index] = scale565(theme::kRed, brightness);
      if (state_ == RigState::Error &&
          (semanticCircularDistance <= 10 ||
           static_cast<uint8_t>(angle - phase - 64) <= 10))
        framePixels_[index] = scale565(theme::kRed, brightness);
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
  if (transition >= 1.0f) return targetPalette_[index];
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

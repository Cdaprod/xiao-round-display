#pragma once

#include <cstdint>

namespace rig {

enum class HaloPresentation : uint8_t {
  Hidden,
  Entering,
  Ambient,
  Touching,
  Holding,
  Confirming,
  Succeeding,
  Failing,
  Exiting
};

enum class HaloCategory : uint8_t { Status, Network, Device, Controls };

inline uint16_t haloRgb565(uint8_t red, uint8_t green, uint8_t blue) {
  return static_cast<uint16_t>(((red & 0xF8) << 8) |
                               ((green & 0xFC) << 3) | (blue >> 3));
}

inline void categoryPalette(HaloCategory category, uint16_t (&colors)[4]) {
  switch (category) {
    case HaloCategory::Status:
      colors[0]=haloRgb565(255,196,32);colors[1]=haloRgb565(255,128,16);
      colors[2]=haloRgb565(255,76,64);colors[3]=colors[0];break;
    case HaloCategory::Network:
      colors[0]=haloRgb565(0,224,255);colors[1]=haloRgb565(0,112,255);
      colors[2]=haloRgb565(52,36,180);colors[3]=colors[0];break;
    case HaloCategory::Device:
      colors[0]=haloRgb565(134,80,255);colors[1]=haloRgb565(255,0,210);
      colors[2]=haloRgb565(0,224,255);colors[3]=colors[0];break;
    case HaloCategory::Controls:
      colors[0]=haloRgb565(160,255,32);colors[1]=haloRgb565(0,220,96);
      colors[2]=haloRgb565(0,170,140);colors[3]=colors[0];break;
  }
}

class HaloPresentationModel {
 public:
  void begin(uint64_t nowUs) {
    presentation_ = HaloPresentation::Entering;
    stateStartedUs_ = nowUs;
    nextFrameAtUs_ = nowUs;
    dirty_ = true;
  }

  void setCategory(HaloCategory category, uint64_t nowUs) {
    if (category == category_) return;
    previousCategory_ = category_;
    category_ = category;
    categoryTransitionStartedUs_ = nowUs;
    categoryTransitioning_ = true;
    nextFrameAtUs_ = nowUs;
    dirty_ = true;
  }

  void setExpanded(bool expanded, uint64_t nowUs) {
    if (expanded && presentation_ != HaloPresentation::Hidden &&
        presentation_ != HaloPresentation::Exiting) {
      presentation_ = HaloPresentation::Exiting;
      stateStartedUs_ = nowUs;
      nextFrameAtUs_ = nowUs;
      dirty_ = true;
    } else if (!expanded && (presentation_ == HaloPresentation::Hidden ||
                            presentation_ == HaloPresentation::Exiting)) {
      presentation_ = HaloPresentation::Entering;
      stateStartedUs_ = nowUs;
      nextFrameAtUs_ = nowUs;
      dirty_ = true;
    }
  }

  void touchDown(uint64_t nowUs) { setTransient(HaloPresentation::Touching, nowUs); }
  void hold(uint64_t nowUs) {
    if (presentation_ == HaloPresentation::Touching)
      setTransient(HaloPresentation::Holding, nowUs);
  }
  void touchCancel(uint64_t nowUs) { settle(nowUs); }
  void touchRelease(uint64_t nowUs) { settle(nowUs); }
  void confirm(bool success, uint64_t nowUs) {
    setTransient(success ? HaloPresentation::Succeeding
                         : HaloPresentation::Failing, nowUs);
  }
  void requestFrame(uint64_t nowUs) { nextFrameAtUs_ = nowUs; dirty_ = true; }

  void update(uint64_t nowUs) {
    if (presentation_ == HaloPresentation::Entering &&
        nowUs - stateStartedUs_ >= 240000) {
      presentation_ = HaloPresentation::Ambient;
      dirty_ = true;
    } else if (presentation_ == HaloPresentation::Exiting &&
               nowUs - stateStartedUs_ >= 180000) {
      dirty_ = true;
      hideAfterFrame_ = true;
    } else if ((presentation_ == HaloPresentation::Succeeding ||
                presentation_ == HaloPresentation::Failing) &&
               nowUs - stateStartedUs_ >= 400000) {
      settle(nowUs);
    }
    if (categoryTransitioning_ &&
        nowUs - categoryTransitionStartedUs_ >= 280000) {
      categoryTransitioning_ = false;
      dirty_ = true;
    }
  }

  bool needsFrame(uint64_t nowUs, bool semanticPulse = false) const {
    if (presentation_ == HaloPresentation::Hidden) return false;
    if (dirty_) return true;
    if (categoryTransitioning_ || presentation_ == HaloPresentation::Entering ||
        presentation_ == HaloPresentation::Exiting ||
        presentation_ == HaloPresentation::Touching ||
        presentation_ == HaloPresentation::Holding ||
        presentation_ == HaloPresentation::Succeeding ||
        presentation_ == HaloPresentation::Failing)
      return nowUs >= nextFrameAtUs_;
    return semanticPulse && nowUs >= nextFrameAtUs_;
  }

  void framePresented(uint64_t nowUs, bool semanticPulse = false) {
    if (hideAfterFrame_) {
      presentation_ = HaloPresentation::Hidden;
      hideAfterFrame_ = false;
      dirty_ = false;
      ++scheduledFrames_;
      return;
    }
    dirty_ = false;
    nextFrameAtUs_ = nowUs +
        (semanticPulse && presentation_ == HaloPresentation::Ambient
             ? 33333 : frameIntervalUs());
    ++scheduledFrames_;
  }

  uint32_t frameIntervalUs() const {
    switch (presentation_) {
      case HaloPresentation::Hidden: return 0;
      case HaloPresentation::Ambient: return 142857;
      case HaloPresentation::Touching:
      case HaloPresentation::Holding: return 33333;
      default: return 33333;
    }
  }

  float visibility(uint64_t nowUs) const {
    if (presentation_ == HaloPresentation::Hidden) return 0.0f;
    if (presentation_ == HaloPresentation::Entering)
      return clamp(static_cast<float>(nowUs - stateStartedUs_) / 240000.0f);
    if (presentation_ == HaloPresentation::Exiting)
      return 1.0f - clamp(static_cast<float>(nowUs - stateStartedUs_) / 180000.0f);
    return 1.0f;
  }

  float categoryBlend(uint64_t nowUs) const {
    if (!categoryTransitioning_) return 1.0f;
    return clamp(static_cast<float>(nowUs - categoryTransitionStartedUs_) / 280000.0f);
  }

  HaloPresentation presentation() const { return presentation_; }
  HaloCategory category() const { return category_; }
  HaloCategory previousCategory() const { return previousCategory_; }
  uint64_t nextFrameAtUs() const { return nextFrameAtUs_; }
  uint32_t scheduledFrames() const { return scheduledFrames_; }

 private:
  static float clamp(float value) {
    return value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value;
  }
  void setTransient(HaloPresentation state, uint64_t nowUs) {
    if (presentation_ == HaloPresentation::Hidden ||
        presentation_ == HaloPresentation::Exiting) return;
    presentation_ = state;
    stateStartedUs_ = nowUs;
    nextFrameAtUs_ = nowUs;
    dirty_ = true;
  }
  void settle(uint64_t nowUs) {
    if (presentation_ == HaloPresentation::Hidden ||
        presentation_ == HaloPresentation::Exiting) return;
    presentation_ = HaloPresentation::Ambient;
    stateStartedUs_ = nowUs;
    nextFrameAtUs_ = nowUs;
    dirty_ = true;
  }

  HaloPresentation presentation_ = HaloPresentation::Hidden;
  HaloCategory category_ = HaloCategory::Status;
  HaloCategory previousCategory_ = HaloCategory::Status;
  uint64_t stateStartedUs_ = 0;
  uint64_t categoryTransitionStartedUs_ = 0;
  uint64_t nextFrameAtUs_ = 0;
  uint32_t scheduledFrames_ = 0;
  bool categoryTransitioning_ = false;
  bool dirty_ = false;
  bool hideAfterFrame_ = false;
};

inline const char *haloPresentationName(HaloPresentation state) {
  switch (state) {
    case HaloPresentation::Hidden: return "hidden";
    case HaloPresentation::Entering: return "entering";
    case HaloPresentation::Ambient: return "ambient";
    case HaloPresentation::Touching: return "touching";
    case HaloPresentation::Holding: return "holding";
    case HaloPresentation::Confirming: return "confirming";
    case HaloPresentation::Succeeding: return "succeeding";
    case HaloPresentation::Failing: return "failing";
    case HaloPresentation::Exiting: return "exiting";
  }
  return "unknown";
}

inline const char *haloCategoryName(HaloCategory category) {
  switch (category) {
    case HaloCategory::Status: return "status";
    case HaloCategory::Network: return "network";
    case HaloCategory::Device: return "device";
    case HaloCategory::Controls: return "controls";
  }
  return "unknown";
}

}  // namespace rig

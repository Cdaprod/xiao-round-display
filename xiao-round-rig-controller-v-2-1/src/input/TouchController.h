#pragma once
#include <Wire.h>
#include "input/GestureRecognizer.h"
namespace rig {
class TouchController {
 public: void begin(); TouchEvent poll(uint32_t nowMs); bool healthy() const{return healthy_;}
 private: bool readPoint(uint16_t &x,uint16_t &y); TouchZone zone(uint16_t x,uint16_t y)const;void filter(uint16_t x,uint16_t y,uint16_t &filteredX,uint16_t &filteredY);
  GestureRecognizer recognizer_{}; bool wasActive_=false,healthy_=false;uint16_t lastX_=0,lastY_=0,sampleX_[3]={0},sampleY_[3]={0};uint8_t sampleCount_=0,sampleIndex_=0;uint32_t lastValidSampleAt_=0;
};
}

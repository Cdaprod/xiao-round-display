#pragma once
#include <cstdint>

namespace rig {
enum class TouchKind : uint8_t { None, PressStarted, DragStarted, DragMoved, SwipeUp, SwipeDown, SwipeLeft, SwipeRight, Tap, HoldStarted, Released, Cancelled };
enum class TouchZone : uint8_t { Outside, Center, Category };
struct TouchEvent {
  TouchKind kind = TouchKind::None; TouchZone zone = TouchZone::Outside;
  uint16_t startX=0,startY=0,x=0,y=0,previousX=0,previousY=0;
  int16_t totalX=0,totalY=0,deltaX=0,deltaY=0,velocityX=0,velocityY=0;
  uint32_t durationMs=0,timestampMs=0;
};
struct GestureThresholds { uint8_t tapMovement=8,dragDistance=14; uint16_t categoryHoldMs=450,centerHoldMs=700,swipeVelocity=180; };
class GestureRecognizer {
 public:
  explicit GestureRecognizer(GestureThresholds t = {}) : thresholds_(t) {}
  TouchEvent sample(bool down,bool valid,uint16_t x,uint16_t y,uint32_t now,TouchZone zone=TouchZone::Outside) {
    if (down && !valid) return none(now);
    if (down && !active_) { active_=true; dragging_=held_=false; zone_=zone; sx_=px_=x; sy_=py_=y; started_=last_=now; return make(TouchKind::PressStarted,x,y,now); }
    if (down && active_) {
      const int dx=int(x)-int(sx_),dy=int(y)-int(sy_); const uint32_t dt=now-last_;
      vx_=dt?int16_t((int(x)-int(px_))*1000/int(dt)):0; vy_=dt?int16_t((int(y)-int(py_))*1000/int(dt)):0;
      const bool moved=(dx*dx+dy*dy)>=int(thresholds_.dragDistance)*thresholds_.dragDistance;
      TouchKind kind=TouchKind::None;
      if (!dragging_ && moved) { dragging_=true; kind=TouchKind::DragStarted; }
      else if (dragging_ && (x!=px_||y!=py_)) kind=TouchKind::DragMoved;
      else if (!held_ && !dragging_ && elapsed(now,started_) >= (zone_==TouchZone::Center?thresholds_.centerHoldMs:thresholds_.categoryHoldMs)) { held_=true; kind=TouchKind::HoldStarted; }
      TouchEvent e=make(kind,x,y,now); px_=x;py_=y;last_=now; return e;
    }
    if (!down && active_) {
      TouchEvent e=make(TouchKind::Released,px_,py_,now); active_=false;
      if (dragging_) {
        if (abs16(e.velocityX) > abs16(e.velocityY) && -e.velocityX>=thresholds_.swipeVelocity) e.kind=TouchKind::SwipeLeft;
        else if (abs16(e.velocityX) > abs16(e.velocityY) && e.velocityX>=thresholds_.swipeVelocity) e.kind=TouchKind::SwipeRight;
        else if (-e.velocityY>=thresholds_.swipeVelocity) e.kind=TouchKind::SwipeUp;
        else if (e.velocityY>=thresholds_.swipeVelocity) e.kind=TouchKind::SwipeDown;
      } else if (!held_ && abs16(e.totalX)<=thresholds_.tapMovement && abs16(e.totalY)<=thresholds_.tapMovement) e.kind=TouchKind::Tap;
      return e;
    }
    return none(now);
  }
  TouchEvent cancel(uint32_t now) { if(!active_) return none(now); TouchEvent e=make(TouchKind::Cancelled,px_,py_,now); active_=false; return e; }
 private:
  static uint32_t elapsed(uint32_t a,uint32_t b){return a-b;} static int abs16(int v){return v<0?-v:v;}
  TouchEvent none(uint32_t n) const {TouchEvent e{};e.timestampMs=n;return e;}
  TouchEvent make(TouchKind k,uint16_t x,uint16_t y,uint32_t n) const { TouchEvent e{};e.kind=k;e.zone=zone_;e.startX=sx_;e.startY=sy_;e.x=x;e.y=y;e.previousX=px_;e.previousY=py_;e.totalX=int16_t(int(x)-int(sx_));e.totalY=int16_t(int(y)-int(sy_));e.deltaX=int16_t(int(x)-int(px_));e.deltaY=int16_t(int(y)-int(py_));e.velocityX=vx_;e.velocityY=vy_;e.durationMs=elapsed(n,started_);e.timestampMs=n;return e; }
  GestureThresholds thresholds_; bool active_=false,dragging_=false,held_=false; TouchZone zone_=TouchZone::Outside; uint16_t sx_=0,sy_=0,px_=0,py_=0;int16_t vx_=0,vy_=0;uint32_t started_=0,last_=0;
};
}

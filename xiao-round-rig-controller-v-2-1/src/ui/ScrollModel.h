#pragma once
#include <cmath>
namespace rig {
class ScrollModel {
 public:
  void configure(float viewport,float content){viewport_=viewport;content_=content;offset_=clamp(offset_);}
  void beginDrag(){dragging_=true;velocity_=0;}
  void drag(float dy){float next=offset_-dy; if(next<0)next*=.35f; const float m=maxOffset();if(next>m)next=m+(next-m)*.35f;offset_=next;}
  void release(float fingerVelocity){dragging_=false;velocity_=-fingerVelocity;}
  void tick(float seconds){if(dragging_||seconds<=0)return; offset_+=velocity_*seconds;velocity_*=std::exp(-7.0f*seconds);const float target=clamp(offset_);if(target!=offset_){offset_+=(target-offset_)*std::fmin(1.0f,12.0f*seconds);velocity_*=.5f;}if(std::fabs(velocity_)<2)velocity_=0;}
  bool shouldCollapse()const{return offset_<=-36.0f;} float offset()const{return offset_;}float velocity()const{return velocity_;}bool moving()const{return dragging_||std::fabs(velocity_)>=2||offset_!=clamp(offset_);}int contentY(int screenY)const{return int(screenY+offset_);} int rowAt(int y,int top,int height,int count)const{int c=contentY(y)-top;return c<0?-1:((c/height)<count?c/height:-1);}
 private: float maxOffset()const{return content_>viewport_?content_-viewport_:0;}float clamp(float v)const{return v<0?0:(v>maxOffset()?maxOffset():v);}float viewport_=1,content_=1,offset_=0,velocity_=0;bool dragging_=false;
};
}

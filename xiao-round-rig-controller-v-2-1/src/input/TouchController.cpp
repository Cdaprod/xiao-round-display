#include "input/TouchController.h"
#include "RigBuildConfig.h"
namespace rig {
void TouchController::begin(){pinMode(build::kPinTouchInterrupt,INPUT_PULLUP);Wire.begin();}
TouchEvent TouchController::poll(uint32_t now){
 const bool active=digitalRead(build::kPinTouchInterrupt)==LOW;
 if(active){uint16_t x,y;if(!readPoint(x,y)){healthy_=false;if(wasActive_&&now-lastValidSampleAt_<=30)return{};if(wasActive_){wasActive_=false;sampleCount_=0;return recognizer_.cancel(now);}return recognizer_.sample(true,false,0,0,now);}healthy_=true;lastValidSampleAt_=now;filter(x,y,lastX_,lastY_);wasActive_=true;return recognizer_.sample(true,true,lastX_,lastY_,now,zone(lastX_,lastY_));}
 if(wasActive_){wasActive_=false;sampleCount_=0;return recognizer_.sample(false,true,lastX_,lastY_,now,zone(lastX_,lastY_));}
 return {};
}
void TouchController::filter(uint16_t x,uint16_t y,uint16_t &fx,uint16_t &fy){sampleX_[sampleIndex_]=x;sampleY_[sampleIndex_]=y;sampleIndex_=(sampleIndex_+1)%3;if(sampleCount_<3)sampleCount_++;if(sampleCount_<3){fx=x;fy=y;return;}auto median=[](uint16_t a,uint16_t b,uint16_t c){if(a>b){uint16_t t=a;a=b;b=t;}if(b>c){uint16_t t=b;b=c;c=t;}if(a>b){uint16_t t=a;a=b;b=t;}return b;};fx=median(sampleX_[0],sampleX_[1],sampleX_[2]);fy=median(sampleY_[0],sampleY_[1],sampleY_[2]);}
bool TouchController::readPoint(uint16_t &x,uint16_t &y){const uint8_t n=Wire.requestFrom(build::kTouchAddress,uint8_t(5));if(n!=5)return false;uint8_t d[5]={};for(auto &v:d)v=Wire.read();if(d[0]!=1)return false;x=d[2];y=d[4];return x<build::kScreenSize&&y<build::kScreenSize;}
TouchZone TouchController::zone(uint16_t x,uint16_t y)const{const int dx=int(x)-120,dy=int(y)-120,d2=dx*dx+dy*dy;if(d2<=70*70)return TouchZone::Center;if(d2<=104*104)return TouchZone::Category;return TouchZone::Outside;}
}

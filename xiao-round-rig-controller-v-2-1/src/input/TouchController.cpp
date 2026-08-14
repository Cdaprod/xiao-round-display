#include "input/TouchController.h"
#include "RigBuildConfig.h"
namespace rig {
void TouchController::begin(){pinMode(build::kPinTouchInterrupt,INPUT_PULLUP);Wire.begin();}
TouchEvent TouchController::poll(uint32_t now){
 const bool interruptActive=digitalRead(build::kPinTouchInterrupt)==LOW;
 RawTouchState raw=RawTouchState::Released;
 bool validPoint=false;
 if(interruptActive){uint16_t x,y;if(readPoint(x,y)){healthy_=true;validPoint=true;filter(x,y,lastX_,lastY_);raw=RawTouchState::Pressed;if(!stableTouch_.pressed()&&!pressCandidate_){pressCandidate_=true;pressX_=lastX_;pressY_=lastY_;}}else{healthy_=false;raw=RawTouchState::Unknown;}}
 else if(!stableTouch_.pressed())pressCandidate_=false;
 const StableTouchTransition transition=stableTouch_.update(raw,now);
 TouchEvent event{};
 if(transition==StableTouchTransition::Down){event=recognizer_.sample(true,true,pressX_,pressY_,now,zone(pressX_,pressY_));pressCandidate_=false;}
 else if(transition==StableTouchTransition::Up){sampleCount_=0;pressCandidate_=false;event=recognizer_.sample(false,true,lastX_,lastY_,now,zone(lastX_,lastY_));}
 else if(stableTouch_.pressed()&&validPoint)event=recognizer_.sample(true,true,lastX_,lastY_,now,zone(lastX_,lastY_));
 event.sequenceId=stableTouch_.sequence();
 return event;
}
void TouchController::filter(uint16_t x,uint16_t y,uint16_t &fx,uint16_t &fy){sampleX_[sampleIndex_]=x;sampleY_[sampleIndex_]=y;sampleIndex_=(sampleIndex_+1)%3;if(sampleCount_<3)sampleCount_++;if(sampleCount_<3){fx=x;fy=y;return;}auto median=[](uint16_t a,uint16_t b,uint16_t c){if(a>b){uint16_t t=a;a=b;b=t;}if(b>c){uint16_t t=b;b=c;c=t;}if(a>b){uint16_t t=a;a=b;b=t;}return b;};fx=median(sampleX_[0],sampleX_[1],sampleX_[2]);fy=median(sampleY_[0],sampleY_[1],sampleY_[2]);}
bool TouchController::readPoint(uint16_t &x,uint16_t &y){const uint8_t n=Wire.requestFrom(build::kTouchAddress,uint8_t(5));if(n!=5)return false;uint8_t d[5]={};for(auto &v:d)v=Wire.read();if(d[0]!=1)return false;x=d[2];y=d[4];return x<build::kScreenSize&&y<build::kScreenSize;}
TouchZone TouchController::zone(uint16_t x,uint16_t y)const{const int dx=int(x)-120,dy=int(y)-120,d2=dx*dx+dy*dy;if(d2<=70*70)return TouchZone::Center;if(d2<=104*104)return TouchZone::Category;return TouchZone::Outside;}
}

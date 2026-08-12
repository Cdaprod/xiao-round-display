#include "input/TouchController.h"
#include "RigBuildConfig.h"
namespace rig {
void TouchController::begin(){pinMode(build::kPinTouchInterrupt,INPUT_PULLUP);Wire.begin();}
TouchEvent TouchController::poll(uint32_t now){
 const bool active=digitalRead(build::kPinTouchInterrupt)==LOW;
 if(active){uint16_t x,y;if(!readPoint(x,y)){healthy_=false;if(wasActive_){wasActive_=false;return recognizer_.cancel(now);}return recognizer_.sample(true,false,0,0,now);}healthy_=true;lastX_=x;lastY_=y;wasActive_=true;return recognizer_.sample(true,true,x,y,now,zone(x,y));}
 if(wasActive_){wasActive_=false;return recognizer_.sample(false,true,lastX_,lastY_,now,zone(lastX_,lastY_));}
 return {};
}
bool TouchController::readPoint(uint16_t &x,uint16_t &y){const uint8_t n=Wire.requestFrom(build::kTouchAddress,uint8_t(5));if(n!=5)return false;uint8_t d[5]={};for(auto &v:d)v=Wire.read();if(d[0]!=1)return false;x=d[2];y=d[4];return x<build::kScreenSize&&y<build::kScreenSize;}
TouchZone TouchController::zone(uint16_t x,uint16_t y)const{const int dx=int(x)-120,dy=int(y)-120,d2=dx*dx+dy*dy;if(d2<=70*70)return TouchZone::Center;if(d2<=104*104)return TouchZone::Category;return TouchZone::Outside;}
}

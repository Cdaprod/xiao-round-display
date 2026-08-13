#pragma once
#include <cmath>
#include <cstdint>
namespace rig {
struct HaloPixel {uint16_t offset;uint8_t angle;HaloPixel(uint16_t o=0,uint8_t a=0):offset(o),angle(a){}};
struct HaloSpan {int16_t left=0,right=-1;bool valid()const{return right>=left;}uint16_t width()const{return valid()?static_cast<uint16_t>(right-left+1):0;}};
struct HaloRow {HaloSpan left,right;};
class HaloGeometry {public:static constexpr int kSize=240,kCenter=120,kOuterRadius=117,kInnerRadius=110,kMaxPixels=5200;
 HaloGeometry(){build();}uint16_t count()const{return count_;}const HaloPixel&pixel(uint16_t i)const{return pixels_[i];}const HaloRow&row(int y)const{return rows_[y];}uint8_t angleAt(int x,int y)const{float degrees=std::atan2(static_cast<float>(y-kCenter),static_cast<float>(x-kCenter))*128.0f/3.14159265358979323846f+64.0f;int value=static_cast<int>(degrees);return static_cast<uint8_t>(value&255);}
 private:void build(){const int outer2=kOuterRadius*kOuterRadius,inner2=kInnerRadius*kInnerRadius;for(int y=0;y<kSize;y++){for(int x=0;x<kSize;x++){int dx=x-kCenter,dy=y-kCenter,d2=dx*dx+dy*dy;if(d2>outer2||d2<inner2)continue;if(count_<kMaxPixels)pixels_[count_++]=HaloPixel(static_cast<uint16_t>(y*kSize+x),angleAt(x,y));HaloRow&r=rows_[y];if(x<kCenter){if(!r.left.valid())r.left.left=x;r.left.right=x;}else{if(!r.right.valid())r.right.left=x;r.right.right=x;}}}}HaloPixel pixels_[kMaxPixels]{};HaloRow rows_[kSize]{};uint16_t count_=0;};
inline uint8_t wrapHaloPhase(uint8_t angle,uint8_t phase){return static_cast<uint8_t>(angle+phase);}
}

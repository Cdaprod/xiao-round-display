#pragma once
#include <cstdint>
#include <cmath>

namespace rig {
struct HorizontalSpan { int16_t left = 0; int16_t right = -1; bool valid() const { return right >= left; } int16_t width() const { return valid() ? right-left+1 : 0; } };
class CircularViewport {
 public:
  static constexpr int16_t kCenter=120,kSummaryRadius=103,kExpandedRadius=108;
  explicit CircularViewport(int16_t radius=kExpandedRadius):radius_(radius){for(int y=0;y<240;y++){int dy=y-kCenter;if(dy < -radius_ || dy > radius_){spans_[y]={0,-1};continue;}int dx=static_cast<int>(std::sqrt(static_cast<float>(radius_*radius_-dy*dy)));int l=kCenter-dx,r=kCenter+dx;spans_[y]={static_cast<int16_t>(l<0?0:l),static_cast<int16_t>(r>239?239:r)};}}
  HorizontalSpan span(int y)const{return y>=0&&y<240?spans_[y]:HorizontalSpan{};}bool contains(int x,int y)const{auto s=span(y);return s.valid()&&x>=s.left&&x<=s.right;}int16_t radius()const{return radius_;}
 private:int16_t radius_;HorizontalSpan spans_[240]{};
};
}

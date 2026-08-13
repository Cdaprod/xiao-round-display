#pragma once
#include <cstdint>
namespace rig {
enum UiDirty:uint16_t{DirtyNone=0,BackgroundDirty=1,HeaderDirty=2,SummaryBodyDirty=4,PanelViewportDirty=8,RowDirty=16,ScrollDirty=32,OverlayDirty=64,FeedbackDirty=128};
class UiInvalidation {public:void add(uint16_t bits){bits_|=bits;}bool has(uint16_t bits)const{return(bits_&bits)!=0;}uint16_t take(){uint16_t b=bits_;bits_=0;return b;}uint16_t peek()const{return bits_;}private:uint16_t bits_=BackgroundDirty|HeaderDirty|SummaryBodyDirty;};
struct RenderCounters{uint32_t fullRedraws=0,rowRedraws=0,viewportRedraws=0,feedbackRedraws=0,composeUs=0,transferUs=0,maxTouchIntervalMs=0,minFreeHeap=0xffffffffu;};
}

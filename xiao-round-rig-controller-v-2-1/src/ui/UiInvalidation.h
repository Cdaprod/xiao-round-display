#pragma once
#include <cstdint>
namespace rig {
enum UiDirty:uint16_t{DirtyNone=0,BackgroundDirty=1,HeaderDirty=2,SummaryBodyDirty=4,PanelViewportDirty=8,RowDirty=16,ScrollDirty=32,OverlayDirty=64,FeedbackDirty=128};
struct DirtyRows {int16_t first,last;constexpr DirtyRows(int16_t firstRow=240,int16_t lastRow=-1):first(firstRow),last(lastRow){}bool valid()const{return last>=first;}void include(int top,int bottom){if(top<0)top=0;if(bottom>239)bottom=239;if(bottom<top)return;if(top<first)first=top;if(bottom>last)last=bottom;}void full(){first=0;last=239;}void clear(){first=240;last=-1;}};
class UiInvalidation {public:void add(uint16_t bits,int top=0,int bottom=239){bits_|=bits;rows_.include(top,bottom);}bool has(uint16_t bits)const{return(bits_&bits)!=0;}uint16_t take(DirtyRows&rows){uint16_t b=bits_;rows=rows_;bits_=0;rows_.clear();return b;}uint16_t take(){DirtyRows ignored;return take(ignored);}uint16_t peek()const{return bits_;}const DirtyRows&rows()const{return rows_;}private:uint16_t bits_=BackgroundDirty|HeaderDirty|SummaryBodyDirty;DirtyRows rows_=DirtyRows(0,239);};
struct RenderCounters{uint32_t fullRedraws=0,rowRedraws=0,viewportRedraws=0,feedbackRedraws=0,composeUs=0,composeMaxUs=0,composeTotalUs=0,transferUs=0,transferMaxUs=0,transferTotalUs=0,transfers=0,bytesTransferred=0,skippedFrames=0,maxTouchIntervalMs=0,minFreeHeap=0xffffffffu;};
}

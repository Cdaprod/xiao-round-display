#include <cassert>
#include <iostream>
#include "../src/ui/CircularViewport.h"
#include "../src/ui/ScrollModel.h"
#include "../src/ui/UiInvalidation.h"
#include "../src/ui/FrameScheduler.h"
using namespace rig;
int main(){
 HorizontalSpan explicitSpan(7, 19);assert(explicitSpan.valid()&&explicitSpan.left==7&&explicitSpan.right==19&&explicitSpan.width()==13);HorizontalSpan invalidSpan;assert(!invalidSpan.valid()&&invalidSpan.width()==0);
 CircularViewport summary(CircularViewport::kSummaryRadius),expanded(CircularViewport::kExpandedRadius);
 assert(summary.radius()==103&&expanded.radius()==108);
 for(int y=0;y<240;y++){for(auto*v:{&summary,&expanded}){auto s=v->span(y);assert(!s.valid()||(s.left>=0&&s.right<=239&&s.left<=s.right));}}
 assert(expanded.contains(120,12)&&!expanded.contains(5,5));auto top=expanded.span(12),middle=expanded.span(120);assert(top.width()<middle.width());
 ScrollModel scroll;scroll.configure(166,270);scroll.beginDrag();scroll.drag(-500);scroll.release(0);for(int i=0;i<200;i++)scroll.tick(.01f);assert(scroll.offset()>103&&scroll.offset()<105);assert(scroll.rowAt(215,52,30,9)==8);float retained=scroll.offset();scroll.configure(166,270);assert(scroll.offset()>retained-.01f&&scroll.offset()<retained+.01f);
 UiInvalidation dirty;DirtyRows rows;uint16_t initial=dirty.take(rows);assert(initial&(BackgroundDirty|HeaderDirty|SummaryBodyDirty));assert(rows.first==0&&rows.last==239);assert(dirty.take()==DirtyNone);dirty.add(RowDirty,145,176);assert(dirty.take(rows)==RowDirty&&rows.first==145&&rows.last==176);dirty.add(FeedbackDirty,80,116);assert(dirty.has(FeedbackDirty)&&!dirty.has(PanelViewportDirty));dirty.take(rows);assert(rows.first==80&&rows.last==116&&dirty.peek()==DirtyNone);
 LatestFrameScheduler frames(41666);assert(frames.tick(0));assert(!frames.tick(1000));assert(frames.tick(200000));assert(frames.coalesced()==3);assert(!frames.tick(201000));
 // Halo ownership is deliberately absent from content invalidation.
 assert((static_cast<uint16_t>(BackgroundDirty|HeaderDirty|SummaryBodyDirty|PanelViewportDirty|RowDirty|ScrollDirty|OverlayDirty|FeedbackDirty)&0x100)==0);
 std::cout<<"circular compositor architecture tests passed\n";
}

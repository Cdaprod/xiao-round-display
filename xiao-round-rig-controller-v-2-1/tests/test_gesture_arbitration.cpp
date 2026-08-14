#include <cassert>
#include "../src/input/GestureArbitrator.h"
using namespace rig;

static GestureArbitrator category() {
  GestureArbitrator g; g.begin(StableLayer::Category, 2, 120, 120, 0, 0, false, -1); return g;
}
static GestureArbitrator menu(float offset=0, bool header=false, int action=-1) {
  GestureArbitrator g; g.begin(StableLayer::Menu, 2, 120, header?20:100, 0, offset, header, action); return g;
}
int main() {
  { auto g=category(); assert(g.move(131,120,0,0,10)==GestureOwner::None); assert(g.release(131,120,0,0,-1)==GestureResolution::None); }
  { GestureArbitrator g; g.begin(StableLayer::Category,2,120,30,0,0,true,-1); assert(g.release(120,30,0,0,-1)==GestureResolution::OpenMenu); }
  { auto g=category(); assert(g.move(138,138,0,0,10)==GestureOwner::None); }
  { auto g=category(); assert(g.move(145,121,0,0,10)==GestureOwner::CategoryHorizontal); assert(g.move(121,80,0,0,20)==GestureOwner::CategoryHorizontal); }
  { auto g=category(); g.move(150,120,0,0,10); assert(g.release(150,80,0,-500,-1)==GestureResolution::None); }
  { auto g=category(); g.move(120,90,0,-500,10); assert(g.session().categoryAtDown==2); assert(g.release(120,80,0,-500,-1)==GestureResolution::OpenMenu); }
  { auto g=category(); g.move(150,120,100,0,10); assert(g.release(150,120,100,0,-1)==GestureResolution::None); assert(g.session().categoryAtDown==2); }
  { auto g=category(); g.move(120,100,0,-50,10); assert(g.release(120,100,0,-50,-1)==GestureResolution::None); }
  { auto g=category(); g.move(120,80,0,-500,10); assert(g.release(120,80,0,-500,-1)==GestureResolution::OpenMenu); assert(g.session().committed); }
  { auto g=menu(40); assert(g.move(120,75,0,-200,10)==GestureOwner::MenuScroll); assert(g.release(120,70,0,-100,-1)==GestureResolution::KeepMenu); }
  { auto g=menu(40); assert(g.move(120,130,0,200,10)==GestureOwner::MenuScroll); assert(g.release(120,160,0,500,-1)==GestureResolution::KeepMenu); }
  { auto g=menu(1); assert(g.move(120,120,0,200,10)==GestureOwner::MenuScroll); assert(g.move(120,170,0,500,20)==GestureOwner::MenuScroll); }
  { auto g=menu(0,true); g.move(120,45,0,100,10); assert(g.release(120,45,0,100,-1)==GestureResolution::KeepMenu); }
  { auto g=menu(0,true); g.move(120,90,0,500,10); assert(g.release(120,90,0,500,-1)==GestureResolution::CloseMenu); assert(g.session().categoryAtDown==2); }
  { auto g=menu(); assert(g.move(170,102,500,0,10)==GestureOwner::None); assert(g.release(170,102,500,0,-1)==GestureResolution::KeepMenu); }
  { auto g=menu(0,false,3); g.move(131,100,0,0,10); assert(g.release(131,100,0,0,3)!=GestureResolution::ActivateAction); }
  { auto g=menu(0,false,3); assert(g.release(120,100,0,0,3)==GestureResolution::ActivateAction); assert(g.release(120,100,0,0,3)==GestureResolution::None); }
  { auto g=category(); assert(g.cancel()==GestureResolution::Cancelled); assert(g.release(120,120,0,0,-1)==GestureResolution::None); }
  { auto g=category(); g.move(70,120,-500,0,10); assert(g.release(70,120,-500,0,-1)==GestureResolution::CategoryNext); }
  return 0;
}

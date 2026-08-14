#include <cassert>
#include <cstring>
#include <iostream>
#include "../src/input/GestureRecognizer.h"
#include "../src/ui/ScrollModel.h"
using namespace rig;
static TouchEvent release(GestureRecognizer&r,uint16_t x,uint16_t y,uint32_t t,TouchZone z=TouchZone::Category){r.sample(true,true,x,y,t-50,z);return r.sample(false,true,x,y,t,z);}
int main(){
 {GestureRecognizer r;auto e=release(r,100,100,100);assert(e.kind==TouchKind::Tap);}
 {GestureRecognizer r;r.sample(true,true,100,100,0);r.sample(true,true,106,103,20);assert(r.sample(false,true,106,103,40).kind==TouchKind::Tap);}
 {GestureRecognizer r;r.sample(true,true,100,100,0);assert(r.sample(true,true,100,80,50).kind==TouchKind::DragStarted);assert(r.sample(false,true,100,80,70).kind!=TouchKind::Tap);}
 {GestureRecognizer r;r.sample(true,true,100,100,0,TouchZone::Category);assert(r.sample(true,true,100,100,450).kind==TouchKind::HoldStarted);assert(r.sample(true,true,100,100,500).kind==TouchKind::None);assert(r.sample(false,true,100,100,510).kind==TouchKind::Released);}
 {GestureRecognizer r;r.sample(true,true,100,150,0);r.sample(true,true,100,80,50);assert(r.sample(false,true,100,80,55).kind==TouchKind::SwipeUp);}
 {GestureRecognizer r;r.sample(true,true,100,80,0);r.sample(true,true,100,150,50);assert(r.sample(false,true,100,150,55).kind==TouchKind::SwipeDown);}
 {GestureRecognizer r;r.sample(true,true,180,100,0);r.sample(true,true,80,100,50);assert(r.sample(false,true,80,100,55).kind==TouchKind::SwipeLeft);}
 {GestureRecognizer r;r.sample(true,true,80,100,0);r.sample(true,true,180,100,50);assert(r.sample(false,true,180,100,55).kind==TouchKind::SwipeRight);}
 {GestureRecognizer r;r.sample(true,true,100,100,0);r.sample(true,true,100,120,1000);assert(r.sample(false,true,100,120,1100).kind==TouchKind::Released);}
 {GestureRecognizer r;assert(r.sample(true,false,0,0,0).kind==TouchKind::None);assert(r.sample(false,true,0,0,10).kind==TouchKind::None);}
 {GestureRecognizer r;r.sample(true,true,120,120,0,TouchZone::Center);assert(r.sample(true,true,120,120,699).kind==TouchKind::None);assert(r.sample(true,true,120,120,700).kind==TouchKind::HoldStarted);}
 {GestureRecognizer r;r.sample(true,true,10,10,0xfffffff0u);assert(r.sample(true,true,10,10,434,TouchZone::Category).kind==TouchKind::HoldStarted);}
 {ScrollModel s;s.configure(100,300);s.beginDrag();s.drag(-20);assert(s.offset()==20);s.release(-300);s.tick(.1f);assert(s.offset()>20);float v=s.velocity();s.tick(.1f);assert(s.velocity()<v);}
 {ScrollModel s;s.configure(100,300);s.beginDrag();s.drag(40);assert(s.offset()<0&&s.offset()>-40);assert(!s.shouldCollapse());s.drag(120);assert(s.shouldCollapse());}
 {ScrollModel s;s.configure(100,300);s.beginDrag();s.drag(-400);assert(s.offset()>200&&s.offset()<400);s.release(0);for(int i=0;i<100;i++)s.tick(.02f);assert(s.offset()>=199&&s.offset()<=201);assert(s.rowAt(10,0,20,20)==10);}
 std::cout<<"core gesture/scroll tests passed\n";
}

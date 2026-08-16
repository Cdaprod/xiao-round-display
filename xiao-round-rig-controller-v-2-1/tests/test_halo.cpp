#include <cassert>
#include <cstdint>
#include <iostream>
#include "../src/ui/HaloGeometry.h"
using namespace rig;
int main(){HaloGeometry geometry;assert(geometry.count()>4000&&geometry.count()<HaloGeometry::kMaxPixels);bool disjoint=false;for(uint16_t i=0;i<geometry.count();i++){const HaloPixel&p=geometry.pixel(i);assert(p.offset<240*240);int x=p.offset%240,y=p.offset/240,dx=x-120,dy=y-120,d2=dx*dx+dy*dy;assert(d2>=110*110&&d2<=117*117);}for(int y=0;y<240;y++){const HaloRow&r=geometry.row(y);if(r.left.valid()&&r.right.valid()&&y>10&&y<230){assert(r.left.right<r.right.left-1);disjoint=true;}}assert(disjoint);assert(wrapHaloPhase(250,10)==4);uint8_t deterministic=wrapHaloPhase(geometry.pixel(100).angle,77);assert(deterministic==wrapHaloPhase(geometry.pixel(100).angle,77));std::cout<<"halo geometry tests passed\n";}

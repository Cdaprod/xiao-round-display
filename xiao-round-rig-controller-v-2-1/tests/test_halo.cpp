#include <cassert>
#include <cstdint>
#include <iostream>
#include "../src/ui/HaloGeometry.h"
#include "../src/ui/HaloPresentationModel.h"
using namespace rig;
int main(){
 HaloGeometry geometry;assert(geometry.count()>4000&&geometry.count()<HaloGeometry::kMaxPixels);bool disjoint=false;
 for(uint16_t i=0;i<geometry.count();i++){const HaloPixel&p=geometry.pixel(i);assert(p.offset<240*240);int x=p.offset%240,y=p.offset/240,dx=x-120,dy=y-120,d2=dx*dx+dy*dy;assert(d2>=110*110&&d2<=117*117);}
 for(int y=0;y<240;y++){const HaloRow&r=geometry.row(y);if(r.left.valid()&&r.right.valid()&&y>10&&y<230){assert(r.left.right<r.right.left-1);disjoint=true;}}
 assert(disjoint);assert(wrapHaloPhase(250,10)==4);uint8_t deterministic=wrapHaloPhase(geometry.pixel(100).angle,77);assert(deterministic==wrapHaloPhase(geometry.pixel(100).angle,77));
 uint16_t status[4],network[4],device[4],controls[4];categoryPalette(HaloCategory::Status,status);categoryPalette(HaloCategory::Network,network);categoryPalette(HaloCategory::Device,device);categoryPalette(HaloCategory::Controls,controls);assert(status[0]!=network[0]);assert(network[0]!=device[0]);assert(device[0]!=controls[0]);
 HaloPresentationModel model;model.begin(0);assert(model.needsFrame(0));model.framePresented(0);model.setCategory(HaloCategory::Network,1000);assert(model.category()==HaloCategory::Network);assert(model.categoryBlend(1000)==0.0f);model.update(281000);assert(model.categoryBlend(281000)==1.0f);
 model.setExpanded(true,300000);assert(model.presentation()==HaloPresentation::Exiting);model.setExpanded(true,350000);assert(model.visibility(350000)<0.8f);model.touchRelease(310000);assert(model.presentation()==HaloPresentation::Exiting);model.update(480000);assert(model.needsFrame(480000));model.framePresented(480000);assert(model.presentation()==HaloPresentation::Hidden);assert(!model.needsFrame(500000,true));
 model.setExpanded(false,600000);assert(model.presentation()==HaloPresentation::Entering);model.touchDown(610000);model.touchCancel(620000);assert(model.presentation()==HaloPresentation::Ambient);
 std::cout<<"halo geometry/presentation tests passed\n";
}

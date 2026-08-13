#include "ui/UiCompositor.h"
namespace rig {
UiCompositor::~UiCompositor(){delete canvas_;}
bool UiCompositor::begin(){if(canvas_)return true;Arduino_Canvas_Indexed*candidate=new Arduino_Canvas_Indexed(240,240,&display_.gfx());if(!candidate)return false;if(!candidate->begin()){delete candidate;return false;}canvas_=candidate;canvas_->setTextWrap(false);canvas_->fillScreen(0);return true;}
Arduino_GFX&UiCompositor::canvas(){return *canvas_;}
void UiCompositor::clear(const CircularViewport&v,uint16_t color){if(!canvas_)return;for(int y=0;y<240;y++){auto s=v.span(y);if(s.valid())canvas_->drawFastHLine(s.left,y,s.width(),color);}}
void UiCompositor::present(const CircularViewport&v){if(!canvas_)return;uint8_t*fb=canvas_->getFramebuffer();uint16_t*palette=canvas_->getColorIndex();Arduino_GFX&output=display_.gfx();for(int y=0;y<240;y++){auto s=v.span(y);if(!s.valid())continue;const uint8_t*src=fb+y*240+s.left;for(int x=0;x<s.width();x++)line_[x]=palette[src[x]];output.draw16bitRGBBitmap(s.left,y,line_,s.width(),1);}}
}

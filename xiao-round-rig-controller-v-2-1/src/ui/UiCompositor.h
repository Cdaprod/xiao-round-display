#pragma once
#include <Arduino_GFX_Library.h>
#include "display/DisplayDevice.h"
#include "ui/CircularViewport.h"
namespace rig {
class UiCompositor {
 public:
  explicit UiCompositor(DisplayDevice&display):display_(display){}
  ~UiCompositor();bool begin();Arduino_GFX&canvas();void clear(const CircularViewport&,uint16_t color=0);void present(const CircularViewport&);bool ready()const{return canvas_!=nullptr;}uint32_t allocationBytes()const{return ready()?57600:0;}
 private:DisplayDevice&display_;Arduino_Canvas_Indexed*canvas_=nullptr;uint16_t line_[240]{};
};
}

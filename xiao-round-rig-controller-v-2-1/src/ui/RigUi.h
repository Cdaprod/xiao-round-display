#pragma once
#include "display/DisplayDevice.h"
#include "model/RigTypes.h"
#include "storage/ConfigStore.h"
#include "ui/CircularViewport.h"
#include "ui/ScrollModel.h"
#include "ui/StatusHalo.h"
#include "ui/UiCompositor.h"
#include "ui/UiInvalidation.h"
namespace rig {
enum class UiAction:uint8_t{None,RetryWifi,ScanWifi,SelectWifi,DisconnectWifi,TestApi,PollApi,StartRecording,StopRecording,ClearError,SaveConfig,ForgetWifi,ClearWifiOverride,ClearOverrides,Reboot,ReloadSd,DisplayTest,TouchTest};
struct UiCommand{UiAction action=UiAction::None;ConfigField field=ConfigField::WifiSsid;char value[96]={0};};
class RigUi {
 public:
  explicit RigUi(DisplayDevice&d):display_(d),halo_(d),compositor_(d){}
  void begin(const RigSnapshot&);void updateSnapshot(const RigSnapshot&);void tick(uint32_t);void handleTouch(const TouchEvent&);bool takeCommand(UiCommand&);void cyclePage();void showStatus();
  UiMode mode()const{return mode_;}uint32_t haloFrames()const{return halo_.renderedFrames();}uint32_t haloDropped()const{return halo_.droppedFrames();}const RenderCounters&renderCounters()const{return counters_;}float scrollOffset()const{return scroll_.offset();}
 private:
  void compose(uint16_t,const DirtyRows&);void drawSummary(Arduino_GFX&);void drawPanel(Arduino_GFX&);void drawKeyboard(Arduino_GFX&);void drawConfirm(Arduino_GFX&);void drawFeedback(Arduino_GFX&);void text(Arduino_GFX&,const char*,int,int,uint8_t,uint16_t,bool=false);void separator(Arduino_GFX&,int,uint16_t);const char*stateLabel()const;uint16_t accent()const;const char*pageName()const;void expand();void collapse();void changePage(int);void openEditor(ConfigField,const char*,bool);void queue(UiAction);int actionAt(uint16_t,uint16_t)const;int keyboardKeyAt(uint16_t,uint16_t)const;void executeRow(int);void appendKey(int);uint16_t snapshotInvalidation(const RigSnapshot&,DirtyRows&)const;bool expanded()const;
  DisplayDevice&display_;StatusHalo halo_;UiCompositor compositor_;CircularViewport summaryView_{CircularViewport::kSummaryRadius},expandedView_{CircularViewport::kExpandedRadius};RigSnapshot snapshot_{};UiPage page_=UiPage::Status;UiMode mode_=UiMode::Summary,modalReturnMode_=UiMode::Expanded;ScrollModel scroll_;UiInvalidation invalidation_;RenderCounters counters_{};UiCommand command_{};bool commandReady_=false,masked_=false,reveal_=false,contact_=false,dragged_=false;ConfigField editField_=ConfigField::WifiSsid;char edit_[96]={0};uint8_t layout_=0;uint32_t animationAtUs_=0,lastTickUs_=0,lastDiagnosticUs_=0,lastFeedbackUs_=0,lastDiagnosticBytes_=0,pressAtMs_=0;uint16_t touchX_=120,touchY_=120;int pressedRow_=-1,confirmRow_=-1;uint16_t transitionProgress_=0;
};
}

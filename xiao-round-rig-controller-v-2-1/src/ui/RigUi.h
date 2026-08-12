#pragma once
#include "display/DisplayDevice.h"
#include "model/RigTypes.h"
#include "storage/ConfigStore.h"
#include "ui/ScrollModel.h"
#include "ui/StatusHalo.h"
namespace rig {
enum class UiAction:uint8_t{None,RetryWifi,ScanWifi,SelectWifi,DisconnectWifi,TestApi,PollApi,StartRecording,StopRecording,ClearError,SaveConfig,ForgetWifi,ClearOverrides,Reboot,ReloadSd,DisplayTest,TouchTest};
struct UiCommand{UiAction action=UiAction::None;ConfigField field=ConfigField::WifiSsid;char value[96]={0};};
class RigUi {public:explicit RigUi(DisplayDevice&d):display_(d),halo_(d){}void begin(const RigSnapshot&);void updateSnapshot(const RigSnapshot&);void tick(uint32_t);void handleTouch(const TouchEvent&);bool takeCommand(UiCommand&);void cyclePage();void showStatus();UiMode mode()const{return mode_;}uint32_t haloFrames()const{return halo_.renderedFrames();}uint32_t haloDropped()const{return halo_.droppedFrames();}
 private:void drawContent();void drawSummary(Arduino_GFX&);void drawPanel(Arduino_GFX&);void drawKeyboard(Arduino_GFX&);void drawConfirm(Arduino_GFX&);void drawTabs(Arduino_GFX&);void centerText(Arduino_GFX&,const char*,int,uint8_t,uint16_t);const char*stateLabel()const;uint16_t stateColor()const;void expand();void collapse();void openEditor(ConfigField,const char*,bool);void queue(UiAction);int tabAt(uint16_t,uint16_t)const;int actionAt(uint16_t,uint16_t)const;void executeRow(int);void appendKey(int);const char*pageName()const;
 DisplayDevice&display_;StatusHalo halo_;RigSnapshot snapshot_{};UiPage page_=UiPage::Status;UiMode mode_=UiMode::Summary;ScrollModel scroll_;UiCommand command_{};bool commandReady_=false,dirty_=true,masked_=false,reveal_=false;ConfigField editField_=ConfigField::WifiSsid;char edit_[96]={0};uint8_t layout_=0;uint32_t animationAt_=0;int confirmRow_=-1,pressedTab_=-1;
};}

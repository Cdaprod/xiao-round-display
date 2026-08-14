#include "ui/RigUi.h"
#include <cstdio>
#include <cstring>
#include "RigBuildConfig.h"
#include "ui/UiTheme.h"
namespace rig {
namespace {
const char* const kNames[]={"STATUS","NETWORK","DEVICE","CONTROLS"};
const char* const kStatus[]={"RETRY FAILED STAGE","REFRESH API","CLEAR LAST ERROR","RETURN TO SUMMARY"};
const char* const kNetwork[]={"RETRY NOW","SCAN NETWORKS","SELECT SSID","EDIT SSID","EDIT PASSWORD","EDIT API URL","TEST API","DISCONNECT","FORGET WIFI","CLEAR WIFI OVERRIDE"};
const char* const kDevice[]={"EDIT NODE ID","EDIT DEVICE TOKEN","EDIT API URL","RELOAD SD CONFIG","CLEAR SAVED OVERRIDES","DISPLAY TEST","TOUCH TEST","REBOOT DEVICE"};
const char* const kControls[]={"POLL SESSION","START RECORDING","STOP RECORDING","ACKNOWLEDGE ERROR","RETURN TO STATUS"};
uint32_t elapsed(uint32_t now,uint32_t then){return now-then;}
}
void RigUi::begin(const RigSnapshot&s){snapshot_=s;const uint32_t before=ESP.getFreeHeap();bool ok=compositor_.begin();const uint32_t after=ESP.getFreeHeap();counters_.minFreeHeap=after;Serial.printf("ui: indexed compositor=%s bytes=%lu heap=%lu->%lu\n",ok?"ready":"span-fallback",(unsigned long)compositor_.allocationBytes(),(unsigned long)before,(unsigned long)after);display_.gfx().fillScreen(theme::kBlack);halo_.begin(s.state);invalidation_.add(BackgroundDirty|HeaderDirty|SummaryBodyDirty);}
uint16_t RigUi::snapshotInvalidation(const RigSnapshot&s,DirtyRows&rows)const{
 if(s.state!=snapshot_.state||s.wifiConnected!=snapshot_.wifiConnected||s.sessionPresent!=snapshot_.sessionPresent||s.configParsed!=snapshot_.configParsed||s.sdReady!=snapshot_.sdReady||s.nvsOverrides!=snapshot_.nvsOverrides||s.tokenConfigured!=snapshot_.tokenConfigured){rows.full();return mode_==UiMode::Summary?SummaryBodyDirty:PanelViewportDirty;}
 if(s.retrySeconds!=snapshot_.retrySeconds){rows.include(mode_==UiMode::Summary?145:45,mode_==UiMode::Summary?176:82);return RowDirty;}
 if(expanded()&&(s.requestInProgress!=snapshot_.requestInProgress||strcmp(s.lastError,snapshot_.lastError)||strcmp(s.sessionStatus,snapshot_.sessionStatus)||strcmp(s.ipAddress,snapshot_.ipAddress))){rows.include(45,220);return RowDirty;}
 return DirtyNone;
}
void RigUi::updateSnapshot(const RigSnapshot&s){DirtyRows rows;const uint16_t dirty=snapshotInvalidation(s,rows);RigState old=snapshot_.state;snapshot_=s;if(old!=s.state)halo_.setState(s.state);if(dirty)invalidation_.add(dirty,rows.first,rows.last);}
bool RigUi::expanded()const{return mode_!=UiMode::Summary&&mode_!=UiMode::TabHolding;}
void RigUi::tick(uint32_t now){if(!lastTickUs_)lastTickUs_=now;uint32_t delta=now-lastTickUs_;lastTickUs_=now;if(mode_==UiMode::Scrolling||scroll_.moving()){scroll_.tick(delta/1000000.0f);selection_.setHighlighted(scroll_.rowAt(120,58,30,rowCount()));invalidation_.add(ScrollDirty);}if(mode_==UiMode::Expanding||mode_==UiMode::Collapsing){uint32_t duration=mode_==UiMode::Expanding?260000:200000;transitionProgress_=static_cast<uint16_t>(min(1000UL,elapsed(now,animationAtUs_)*1000UL/duration));invalidation_.add(PanelViewportDirty);if(transitionProgress_>=1000){mode_=mode_==UiMode::Expanding?UiMode::Expanded:UiMode::Summary;invalidation_.add(BackgroundDirty|HeaderDirty|(mode_==UiMode::Summary?SummaryBodyDirty:PanelViewportDirty));}}
 if(mode_==UiMode::Keyboard&&contact_&&keyboardPressedKey_==24&&!dragged_){uint32_t held=elapsed(now/1000,pressAtMs_);if(held>=500&&elapsed(now/1000,lastBackspaceRepeatMs_)>=110){appendKey(24);lastBackspaceRepeatMs_=now/1000;keyboardRepeatStarted_=true;}}
 if(contact_&&touchY_>=24&&touchY_<=52&&elapsed(now/1000,pressAtMs_)>=3000&&mode_!=UiMode::Summary){strncpy(edit_,originalEdit_,sizeof(edit_)-1);edit_[sizeof(edit_)-1]=0;contact_=false;stableLayer_=StableLayer::Category;mode_=UiMode::Summary;selection_.cancel();invalidation_.add(BackgroundDirty|HeaderDirty|SummaryBodyDirty|OverlayDirty);}
 if(contact_){uint32_t held=elapsed(now/1000,pressAtMs_);uint16_t progress=selection_.holding()?selection_.holdProgress(now/1000):static_cast<uint16_t>(min(1000UL,held*1000UL/450UL));halo_.setTouchFeedback(true,touchX_,touchY_,progress,dragged_);if(stableLayer_==StableLayer::Menu&&mode_==UiMode::Expanded&&!rowHoldConsumed_){int activated=selection_.update(now/1000,actionAt(touchX_,touchY_)==selection_.pressed(),true);if(activated>=0){rowHoldConsumed_=true;contact_=false;touchPhase_=TouchPhase::WaitForRelease;gestures_.cancel();inputBarrier_.open(++inputGeneration_);TouchEvent event{};event.sequenceId=touchSequence_;event.x=touchX_;event.y=touchY_;logTouchTransition(event,"ROW_EXECUTE");executeRow(activated);}}if(elapsed(now,lastFeedbackUs_)>=41666){lastFeedbackUs_=now;invalidation_.add(FeedbackDirty,touchY_>18?touchY_-18:0,touchY_<221?touchY_+18:239);}else counters_.skippedFrames++;}else halo_.setTouchFeedback(false,0,0,0,false);
 DirtyRows dirtyRows;uint16_t dirty=invalidation_.take(dirtyRows);if(dirty)compose(dirty,dirtyRows);halo_.tick(now);counters_.minFreeHeap=min(counters_.minFreeHeap,ESP.getFreeHeap());if(elapsed(now,lastDiagnosticUs_)>=5000000){lastDiagnosticUs_=now;const uint32_t bytesPerSecond=(counters_.bytesTransferred-lastDiagnosticBytes_)/5;lastDiagnosticBytes_=counters_.bytesTransferred;Serial.printf("ui: full=%lu viewport=%lu rows=%lu feedback=%lu bytes/s=%lu compose=%lu/%luus transfer=%lu/%luus skipped=%lu touchmax=%lums heap=%lu/%lu\n",(unsigned long)counters_.fullRedraws,(unsigned long)counters_.viewportRedraws,(unsigned long)counters_.rowRedraws,(unsigned long)counters_.feedbackRedraws,(unsigned long)bytesPerSecond,(unsigned long)(counters_.transfers?counters_.composeTotalUs/counters_.transfers:0),(unsigned long)counters_.composeMaxUs,(unsigned long)(counters_.transfers?counters_.transferTotalUs/counters_.transfers:0),(unsigned long)counters_.transferMaxUs,(unsigned long)counters_.skippedFrames,(unsigned long)counters_.maxTouchIntervalMs,(unsigned long)ESP.getFreeHeap(),(unsigned long)counters_.minFreeHeap);}}
void RigUi::handleTouch(const TouchEvent &e) {
  if (e.kind == TouchKind::None) return;
  touchX_ = e.x;
  touchY_ = e.y;
  const bool released = e.kind == TouchKind::Released || e.kind == TouchKind::Tap ||
      e.kind == TouchKind::SwipeLeft || e.kind == TouchKind::SwipeRight ||
      e.kind == TouchKind::SwipeUp || e.kind == TouchKind::SwipeDown ||
      e.kind == TouchKind::Cancelled;

  if (mode_ != UiMode::Keyboard && mode_ != UiMode::Confirm &&
      inputBarrier_.blocking()) {
    if (released) inputBarrier_.released(e.timestampMs);
    else if (e.kind == TouchKind::PressStarted && inputBarrier_.acceptsPress(e.timestampMs))
      inputBarrier_.consumeFreshPress();
    else return;
  }

  if (mode_ == UiMode::Keyboard) {
    if (inputBarrier_.blocking()) {
      if (released) inputBarrier_.released(e.timestampMs);
      else if (e.kind == TouchKind::PressStarted && inputBarrier_.acceptsPress(e.timestampMs))
        inputBarrier_.consumeFreshPress();
      else return;
    }
    if (e.kind == TouchKind::PressStarted) {
      keyboardTouchActive_ = true;
      dragged_ = false;
      pressAtMs_ = e.timestampMs;
      keyboardPressedKey_ = keyboardKeyAt(e.x, e.y);
      keyboardRepeatStarted_ = false;
      lastBackspaceRepeatMs_ = e.timestampMs;
      contact_ = true;
      return;
    }
    if (e.kind == TouchKind::DragStarted || e.kind == TouchKind::DragMoved) {
      dragged_ = true;
      keyboardPressedKey_ = -1;
      return;
    }
    if (released) {
      const int key = keyboardKeyAt(e.x, e.y);
      contact_ = false;
      if (keyboardTouchActive_ && !dragged_ && key >= 0 && key == keyboardPressedKey_) {
        if (key == 30) closeModal(false);
        else if (key != 24 || !keyboardRepeatStarted_) appendKey(key);
      }
      keyboardTouchActive_ = false;
      keyboardPressedKey_ = -1;
      keyboardRepeatStarted_ = false;
    }
    return;
  }

  if (mode_ == UiMode::Confirm) {
    if (inputBarrier_.blocking()) {
      if (released) inputBarrier_.released(e.timestampMs);
      else if (e.kind == TouchKind::PressStarted && inputBarrier_.acceptsPress(e.timestampMs))
        inputBarrier_.consumeFreshPress();
      else return;
    }
    if (e.kind == TouchKind::Tap && e.y >= 24 && e.y <= 52) {
      closeModal(false);
    } else if (e.y >= 150 && e.y <= 190) {
      if (e.kind == TouchKind::Tap && e.x < 120) closeModal(false);
      else if (e.kind == TouchKind::HoldStarted && e.x >= 120) {
        if (page_ == UiPage::Network && confirmRow_ == 8) queue(UiAction::ForgetWifi);
        else if (page_ == UiPage::Network && confirmRow_ == 9) queue(UiAction::ClearWifiOverride);
        else if (confirmRow_ == 4) queue(UiAction::ClearOverrides);
        else queue(UiAction::Reboot);
        closeModal(true);
      }
    }
    return;
  }

  if (e.kind == TouchKind::PressStarted) {
    touchSequence_ = e.sequenceId;
    touchPhase_ = TouchPhase::Pressed;
    contact_ = true;
    dragged_ = false;
    scrollDragStarted_ = false;
    rowHoldConsumed_ = false;
    pressAtMs_ = e.timestampMs;
    const int row = stableLayer_ == StableLayer::Menu ? actionAt(e.x, e.y) : -1;
    if (stableLayer_ == StableLayer::Category)
      touchOwner_ = categoryOpenControl(e.x, e.y) ? TouchOwner::CategoryOpenControl : TouchOwner::CategoryPager;
    else
      touchOwner_ = row >= 0 ? TouchOwner::MenuRow : TouchOwner::MenuScroller;
    selection_.begin(row, e.timestampMs,
                     selection_.settled(e.timestampMs) && !scroll_.moving());
    pressedRow_ = selection_.holding() ? row : -1;
    const bool navigationControl = stableLayer_ == StableLayer::Menu
        ? e.y >= 24 && e.y <= 52 : categoryOpenControl(e.x, e.y);
    gestures_.begin(stableLayer_, static_cast<uint8_t>(page_), e.x, e.y,
                    e.timestampMs, scroll_.offset(), navigationControl, row);
    logTouchTransition(e, "DOWN");
    invalidation_.add(FeedbackDirty, e.y > 18 ? e.y - 18 : 0,
                      e.y < 221 ? e.y + 18 : 239);
    return;
  }

  if (e.kind == TouchKind::HoldStarted && stableLayer_ == StableLayer::Category &&
      touchOwner_ == TouchOwner::CategoryOpenControl && gestures_.active()) {
    touchPhase_ = TouchPhase::HoldFired; gestures_.cancel(); contact_ = false;
    requestOpenMenu(e.sequenceId); logTouchTransition(e, "HOLD_OPEN"); return;
  }
  if (e.kind == TouchKind::HoldStarted && stableLayer_ == StableLayer::Menu &&
      gestures_.active() && gestures_.session().startedInMenuHeader) {
    touchPhase_ = TouchPhase::HoldFired; gestures_.cancel(); selection_.cancel();
    contact_ = false; requestCloseMenu(e.sequenceId); logTouchTransition(e,"HOLD_BACK"); return;
  }
  if (e.kind == TouchKind::Cancelled) {
    touchPhase_ = TouchPhase::Cancelled;
    gestures_.cancel(); selection_.cancel();
    if (scrollDragStarted_) scroll_.stopAndSnap(30);
    mode_ = stableLayer_ == StableLayer::Menu ? UiMode::Expanded : UiMode::Summary;
    contact_ = dragged_ = scrollDragStarted_ = false; pressedRow_ = -1;
    invalidation_.add(FeedbackDirty); return;
  }

  if (e.kind == TouchKind::DragStarted || e.kind == TouchKind::DragMoved) {
    selection_.moved(e.totalX, e.totalY);
    const GestureOwner owner = gestures_.move(e.x, e.y, e.velocityX,
                                               e.velocityY, e.timestampMs);
    dragged_ = owner != GestureOwner::None;
    if(owner==GestureOwner::CategoryHorizontal)touchPhase_=TouchPhase::DraggingHorizontal;
    else if(owner==GestureOwner::CategoryMenuOpen||owner==GestureOwner::MenuScroll||owner==GestureOwner::MenuClose)touchPhase_=TouchPhase::DraggingVertical;
    if (!selection_.holding()) pressedRow_ = -1;
    if (owner == GestureOwner::MenuScroll) {
      if (!scrollDragStarted_) { scroll_.beginDrag(); scrollDragStarted_ = true; mode_ = UiMode::Scrolling; }
      scroll_.drag(e.deltaY);
      selection_.setHighlighted(scroll_.rowAt(120,58,30,rowCount()));
      invalidation_.add(ScrollDirty);
    } else if (owner != GestureOwner::None) invalidation_.add(FeedbackDirty);
    return;
  }
  if (!released || !gestures_.active()) return;

  const bool headerTap = stableLayer_ == StableLayer::Menu &&
      gestures_.session().startedInMenuHeader &&
      e.totalX >= -8 && e.totalX <= 8 && e.totalY >= -8 && e.totalY <= 8;
  const int releaseRow = stableLayer_ == StableLayer::Menu ? actionAt(e.x,e.y) : -1;
  const GestureResolution resolution = gestures_.release(e.x,e.y,e.velocityX,e.velocityY,releaseRow);
  contact_ = false; pressedRow_ = -1;
  if (headerTap) {
    selection_.cancel(); requestCloseMenu(e.sequenceId); inputBarrier_.released(e.timestampMs);
    scrollDragStarted_ = false; return;
  }
  switch (resolution) {
    case GestureResolution::CategoryPrevious: changePage(-1); break;
    case GestureResolution::CategoryNext: changePage(1); break;
    case GestureResolution::OpenMenu: requestOpenMenu(e.sequenceId); inputBarrier_.released(e.timestampMs); break;
    case GestureResolution::KeepMenu:
      stableLayer_ = StableLayer::Menu;
      if (scrollDragStarted_) scroll_.stopAndSnap(30);
      selection_.setHighlighted(scroll_.rowAt(120,58,30,rowCount()));
      selection_.release(releaseRow,e.timestampMs,scrollDragStarted_);
      mode_ = UiMode::Expanded;
      invalidation_.add(PanelViewportDirty);
      break;
    case GestureResolution::CloseMenu: selection_.cancel(); requestCloseMenu(e.sequenceId); inputBarrier_.released(e.timestampMs); break;
    case GestureResolution::ActivateAction:
    case GestureResolution::None:
    case GestureResolution::Cancelled:
      selection_.release(releaseRow,e.timestampMs,false);
      mode_ = stableLayer_ == StableLayer::Menu ? UiMode::Expanded : UiMode::Summary;
      invalidation_.add(FeedbackDirty);
      break;
  }
  scrollDragStarted_ = false;
  touchPhase_=TouchPhase::Idle;touchOwner_=TouchOwner::None;logTouchTransition(e,"RESOLVE");
}

void RigUi::compose(uint16_t dirty,const DirtyRows&dirtyRows){
 uint32_t start=micros();Arduino_GFX&g=compositor_.ready()?compositor_.canvas():display_.gfx();
 const bool rebuild=(dirty&(BackgroundDirty|HeaderDirty|SummaryBodyDirty|PanelViewportDirty|RowDirty|ScrollDirty|OverlayDirty|FeedbackDirty))!=0;
 if(rebuild){if(compositor_.ready())compositor_.clear(expandedView_,theme::kBlack);else for(int y=0;y<240;y++){auto span=expandedView_.span(y);if(span.valid())g.drawFastHLine(span.left,y,span.width(),theme::kBlack);}
  if(mode_==UiMode::Summary)drawSummary(g);else if(mode_==UiMode::Keyboard)drawKeyboard(g);else if(mode_==UiMode::Confirm)drawConfirm(g);else drawPanel(g);if(contact_)drawFeedback(g);
#if RIG_UI_SHOW_SAFE_AREAS
  g.drawCircle(120,120,106,theme::kAmber);g.drawCircle(120,120,100,theme::kCyan);
#endif
  if(dirty&FeedbackDirty)counters_.feedbackRedraws++;}
 counters_.composeUs=micros()-start;counters_.composeTotalUs+=counters_.composeUs;counters_.composeMaxUs=max(counters_.composeMaxUs,counters_.composeUs);if(compositor_.ready()&&dirtyRows.valid()){start=micros();const uint32_t bytes=compositor_.present(expandedView_,dirtyRows.first,dirtyRows.last);counters_.transferUs=micros()-start;counters_.transferTotalUs+=counters_.transferUs;counters_.transferMaxUs=max(counters_.transferMaxUs,counters_.transferUs);counters_.bytesTransferred+=bytes;counters_.transfers++;if(dirtyRows.first==0&&dirtyRows.last==239)counters_.fullRedraws++;else{counters_.viewportRedraws++;counters_.rowRedraws+=dirtyRows.last-dirtyRows.first+1;}}
}
void RigUi::drawSummary(Arduino_GFX&g){text(g,pageName(),120,25,2,theme::kWhite,true);separator(g,47,accent());text(g,stateLabel(),120,75,3,accent(),true);char secondary[64],diagnostic[72];if(page_==UiPage::Status){snprintf(secondary,sizeof(secondary),"%.40s",snapshot_.detail);snprintf(diagnostic,sizeof(diagnostic),"WIFI %s  RETRY %us",snapshot_.wifiConnected?"READY":"UNAVAILABLE",snapshot_.retrySeconds);}else if(page_==UiPage::Network){snprintf(secondary,sizeof(secondary),"%.32s",snapshot_.wifiSsid[0]?snapshot_.wifiSsid:"SSID NOT SET");snprintf(diagnostic,sizeof(diagnostic),"%s %s SRC %s",wifiStageLabel(snapshot_.wifiStage),snapshot_.wifiReasonText,snapshot_.credentialSource);}else if(page_==UiPage::Device){snprintf(secondary,sizeof(secondary),"XIAO C3");snprintf(diagnostic,sizeof(diagnostic),"SD %s CFG %s SRC %s TOKEN %s",snapshot_.sdReady?"OK":"--",snapshot_.configParsed?"OK":"BAD",snapshot_.credentialSource,snapshot_.tokenConfigured?"SET":"--");}else{snprintf(secondary,sizeof(secondary),"%s",snapshot_.sessionPresent?"LIVE SESSION":"NO SESSION");snprintf(diagnostic,sizeof(diagnostic),"TAP OR HOLD CENTER");}text(g,secondary,120,126,1,theme::kWhite,true);text(g,diagnostic,120,158,1,theme::kMuted,true);}
void RigUi::drawPanel(Arduino_GFX&g){char header[24];snprintf(header,sizeof(header),"< BACK  %s",pageName());text(g,header,120,25,1,theme::kWhite,true);if(page_==UiPage::Network)text(g,snapshot_.lanReady?snapshot_.ipAddress:snapshot_.hostname,120,40,1,snapshot_.lanReady?theme::kGreen:theme::kMuted,true);separator(g,52,accent());const char*const*rows=page_==UiPage::Status?kStatus:page_==UiPage::Network?kNetwork:page_==UiPage::Device?kDevice:kControls;int count=rowCount();int rowHeight=30;for(int i=0;i<count;i++){int y=58+i*rowHeight-static_cast<int>(scroll_.offset());if(mode_==UiMode::Expanding)y+=(1000-transitionProgress_)*90/1000;else if(mode_==UiMode::Collapsing)y+=transitionProgress_*90/1000;if(y<52||y>218)continue;bool disabled=page_==UiPage::Help&&((i==1&&(!snapshot_.sessionPresent||!snapshot_.tokenConfigured||snapshot_.state==RigState::Recording||snapshot_.requestInProgress))||(i==2&&(!snapshot_.sessionPresent||!snapshot_.tokenConfigured||snapshot_.state!=RigState::Recording||snapshot_.requestInProgress)));auto span=interactiveView_.span(y+8);if(!span.valid())continue;const bool highlighted=selection_.highlighted()==i;uint16_t color=disabled?theme::kMuted:(page_==UiPage::Device&&(i==4||i==7)||page_==UiPage::Network&&i==8)?theme::kAmber:theme::kWhite;if(highlighted){g.drawFastHLine(span.left+6,y+25,span.width()-12,accent());color=theme::kWhite;}g.fillCircle(span.left+8,y+7,2,(pressedRow_==i&&selection_.holding())?theme::kWhite:accent());text(g,rows[i],span.left+16,y+2,1,color,false);if(disabled){const char*reason=!snapshot_.sessionPresent?"NO LIVE SESSION":!snapshot_.tokenConfigured?"TOKEN REQUIRED":snapshot_.requestInProgress?"REQUEST IN PROGRESS":i==1?"ALREADY RECORDING":"NOT RECORDING";text(g,reason,span.left+16,y+13,1,theme::kMuted,false);}if(pressedRow_==i&&selection_.holding()){int width=(span.width()-12)*selection_.holdProgress(lastTickUs_/1000)/1000;g.drawFastHLine(span.left+6,y+27,width,theme::kWhite);}separator(g,y+28,theme::kPanel);}}

void RigUi::drawKeyboard(Arduino_GFX&g){text(g,"CANCEL",120,28,1,theme::kAmber,true);char shown[29];if(masked_&&!reveal_){size_t n=min(strlen(edit_),sizeof(shown)-1);memset(shown,'*',n);shown[n]=0;}else snprintf(shown,sizeof(shown),"%.28s",edit_);text(g,shown,120,57,1,theme::kWhite,true);separator(g,76,accent());static const char lower[]="abcdefghijklmnopqrstuvwx";static const char upper[]="ABCDEFGHIJKLMNOPQRSTUVWX";static const char symbol[]="1234567890-_=+./:?@#$%^&";const char*keys=layout_==0?lower:layout_==1?upper:symbol;for(int i=0;i<24;i++){int row=i/6,y=84+row*23;auto span=interactiveView_.span(y+8);int cell=span.width()/6;int x=span.left+(i%6)*cell+cell/2;char label[2]={keys[i],0};text(g,label,x,y,1,theme::kWhite,true);}text(g,"DEL",67,191,1,theme::kAmber,true);text(g,layout_==0?"ABC":layout_==1?"abc":"123",103,191,1,theme::kMuted,true);text(g,"SPACE",139,191,1,theme::kMuted,true);text(g,"DONE",177,191,1,accent(),true);}

void RigUi::drawConfirm(Arduino_GFX&g){drawPanel(g);text(g,"CANCEL",120,28,1,theme::kGreen,true);for(int y=45;y<220;y+=3){auto s=expandedView_.span(y);if(s.valid())for(int x=s.left;x<=s.right;x+=4)g.drawPixel(x,y,theme::kBlack);}text(g,"CONFIRM",120,72,2,theme::kAmber,true);text(g,"THIS CANNOT BE UNDONE",120,112,1,theme::kWhite,true);text(g,"CANCEL",70,169,1,theme::kGreen,true);text(g,"CONFIRM",169,169,1,theme::kRed,true);}
void RigUi::drawFeedback(Arduino_GFX&g){if(!contact_)return;if(mode_==UiMode::Summary&&touchY_<65)separator(g,48,theme::kWhite);else if(pressedRow_>=0){int y=58+pressedRow_*30-static_cast<int>(scroll_.offset());auto s=expandedView_.span(y+10);if(s.valid()){const int feedbackWidth=static_cast<int>(s.width())-10;if(feedbackWidth>0)g.drawFastHLine(s.left+5,y+25,feedbackWidth,accent());}}}
void RigUi::text(Arduino_GFX&g,const char*t,int x,int y,uint8_t size,uint16_t color,bool centered){if(!t)return;auto span=(mode_==UiMode::Summary?summaryView_:expandedView_).span(y+size*4);if(!span.valid())return;char clipped[48];snprintf(clipped,sizeof(clipped),"%.47s",t);int maxChars=static_cast<int>(span.width())/(6*size);if(maxChars<1)maxChars=1;if((int)strlen(clipped)>maxChars)clipped[maxChars]=0;g.setTextSize(size);g.setTextColor(color);if(centered){int16_t x1,y1;uint16_t w,h;g.getTextBounds(clipped,0,y,&x1,&y1,&w,&h);{int centeredX=(240-static_cast<int>(w))/2-x1;x=centeredX<span.left?span.left:centeredX;}}else x=x<span.left?span.left:x;g.setCursor(x,y);g.print(clipped);}
void RigUi::separator(Arduino_GFX&g,int y,uint16_t color){auto s=(mode_==UiMode::Summary?summaryView_:expandedView_).span(y);if(s.valid()){const int separatorWidth=static_cast<int>(s.width())-16;if(separatorWidth>0)g.drawFastHLine(s.left+8,y,separatorWidth,color);}}
int RigUi::rowCount()const{return page_==UiPage::Status?4:page_==UiPage::Network?10:page_==UiPage::Device?8:5;}
int RigUi::actionAt(uint16_t x,uint16_t y)const{if(!interactiveView_.contains(x,y)||y<54)return-1;return scroll_.rowAt(y,58,30,rowCount());}
int RigUi::keyboardKeyAt(uint16_t x,uint16_t y)const{if(!interactiveView_.contains(x,y))return-1;if(y>=24&&y<=52)return 30;if(y>=82&&y<178){auto span=interactiveView_.span(y);if(!span.valid())return-1;int row=(y-82)/23;if(row>3)return-1;int col=(x-span.left)*6/span.width();return col>=0&&col<6?row*6+col:-1;}if(y>=184&&y<=204){if(x>=56&&x<88)return 24;if(x>=88&&x<120)return 27;if(x>=120&&x<152)return 25;if(x>=152&&x<=184)return 29;}return-1;}

void RigUi::appendKey(int key){static const char lower[]="abcdefghijklmnopqrstuvwx",upper[]="ABCDEFGHIJKLMNOPQRSTUVWX",symbols[]="1234567890-_=+./:?@#$%^&";if(key>=0&&key<24){const char*m=layout_==0?lower:layout_==1?upper:symbols;size_t n=strlen(edit_);if(n<sizeof(edit_)-1){edit_[n]=m[key];edit_[n+1]=0;}}else if(key==24){size_t n=strlen(edit_);if(n)edit_[n-1]=0;}else if(key==25){size_t n=strlen(edit_);if(n<sizeof(edit_)-1){edit_[n]=' ';edit_[n+1]=0;}}else if(key==27)layout_=(layout_+1)%3;else if(key==28)reveal_=!reveal_;else if(key==29){if(!validateDraft()){halo_.setOutcomeFeedback(false);invalidation_.add(FeedbackDirty);return;}command_=UiCommand();command_.action=UiAction::SaveConfig;command_.field=editField_;strncpy(command_.value,edit_,sizeof(command_.value)-1);command_.value[sizeof(command_.value)-1]=0;commandReady_=true;closeModal(true);}invalidation_.add(OverlayDirty);}
void RigUi::openEditor(ConfigField f,const char*v,bool mask){editField_=f;strncpy(originalEdit_,v?v:"",sizeof(originalEdit_)-1);originalEdit_[sizeof(originalEdit_)-1]=0;strncpy(edit_,originalEdit_,sizeof(edit_)-1);edit_[sizeof(edit_)-1]=0;masked_=mask;reveal_=false;layout_=0;modalReturnMode_=UiMode::Expanded;mode_=UiMode::Keyboard;keyboardTouchActive_=false;keyboardPressedKey_=-1;inputBarrier_.open(++inputGeneration_);invalidation_.add(BackgroundDirty|OverlayDirty);}
bool RigUi::validateDraft()const{const size_t n=strlen(edit_);if(editField_==ConfigField::WifiSsid)return n>=1&&n<=32;if(editField_==ConfigField::WifiPassword)return n==0||(n>=8&&n<=63);if(editField_==ConfigField::ApiBase)return (strncmp(edit_,"http://",7)==0&&n>7)||(strncmp(edit_,"https://",8)==0&&n>8);if(editField_==ConfigField::NodeId)return n>=1&&n<=48;return n>0;}
void RigUi::closeModal(bool committed){if(!committed){strncpy(edit_,originalEdit_,sizeof(edit_)-1);edit_[sizeof(edit_)-1]=0;}mode_=modalReturnMode_;contact_=false;selection_.cancel();inputBarrier_.open(++inputGeneration_);inputBarrier_.released(lastTickUs_/1000);invalidation_.add(BackgroundDirty|PanelViewportDirty|OverlayDirty);}

void RigUi::executeRow(int r){if(page_==UiPage::Status){if(r==0)queue(UiAction::RetryWifi);else if(r==1)queue(UiAction::PollApi);else if(r==2)queue(UiAction::ClearError);else if(r==3)requestCloseMenu(touchSequence_);}else if(page_==UiPage::Network){if(r==0)queue(UiAction::RetryWifi);else if(r==1)queue(UiAction::ScanWifi);else if(r==2)queue(UiAction::SelectWifi);else if(r==3)openEditor(ConfigField::WifiSsid,snapshot_.wifiSsid,false);else if(r==4)openEditor(ConfigField::WifiPassword,"",true);else if(r==5)openEditor(ConfigField::ApiBase,snapshot_.apiBase,false);else if(r==6)queue(UiAction::TestApi);else if(r==7)queue(UiAction::DisconnectWifi);else if(r==8||r==9){confirmRow_=r;modalReturnMode_=UiMode::Expanded;mode_=UiMode::Confirm;inputBarrier_.open(++inputGeneration_);invalidation_.add(OverlayDirty);}}else if(page_==UiPage::Device){if(r==0)openEditor(ConfigField::NodeId,snapshot_.nodeId,false);else if(r==1)openEditor(ConfigField::BearerToken,"",true);else if(r==2)openEditor(ConfigField::ApiBase,snapshot_.apiBase,false);else if(r==3)queue(UiAction::ReloadSd);else if(r==4||r==7){confirmRow_=r;modalReturnMode_=UiMode::Expanded;mode_=UiMode::Confirm;inputBarrier_.open(++inputGeneration_);invalidation_.add(OverlayDirty);}else if(r==5)queue(UiAction::DisplayTest);else if(r==6)queue(UiAction::TouchTest);}else{bool startOk=r==1&&snapshot_.sessionPresent&&snapshot_.tokenConfigured&&snapshot_.state!=RigState::Recording&&!snapshot_.requestInProgress;bool stopOk=r==2&&snapshot_.sessionPresent&&snapshot_.tokenConfigured&&snapshot_.state==RigState::Recording&&!snapshot_.requestInProgress;if(r==0)queue(UiAction::PollApi);else if(startOk)queue(UiAction::StartRecording);else if(stopOk)queue(UiAction::StopRecording);else if(r==3)queue(UiAction::ClearError);else if(r==4)showStatus();else{halo_.setOutcomeFeedback(false);invalidation_.add(FeedbackDirty);}}}
bool RigUi::requestOpenMenu(uint32_t sequence){if(stableLayer_==StableLayer::Menu||mode_==UiMode::Expanding||mode_==UiMode::Expanded||mode_==UiMode::Scrolling)return false;inputBarrier_.open(++inputGeneration_);touchSequence_=sequence;touchPhase_=TouchPhase::WaitForRelease;stableLayer_=StableLayer::Menu;mode_=UiMode::Expanding;halo_.setViewMode(mode_,micros());animationAtUs_=micros();transitionProgress_=0;int count=rowCount();scroll_.configure(166,count*30);selection_.setHighlighted(scroll_.rowAt(120,58,30,count));invalidation_.add(BackgroundDirty|PanelViewportDirty);return true;}
bool RigUi::requestCloseMenu(uint32_t sequence){if(stableLayer_==StableLayer::Category||mode_==UiMode::Collapsing||mode_==UiMode::Summary)return false;inputBarrier_.open(++inputGeneration_);touchSequence_=sequence;touchPhase_=TouchPhase::WaitForRelease;stableLayer_=StableLayer::Category;mode_=UiMode::Collapsing;halo_.setViewMode(mode_,micros());animationAtUs_=micros();transitionProgress_=0;invalidation_.add(BackgroundDirty|PanelViewportDirty);return true;}
bool RigUi::categoryOpenControl(uint16_t x,uint16_t y)const{const int dx=int(x)-120,dy=int(y)-120;return dx*dx+dy*dy<=52*52;}
void RigUi::logTouchTransition(const TouchEvent&e,const char*event)const{const bool up=e.kind==TouchKind::Released||e.kind==TouchKind::Tap||e.kind==TouchKind::SwipeLeft||e.kind==TouchKind::SwipeRight||e.kind==TouchKind::SwipeUp||e.kind==TouchKind::SwipeDown;const unsigned raw=e.kind==TouchKind::Cancelled?2U:(up?0U:1U);Serial.printf("touch seq=%lu raw=%u stable=%u phase=%u owner=%u x=%u y=%u dx=%d dy=%d row=%d ui=%u event=%s\n",(unsigned long)e.sequenceId,raw,up?0U:1U,(unsigned)touchPhase_,(unsigned)touchOwner_,e.x,e.y,e.totalX,e.totalY,pressedRow_,(unsigned)mode_,event);}
void RigUi::changePage(int direction){int p=(static_cast<int>(page_)+direction+4)%4;page_=static_cast<UiPage>(p);halo_.setCategory(page_,micros());animationAtUs_=micros();invalidation_.add(BackgroundDirty|HeaderDirty|SummaryBodyDirty);}
void RigUi::cyclePage(){changePage(1);}void RigUi::showStatus(){stableLayer_=StableLayer::Category;page_=UiPage::Status;halo_.setCategory(page_,micros());halo_.setViewMode(UiMode::Summary,micros());mode_=UiMode::Summary;contact_=false;scroll_.configure(1,1);invalidation_.add(BackgroundDirty|HeaderDirty|SummaryBodyDirty|OverlayDirty);}void RigUi::queue(UiAction a){command_={};command_.action=a;commandReady_=true;}bool RigUi::takeCommand(UiCommand&c){if(!commandReady_)return false;c=command_;commandReady_=false;return true;}
const char*RigUi::pageName()const{return kNames[static_cast<int>(page_)];}const char*RigUi::stateLabel()const{switch(snapshot_.state){case RigState::Booting:return"BOOT";case RigState::WifiConnecting:return"JOINING";case RigState::WifiOffline:return"OFFLINE";case RigState::ApiOffline:return"API OFFLINE";case RigState::NoSession:return"RIG READY";case RigState::Previewing:return"STANDBY";case RigState::Recording:return"REC";case RigState::Sending:return"SENDING";case RigState::Error:return"ERROR";}return"UNKNOWN";}uint16_t RigUi::accent()const{if(snapshot_.state==RigState::Error||snapshot_.state==RigState::Recording)return theme::kRed;if(snapshot_.state==RigState::NoSession||snapshot_.state==RigState::Previewing)return theme::kGreen;return page_==UiPage::Help?theme::kViolet:page_==UiPage::Device?theme::kCyan:theme::kAmber;}
}

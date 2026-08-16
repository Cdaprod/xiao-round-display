#include <cassert>

#include "../src/input/GestureRecognizer.h"
#include "../src/input/TouchLifecycle.h"

using namespace rig;

int main() {
  StableTouchFilter touch;

  assert(touch.update(RawTouchState::Pressed, 0) == StableTouchTransition::None);
  assert(touch.update(RawTouchState::Pressed, 19) == StableTouchTransition::None);
  assert(touch.update(RawTouchState::Pressed, 20) == StableTouchTransition::Down);
  assert(touch.sequence() == 1);
  assert(touch.fireHoldOnce());
  assert(!touch.fireHoldOnce());

  // An unreadable sample cannot release or fragment the active sequence.
  assert(touch.update(RawTouchState::Unknown, 30) == StableTouchTransition::None);
  assert(touch.pressed());
  assert(touch.sequence() == 1);
  assert(touch.update(RawTouchState::Released, 40) == StableTouchTransition::None);
  assert(touch.update(RawTouchState::Pressed, 80) == StableTouchTransition::None);
  assert(touch.sequence() == 1);

  assert(touch.update(RawTouchState::Released, 100) == StableTouchTransition::None);
  assert(touch.update(RawTouchState::Released, 169) == StableTouchTransition::None);
  assert(touch.update(RawTouchState::Released, 170) == StableTouchTransition::Up);
  assert(!touch.pressed());

  assert(touch.update(RawTouchState::Pressed, 200) == StableTouchTransition::None);
  assert(touch.update(RawTouchState::Pressed, 220) == StableTouchTransition::Down);
  assert(touch.sequence() == 2);
  assert(touch.fireHoldOnce());

  GestureRecognizer recognizer;
  assert(recognizer.sample(true, true, 120, 120, 0, TouchZone::Category).kind ==
         TouchKind::PressStarted);
  assert(recognizer.sample(true, true, 120, 120, 1199, TouchZone::Category).kind ==
         TouchKind::None);
  assert(recognizer.sample(true, true, 120, 120, 1200, TouchZone::Category).kind ==
         TouchKind::HoldStarted);
  assert(recognizer.sample(true, true, 120, 120, 1500, TouchZone::Category).kind ==
         TouchKind::None);
  assert(recognizer.sample(false, true, 120, 120, 1501, TouchZone::Category).kind ==
         TouchKind::Released);
  return 0;
}

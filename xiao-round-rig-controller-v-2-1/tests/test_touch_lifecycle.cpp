#include <cassert>

#include "../src/input/GestureRecognizer.h"
#include "../src/input/TouchLifecycle.h"

using namespace rig;

int main() {
  StableTouchFilter touch;

  assert(touch.update(RawTouchState::Pressed) == StableTouchTransition::None);
  assert(touch.update(RawTouchState::Pressed) == StableTouchTransition::Down);
  assert(touch.sequence() == 1);
  assert(touch.fireHoldOnce());
  assert(!touch.fireHoldOnce());

  // An unreadable sample cannot release or fragment the active sequence.
  assert(touch.update(RawTouchState::Unknown) == StableTouchTransition::None);
  assert(touch.pressed());
  assert(touch.sequence() == 1);
  assert(touch.update(RawTouchState::Released) == StableTouchTransition::None);
  assert(touch.update(RawTouchState::Pressed) == StableTouchTransition::None);
  assert(touch.sequence() == 1);

  assert(touch.update(RawTouchState::Released) == StableTouchTransition::None);
  assert(touch.update(RawTouchState::Released) == StableTouchTransition::None);
  assert(touch.update(RawTouchState::Released) == StableTouchTransition::Up);
  assert(!touch.pressed());

  assert(touch.update(RawTouchState::Pressed) == StableTouchTransition::None);
  assert(touch.update(RawTouchState::Pressed) == StableTouchTransition::Down);
  assert(touch.sequence() == 2);
  assert(touch.fireHoldOnce());

  GestureRecognizer recognizer;
  assert(recognizer.sample(true, true, 120, 120, 0, TouchZone::Category).kind ==
         TouchKind::PressStarted);
  assert(recognizer.sample(true, true, 120, 120, 449, TouchZone::Category).kind ==
         TouchKind::None);
  assert(recognizer.sample(true, true, 120, 120, 450, TouchZone::Category).kind ==
         TouchKind::HoldStarted);
  assert(recognizer.sample(true, true, 120, 120, 900, TouchZone::Category).kind ==
         TouchKind::None);
  assert(recognizer.sample(false, true, 120, 120, 901, TouchZone::Category).kind ==
         TouchKind::Released);
  return 0;
}

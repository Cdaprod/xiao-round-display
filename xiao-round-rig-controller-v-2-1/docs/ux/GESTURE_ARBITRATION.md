# Gesture arbitration

A touch captures the stable Category or Menu layer at contact. The first dominant movement beyond the 12 px slop locks one owner for the session; rendering previews and release resolution consult only that owner.

- Category accepts horizontal navigation and upward menu opening. A tap leaves the category unchanged.
- Menu accepts list scrolling, bounded action taps, and deliberate closing. It never accepts category navigation.
- Menu closing requires a downward header drag, or a downward gesture that began while the list was already at offset zero. Reaching zero during an ordinary scroll cannot change ownership.
- Release is not a close command. It commits or reverses the captured owner, and ordinary scrolling retains the menu.
- One invalid touch-controller sample is tolerated for 30 ms. Longer loss cancels rather than commits the session.

Thresholds and release semantics are implemented in `src/input/GestureArbitrator.h` and covered by the deterministic host test `tests/test_gesture_arbitration.cpp`.

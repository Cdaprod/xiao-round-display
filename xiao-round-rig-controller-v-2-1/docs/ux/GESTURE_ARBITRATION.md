# Gesture arbitration

A touch captures the stable Category or Menu layer at contact. The first dominant movement beyond the 12 px slop locks one owner for the session; rendering previews and release resolution consult only that owner.

- Category accepts horizontal navigation anywhere in the circular viewport. A center tap opens the selected menu; vertical movement and holding do nothing.
- Menu accepts vertical list scrolling and row holds. It never accepts category navigation or drag-to-close.
- A tap on the fixed header closes one level. A stationary 1200 ms header hold returns to Status.
- Release is not a close command. It commits or reverses the captured owner, and ordinary scrolling retains the menu.
- Press and release are stabilized for 20 ms and 70 ms respectively. Unreadable controller samples receive a 40 ms grace period.

Thresholds and release semantics are implemented in `src/input/GestureArbitrator.h` and covered by the deterministic host test `tests/test_gesture_arbitration.cpp`.

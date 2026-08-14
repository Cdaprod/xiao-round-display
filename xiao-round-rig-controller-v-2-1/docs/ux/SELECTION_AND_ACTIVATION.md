# Selection and activation

Menu interaction uses three independent states: highlight identifies focus, hold progress indicates a possible choice, and activation records the completed choice.

- Dragging owns scrolling after movement exceeds tolerance. It updates only the focused row.
- Release snaps the list, retains focus, and invokes no action.
- A short tap changes focus and invokes no action.
- Only a fresh, stationary 600 ms hold on the focused row invokes it, once.
- Moving more than 8 px, leaving the row, touch loss, or action invalidation cancels the hold permanently for that session.
- Modal opening consumes the source touch and requires confirmed release, 40 ms stabilization, and a fresh press.

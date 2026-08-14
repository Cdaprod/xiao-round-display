#include <cassert>
#include "../src/ui/CircularViewport.h"
#include "../src/ui/SelectionModel.h"

using namespace rig;

int main() {
  CircularViewport safe(106);
  assert(safe.containsRect(Rect(88, 24, 64, 28)));
  assert(safe.containsRect(Rect(56, 184, 32, 21)));
  assert(safe.containsRect(Rect(152, 184, 32, 21)));
  assert(!safe.containsRect(Rect(0, 0, 30, 30)));

  SelectionModel selection;
  selection.setHighlighted(3);
  selection.begin(3, 100, true);
  selection.release(3, 200, false);
  assert(selection.activated() == -1);
  selection.begin(3, 400, true);
  assert(selection.update(999, true, true) == -1);
  assert(selection.update(1000, true, true) == 3);
  assert(selection.update(1100, true, true) == -1);

  selection.begin(3, 1200, true);
  selection.moved(8, 0);
  assert(selection.update(2000, true, true) == -1);
  selection.release(4, 2000, true);
  selection.begin(4, 2100, selection.settled(2100));
  assert(selection.update(3000, true, true) == -1);
  selection.begin(4, 2200, selection.settled(2200));
  assert(selection.update(2800, true, false) == -1);

  InputBarrier barrier;
  barrier.open(7);
  assert(!barrier.acceptsPress(100));
  barrier.released(100);
  assert(!barrier.acceptsPress(139));
  assert(barrier.acceptsPress(140));
  assert(barrier.generation() == 7);

  NavigationStack stack;
  NavigationEntry entry;
  entry.layer = NavigationLayer::CategoryMenu;
  entry.category = 2;
  entry.highlightedRow = 4;
  assert(stack.push(entry));
  for (int i = 1; i < NavigationStack::kCapacity; ++i) assert(stack.push(entry));
  assert(!stack.push(entry));
  NavigationEntry restored;
  assert(stack.pop(restored));
  assert(restored.category == 2 && restored.highlightedRow == 4);
  return 0;
}

# Cyclic garbage. Every round builds a list that contains itself and a local
# function that calls itself, which is also a cycle: its closure holds the
# cell for `countdown`, and the cell holds the closure. Once a round returns,
# both cycles are unreachable. A tracing collector frees them; reference
# counting alone never would, because every object in a cycle stays referenced.
fn make_cycles(n) {
  fn countdown(k) { if (k == 0) { return 0; } return countdown(k - 1); }
  let ring = [n];
  push(ring, ring);
  return countdown(2) + len(ring);
}

let rounds = 20000;
let total = 0;
let i = 0;
while (i < rounds) {
  total = total + make_cycles(i);
  i = i + 1;
}
print("rounds:", rounds, "total:", total);

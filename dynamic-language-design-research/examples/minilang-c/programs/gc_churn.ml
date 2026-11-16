# A long run that makes garbage of every kind each round: strings, lists,
# nested lists, a closure and its cell. Only `keep`, a window of the last 8
# results, survives, so the live heap is the same size however many rounds
# run. The roadmap's acceptance test for stage 5: memory does not grow.
fn adder(n) { return fn(x) { return x + n; }; }

let rounds = 20000;
let keep = [0, 0, 0, 0, 0, 0, 0, 0];
let i = 0;
while (i < rounds) {
  let label = "round " + str(i);
  let parts = [label, [i, i * 2], adder(i)];
  keep[i % 8] = parts[2](len(parts[0]));
  i = i + 1;
}
print(keep);

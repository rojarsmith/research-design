# Benchmark 3: string concatenation. Builds 2,000 strings of 100 pieces each,
# so each string stays short and the time goes to the loop, the str() builtin
# call and string allocation rather than to copying one huge string.
fn build(count, pieces) {
  let total = 0;
  let k = 0;
  while (k < count) {
    let s = "";
    let i = 0;
    while (i < pieces) {
      s = s + str(i % 10) + ",";
      i = i + 1;
    }
    total = total + len(s);
    k = k + 1;
  }
  return total;
}
print(build(2000, 100));

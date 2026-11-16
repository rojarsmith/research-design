# Benchmark 2: a counting loop inside a function. Local variables, integer
# arithmetic and jumps; 1,000,000 iterations.
fn sum_to(n) {
  let total = 0;
  let i = 0;
  while (i < n) {
    total = total + i;
    i = i + 1;
  }
  return total;
}
print(sum_to(1000000));

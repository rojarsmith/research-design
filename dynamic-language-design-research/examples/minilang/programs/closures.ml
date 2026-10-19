# Closures capture variables, not values.
fn make_counter(step) {
  let count = 0;
  return fn() {
    count = count + step;
    return count;
  };
}

let by_one = make_counter(1);
let by_ten = make_counter(10);
print(by_one(), by_one(), by_ten(), by_one());

# Higher-order functions over lists.
fn map(xs, f) {
  let out = []; let i = 0;
  while (i < len(xs)) { push(out, f(xs[i])); i = i + 1; }
  return out;
}
print(map([1, 2, 3], fn(x) { return x * x; }));

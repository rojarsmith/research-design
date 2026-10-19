# Runtime errors become catchable values; uncaught ones stop the program with a line number.
fn parse_age(text) {
  let n = num(text);
  if (n < 0) { throw "age cannot be negative: " + str(n); }
  return n;
}

let inputs = ["42", "-3", "abc"];
let i = 0;
while (i < len(inputs)) {
  try {
    print("ok:", parse_age(inputs[i]));
  } catch (e) {
    print("error:", e);
  }
  i = i + 1;
}

print("now an uncaught error:");
let xs = [1, 2, 3];
print(xs[10]);
print("never printed");

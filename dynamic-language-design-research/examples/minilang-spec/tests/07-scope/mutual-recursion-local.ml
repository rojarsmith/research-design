fn main() {
  fn isEven(n) { if (n == 0) { return true; } return isOdd(n - 1); }
  fn isOdd(n) { if (n == 0) { return false; } return isEven(n - 1); }
  return [isEven(10), isOdd(7)];
}
print(main());

# ---
# spec: 7.3
# title: local functions may call each other in either order
# end: ok
#> [true, true]

let x = "outer";
fn test() {
  fn show() { return x; }
  let a = show();
  let x = "inner";
  print(a, show());
}
test();

# ---
# spec: 7.4
# title: a closure called before and after a later let of the same name
# level: full
# status: legacy P2
# end: ok
#> outer inner

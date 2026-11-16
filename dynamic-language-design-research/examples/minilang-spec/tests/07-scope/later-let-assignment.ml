let x = "global";
fn f() {
  fn set(v) { x = v; }
  set("a");
  let x = "local";
  set("b");
  print(x);
}
f();
print(x);

# ---
# spec: 7.4
# title: assignment from a closure follows the same rule
# level: full
# status: legacy P2
# end: ok
#> b
#> a

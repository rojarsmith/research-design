let x = 1;
{
  fn g() { return x; }
  print(g());
  let x = 2;
  print(g());
}
print(x);

# ---
# spec: 7.4
# title: the same in a top-level block
# level: full
# status: legacy P2
# end: ok
#> 1
#> 2
#> 1

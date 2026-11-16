let i = 0; let log = [];
while (i < 3) {
  let j = 0;
  while (j < 5) { if (j == 2) { break; } push(log, i * 10 + j); j = j + 1; }
  i = i + 1;
}
print(log);

# ---
# spec: 8.3
# title: break leaves the innermost loop only
# end: ok
#> [0, 1, 10, 11, 20, 21]

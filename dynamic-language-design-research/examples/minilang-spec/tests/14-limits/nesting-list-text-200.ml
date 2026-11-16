let a = []; let i = 0;
while (i < 200) { a = [a]; i = i + 1; }
print(len(str(a)));

# ---
# spec: 14.3
# title: a list nested 200 deep converts to text
# end: ok
#> 402

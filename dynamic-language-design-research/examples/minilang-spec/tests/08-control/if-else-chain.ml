fn classify(n) { if (n < 0) { return "neg"; } else if (n == 0) { return "zero"; } else { return "pos"; } }
print(classify(-3), classify(0), classify(8));

# ---
# spec: 8.1
# title: else if chains
# end: ok
#> neg zero pos

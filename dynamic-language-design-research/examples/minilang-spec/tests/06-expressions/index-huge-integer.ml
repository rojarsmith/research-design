print([1][100000000000000000000000]);

# ---
# spec: 6.4 12.1
# title: an out-of-range index of any size is an ordinary error
# level: full
# end: runtime-error 1
# message: index 100000000000000000000000 out of range for length 1

let t = [1, 2];
t[5]
  = 1;

# ---
# spec: 10.4 6.4
# title: an index assignment error is reported at the line of =
# end: runtime-error 3
# message: index 5 out of range for length 2

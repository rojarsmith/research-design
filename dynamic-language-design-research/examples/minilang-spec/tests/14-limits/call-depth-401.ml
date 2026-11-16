fn d(n) {
  if (n == 0) { return 0; }
  return 1 + d(n - 1);
}
print(d(400));

# ---
# spec: 14.1
# title: the 401st nested call is a stack overflow at the call's line
# level: full
# end: runtime-error 3
# message: stack overflow (max call depth 400)

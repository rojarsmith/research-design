print("never");
fn f() {
  print(1);

# ---
# spec: 3.5
# title: an unclosed block is reported at its opening brace
# end: syntax-error 2
# message: unexpected end of input, missing '}'

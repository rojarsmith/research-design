# Used by embed_ctypes.py, where Python is the host and py_upper() is a Python function.
fn shout(x) { return py_upper(x) + "!"; }
fn safe_shout(x) {
  try { return shout(x); } catch (e) { return "caught: " + e; }
}

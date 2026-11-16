# A report over a CSV file, driven by mlfilter.c.
# mlfilter calls keep(row) for every data row and report(rows) once with the
# rows that were kept. It provides three host functions written in C:
#   field(row, name)        the value of a named column (numbers already parsed)
#   sort_by(list, key_fn)   a new list sorted by key_fn(item); calls back into minilang
#   fmt(number, decimals)   fixed-point text, which minilang cannot produce itself

let threshold = 100;

fn keep(row) {
  let amount = field(row, "amount");
  if (amount < 0) {
    throw "negative amount " + str(amount);
  }
  return field(row, "status") == "paid" and amount >= threshold;
}

fn pad(text, width) {
  while (len(text) < width) { text = text + " "; }
  return text;
}

fn report(rows) {
  let sorted = sort_by(rows, fn(row) { return 0 - field(row, "amount"); });
  let total = 0;
  let i = 0;
  while (i < len(sorted)) {
    let row = sorted[i];
    print(pad(field(row, "customer"), 16) + pad(field(row, "country"), 4) + fmt(field(row, "amount"), 2));
    total = total + field(row, "amount");
    i = i + 1;
  }
  print("total " + fmt(total, 2) + " in " + str(len(sorted)) + " orders of at least " + str(threshold));
  return total;
}

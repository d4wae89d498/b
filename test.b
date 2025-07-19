g = 42;
h;
foo(a, b) {
  return a + b;
}
extern y;
extern bar;
main() {
  auto k;
  auto f;
  auto x;


(1+5)(41);
  k = "hello";
  x = 1 + 2;
  g = g + 1;
  x = h;
  y = x[3] + bar(x[1 + 1]);
  if (x > 2) x = x - 1;
  (x + 2)[0];
  y = *x;
  y = &x;
  y = *foo(x);
  y = &x[1];
  auto i;
  i = 0;
  while (i < 10) {
    if (i == 5) break;
    if (i == 2) {
      i = i + 1;
      continue;
    }
    i = i + 1;
  }
  i = 0;
  loop:
  i = i + 1;
  if (i < 5) goto loop;
  return i;
} 
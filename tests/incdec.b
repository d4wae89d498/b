main() {
    extern printf;
    auto i;
    auto j;
    i = 0;
    j = 0;
    ++i;
    printf("%d ", i); // EXPECTED 1
    i++;
    printf("%d ", i); // EXPECTED 2
    --i;
    printf("%d ", i); // EXPECTED 1
    i--;
    printf("%d ", i); // EXPECTED 0
    j = ++i;
    printf("%d %d ", i, j); // EXPECTED 1 1
    j = i++;
    printf("%d %d ", i, j); // EXPECTED 2 1
    j = --i;
    printf("%d %d ", i, j); // EXPECTED 1 1
    j = i--;
    printf("%d %d", i, j); // EXPECTED 0 1
}
// EXPECTED
// 1 2 1 0 1 1 2 1 1 1 0 1
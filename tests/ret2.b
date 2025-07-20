extern printf;

f() {
    printf("hello\n");
    return 8;
}

main() {
    auto k;

    k = f();
    return k;
}
// EXPECTED
// hello
main()
{
    extern printf;
    auto i;
    auto y;

    i = 0;
    while (i < 10)
    {
        printf("%d", i);
        y = i + 1;
        i = y = y;
    }
}

// EXPECTED
// 0123456789
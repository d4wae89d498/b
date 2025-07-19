main()
{
    extern printf;
    auto i;

    i = 0;
    while (i < 10)
    {
        printf("%d", i);
        i = i + 1;
    }
}

// EXPECTED
// 0123456789
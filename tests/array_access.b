main()
{
    auto str;
    extern printf;

    str = "hello";
    printf("%c %c %c %c %c", 
        str[0] & 255, 
        (str[0] >> 8) & 255, 
        (str[0] >> 16) & 255, 
        (str[0] >> 24) & 255, 
        str[1] & 255);
}

// EXPECTED
// h e l l o
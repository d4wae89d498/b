meta {
    extern printf;
    
    main() {
        auto k;

        printf("# [test from META %i] ... #\n", 42);
        
        return 21;
    }
}

main() {
    return 0;
} 
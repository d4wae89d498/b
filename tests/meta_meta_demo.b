meta {
    extern printf;
    main() {
        auto k;

        printf("# [test from META %i] ... #\n", 42);
        
        return 21;
    }

    meta {
        extern printf;
        
        main() {
            auto k;

            printf("# [test from META %i] ... #\n", 66);
            
            return 88;
        }
    }
}

main() {
    return 0;
} 
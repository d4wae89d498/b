#!/bin/bash
B_PARSER=./b
ASM=out.s
EXE=out
PASS=0
FAIL=0

for bfile in tests/*.b; do
    echo "Testing $bfile"
    # Extract expected output (strip leading // and whitespace)
    expected=$(awk '/^\/\/ EXPECTED/{flag=1; next} /^$/{flag=0} flag' "$bfile" | sed 's/^\/\/[ ]*//;s/^ *//;s/[\r\n]*$//')
    expected_exit=$(awk '/^\/\/ EXPECTED/{getline; if ($0 ~ /^\/\/[ ]*EXITCODE:/) print $0}' "$bfile" | sed 's/^\/\/[ ]*EXITCODE:[ ]*//')
    # Generate assembly filename based on input .b file
    asm_file="${bfile%.b}.s"
    exe_file="${bfile%.b}.out"
    $B_PARSER -S "$bfile" > "$asm_file"
    gcc -m32 -fno-pie -no-pie -o "$exe_file" "$asm_file"
    if [ -n "$expected_exit" ]; then
        ./$exe_file > actual.out 2>&1 || true
        actual_exit=$?
        if [ "$actual_exit" = "$expected_exit" ]; then
            echo "  PASS (exit code $actual_exit)"
            PASS=$((PASS+1))
        else
            echo "  FAIL (got exit $actual_exit, expected $expected_exit)"
            FAIL=$((FAIL+1))
        fi
    else
        actual=$(./$exe_file)
        if [ "$actual" = "$expected" ]; then
            echo "  PASS"
            PASS=$((PASS+1))
        else
            echo "  FAIL"
            echo "    Got:      $actual"
            echo "    Expected: $expected"
            FAIL=$((FAIL+1))
        fi
    fi
done

echo "Tests passed: $PASS"
echo "Tests failed: $FAIL"
exit $FAIL 
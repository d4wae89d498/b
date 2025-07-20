# Compile-time Extensible B Compiler

This project aims to extend the historical **B language** with a novel compile-time meta-programming feature:
`meta { ... main() { ... } ... }` blocks that are **JIT-compiled and executed during compilation**.

---

## Overview

The goal is to enable *full compile-time extensibility* of the compiler itself by running user-defined code inside `meta` blocks. This code would freely:

* Access and modify all compiler internals exposed via `b.h`.
* Override or hook compiler functions - `b.c` would uses **function pointers internally**.
* Alter or add new language syntax and backends dynamically during compilation.


---

## Why B?

B is a minimal language, just above assembly. It allows incremental introduction of new syntax while staying compatible with compiler binaries. B’s position in the historical chain (**B → C → C++**) and its low-level nature make it a suitable substrate for an extensible ecosystem experiment.



Features such as type checking or new semantics could be implemented as compile-time extensions with layered dependencies. A package manager could distribute compiler extensions and syntactic modules with dependency tracking.


---

## Project roadmap / scope

* **JIT for compile-time extensions**

  * Targeting **x86 only** initially, to leverage the simple cdecl ABI for interaction with compiler-exposed functions.

* **Planned backends**

  * QBE (SSA-based IR)
  * C code generation
  * Native x86

---

## Example meta code snippet

```c
meta
{
  parse_identifier(p)
  {
    extern _parse_name;
    extern _parser_skip_ws;
  
        n = make_node(AST_VAR);
//...
        n[STRUCT_AST_VAR_NAME] = name;
        return n;
    }

    main()
    {
        extern parse_identifier;

        parse_identifier = &my_parse_identifier;
    }
}
```

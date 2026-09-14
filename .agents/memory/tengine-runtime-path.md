---
name: T-Engine runtime paths
description: Execution modes and native build constraint for the T-Engine compiler.
---

T-Engine keeps the AST interpreter as a reference/debug path and exposes the bytecode VM through the explicit `--vm` CLI flag until the instruction set matures.

**Why:** The VM is new and should be compared against the simpler interpreter while compiler semantics continue to evolve.

**How to apply:** Preserve both execution paths when changing language features, and verify examples through both when practical. The verified native build path is `make`; do not assume CMake is installed in the base shell.

Built-in functions are registered once in the shared builtin registry and implemented by both runtime paths.

**Why:** Duplicated name/arity/type rules caused semantic drift risk between the interpreter and VM.

**How to apply:** Add a builtin to the shared registry first, then wire its type-check and runtime behavior in both execution paths.
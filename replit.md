# T-Engine

Compiler dan runtime untuk membangun bahasa pemrograman baru dengan frontend
C++20 dan runtime C11.

## Run & Operate

- `pnpm --filter @workspace/api-server run dev` — run the API server (port 5000)
- `pnpm run typecheck` — full typecheck across all packages
- `pnpm run build` — typecheck + build all packages
- `pnpm --filter @workspace/api-spec run codegen` — regenerate API hooks and Zod schemas from the OpenAPI spec
- `pnpm --filter @workspace/db run push` — push DB schema changes (dev only)
- Required env: `DATABASE_URL` — Postgres connection string
- `make` — build CLI T-Engine
- `make test` — run frontend, type checker, interpreter, and bytecode VM tests
- `./build/tengine --vm examples/control_flow.te` — run through bytecode VM

## Stack

- pnpm workspaces, Node.js 24, TypeScript 5.9
- API: Express 5
- DB: PostgreSQL + Drizzle ORM
- Validation: Zod (`zod/v4`), `drizzle-zod`
- API codegen: Orval (from OpenAPI spec)
- Build: esbuild (CJS bundle)
- Engine: C++20 lexer/parser/type checker/interpreter and C11 runtime ABI

## Where things live

- `include/tengine/` — public compiler, AST, interpreter, and bytecode interfaces
- `src/` — lexer, parser, type checker, interpreter, bytecode compiler, and CLI
- `runtime/` — C11 output runtime exposed through a C ABI
- `examples/` — executable T-Engine programs
- `tests/` — native compiler/runtime checks

## Architecture decisions

- The AST interpreter remains available as a debugging/reference execution path.
- The bytecode VM is an opt-in CLI mode (`--vm`) until its instruction set matures.
- The runtime boundary stays C-compatible so future native backends can reuse it.

## Product

T-Engine currently supports variables, primitive values, arithmetic, comparisons,
boolean logic, functions, local scopes, `if/else`, `while`, a static type checker,
AST interpretation, and bytecode execution.

## User preferences

_Populate as you build — explicit user instructions worth remembering across sessions._

## Gotchas

- The CMake toolchain may not be available in the base shell; `Makefile` is the
  verified local build path.

## Pointers

- See the `pnpm-workspace` skill for workspace structure, TypeScript setup, and package details

# Contributing

Contributions must preserve these project rules:

- C99 is the minimum supported C language level.
- Capacity is fixed at initialization time.
- Storage is caller-owned.
- Dynamic allocation is out of scope.
- Hidden global mutable state is out of scope.
- OS and RTOS dependencies are out of scope.
- Public pointer-borrow APIs are out of scope.
- Tests must not be weakened or removed to make failures disappear.
- Compile-fail tests must not be removed.
- Tags and releases are only created when explicitly requested.

When behavior changes, update code, tests, and docs together.

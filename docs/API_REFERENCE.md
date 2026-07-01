# API Reference

Header: `#include "mring.h"`

## Initialization

```c
mring_err_t mring_init(
    mring_t *ring,
    void *storage,
    size_t storage_size,
    uint32_t capacity,
    size_t elem_size);
```

Validation includes null checks, power-of-two capacity, non-zero element size, multiplication
overflow, storage sufficiency, and ring/storage non-overlap.

## Concurrency

- Default mode: `MRING_CONCURRENCY_SINGLE_CONTEXT`
- Optional mode: `MRING_CONCURRENCY_SPSC_ATOMIC`
- Generic `mring_count()` and `mring_free()` are snapshot queries, not synchronization primitives.

## Queries

All query functions return `mring_err_t` and write through output parameters:

- `mring_capacity`
- `mring_element_size`
- `mring_count`
- `mring_free`
- `mring_is_empty`
- `mring_is_full`

## Copy Operations

- `mring_push` copies `elem_size` bytes from caller memory before returning.
- `mring_pop` and `mring_peek` copy `elem_size` bytes to caller memory before returning.
- `mring_pop_many` supports discard mode with `elements == NULL`.
- Batch source and destination ranges must not overlap ring backing storage.

## Overwrite

`mring_push_overwrite` is only supported in single-context builds. Atomic SPSC builds return
`MRING_ERR_UNSUPPORTED`.

## Typed Wrappers

`MRING_DEFINE_TYPED(prefix, type)` generates `prefix_push`, `prefix_pop`, and `prefix_peek`.
Wrappers check `mring_element_size(ring) == sizeof(type)` and return `MRING_ERR_TYPE` on mismatch.

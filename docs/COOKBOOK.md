# Cookbook

## 1. Minimal sized initialization

```c
mring_t ring;
uint8_t storage[8U * sizeof(uint32_t)];
mring_init(&ring, storage, sizeof(storage), 8U, sizeof(uint32_t));
```

## 2. Exact storage size

Use `capacity * elem_size` bytes exactly. Oversizing is allowed, undersizing is rejected.

## 3. Scalar push/pop

Use `mring_push()` and `mring_pop()` with caller-owned objects.

## 4. Struct push/pop

Store structs by value; the ring copies `elem_size` bytes on each operation.

## 5. Safe peek and `peek_at`

Use `mring_peek()` or `mring_peek_at()` to copy data out without borrowing pointers into ring storage.

## 6. Batch push/pop

`mring_push_many()` and `mring_pop_many()` return status plus exact transferred counts.

## 7. Discard batch pop

Pass `elements == NULL` to `mring_pop_many()` to discard a transferred prefix.

## 8. Overwrite mode

`mring_push_overwrite()` is for single-context or externally serialized use only.

## 9. Typed wrapper matched type

`MRING_DEFINE_TYPED(prefix, type)` checks `mring_element_size(ring) == sizeof(type)` before copying.

## 10. Typed wrapper mismatch

Mismatches return `MRING_ERR_TYPE` and do not copy data.

## 11. Multiple rings

Each ring instance carries its own storage pointer and metadata. No global state is used.

## 12. C++ consumer

The public header already provides `extern "C"` guards for C++ consumers.

## 13. Installed package consumer

Use `find_package(micoring CONFIG REQUIRED)` and link `micoring::micoring`.

## 14. `add_subdirectory` consumer

Link the exported alias target `micoring::micoring`.

## 15. Single-context main-loop pattern

Own one ring instance from one execution context at a time unless you add external serialization.

## 16. SPSC atomic mode

Assign one producer to `push` and one consumer to `pop`/`peek`. Treat generic snapshot queries as quiescent diagnostics.

## 17. External locking pattern

Wrap all ring operations in the caller’s critical section or mutex when multiple contexts touch one ring.

## 18. Counter wrap

The implementation relies on monotonic unsigned counters and power-of-two capacity.

## 19. Storage lifetime and non-overlap

Keep backing storage valid and exclusively associated with the ring for the full usage lifetime.

## 20. Abnormal termination limits

No cleanup callbacks are run on `abort`, `_Exit`, reset, power loss, faults, or interrupted control flow.

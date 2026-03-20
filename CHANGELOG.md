# Changelog

All notable changes to this project will be documented in this file.

## [1.0.0] - 2026-03-20

### Added

- Generic ring buffer with configurable element size
- Power-of-2 capacity with bitmask wrapping
- Lock-free SPSC operation
- Push, pop, peek, and `peek_at` operations
- Batch `push_many` and `pop_many`
- Overwrite mode via `mring_push_overwrite`
- Direct pointer access with `mring_ptr_at`
- Typed wrapper macros through `MRING_DEFINE_TYPED`
- Test suite covering wraparound, batch, overwrite, structs, and edge cases

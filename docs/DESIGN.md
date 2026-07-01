# Design

## Fixed-capacity storage

The library uses caller-owned fixed storage and never allocates dynamically.

## Monotonic counters

`head` and `tail` are monotonic `uint32_t` counters. Occupancy is derived from unsigned subtraction.

## Copy-based API

The public API exposes copy operations only. Direct typed pointer borrowing was removed because it
cannot provide a safe general contract for alignment, effective type, overwrite lifetime, or
concurrent publication.

## Concurrency split

The default build is single-context. Optional atomic SPSC mode uses acquire/release publication on
fixed-width counters without exposing `_Atomic` in the public ABI.

## Failure model

Errors are returned synchronously. The library does not attempt cleanup or recovery after abnormal
termination.

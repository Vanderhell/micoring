# Porting Guide

## Supported surface

- C99 library consumers
- C11 library consumers
- C++ consumers through the guarded public header
- GCC, Clang, and MSVC

## Concurrency modes

- `single-context`: portable baseline
- `spsc-atomic`: requires supported compiler atomics or explicit user hooks

## Cross compilation

Cortex-M0/M0+, Cortex-M4, and similar targets should treat repository CI as compile-only evidence
unless the code has been executed on target hardware.

## Notes

- `volatile` is not used as a general thread-safety mechanism.
- Atomic SPSC mode is not multi-producer or multi-consumer.
- Overwrite mode is not supported in atomic SPSC builds.

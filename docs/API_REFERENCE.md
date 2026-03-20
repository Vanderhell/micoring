# API Reference

> Header: `#include "mring.h"`  
> Version: `1.0.0`

## Error Codes

| Code | Meaning |
|------|---------|
| `MRING_OK` | Success |
| `MRING_ERR_NULL` | NULL pointer |
| `MRING_ERR_FULL` | Buffer full |
| `MRING_ERR_EMPTY` | Buffer empty |
| `MRING_ERR_INVALID` | Invalid configuration |
| `MRING_ERR_SIZE` | Offset out of range |

## Lock-Free SPSC Guarantee

One producer (`push`) and one consumer (`pop`) can run concurrently on
32-bit ARM without locks. The producer only writes `head`, the consumer only
writes `tail`, and both counters are stored as `volatile uint32_t`.

For multi-producer or multi-consumer usage, protect calls with a platform
critical section, mutex, or another synchronization primitive.

## Thread Safety Summary

| Operation | ISR-safe? | Notes |
|-----------|-----------|-------|
| `push` (single producer) | Yes | One producer only |
| `pop` (single consumer) | Yes | One consumer only |
| `push` and `pop` concurrent | Yes | Lock-free SPSC |
| `push` and `push` concurrent | No | External synchronization required |
| `clear` | No | Stop both sides first |
| `peek`, `count`, `free` | Usually safe | Snapshot can be stale |

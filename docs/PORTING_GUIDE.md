# Porting Guide

`micoring` needs `mring.h`, `mring.c`, and a C99 compiler. No platform
callbacks are required.

## Integration

```cmake
add_library(micoring STATIC lib/micoring/src/mring.c)
target_include_directories(micoring PUBLIC lib/micoring/include)
```

## Platform Notes

### ARM Cortex-M

Aligned 32-bit reads and writes are suitable for the intended SPSC pattern.
ISR-to-main-loop use works well without a mutex.

### ESP32

The same single-producer / single-consumer model works well for task-to-task or
ISR-to-task pipelines, assuming only one writer and one reader.

### Linux And Windows

Desktop builds are useful for testing, but `volatile` is not a full threading
model. For true multi-threaded desktop production code, prefer `_Atomic` or
another synchronization strategy.

### Arduino

```cpp
extern "C" {
#include "mring.h"
}
```

## Checklist

1. Use a C99 compiler.
2. Allocate at least `capacity * elem_size` bytes of storage.
3. Keep capacity as a power of 2.
4. Use one producer and one consumer for lock-free operation.
5. Add synchronization if multiple writers or readers exist.

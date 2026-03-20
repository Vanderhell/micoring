# micoring

[![CI](https://github.com/Vanderhell/micoring/actions/workflows/ci.yml/badge.svg)](https://github.com/Vanderhell/micoring/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![C Standard](https://img.shields.io/badge/C-C99-blue.svg)](https://en.cppreference.com/w/c/language)
[![Embedded](https://img.shields.io/badge/target-embedded%20systems-black.svg)](#use-cases)

Generic, ISR-safe ring buffer for embedded systems.

`C99` | `Zero dependencies` | `Zero allocations` | `Lock-free SPSC` | `Portable`

## Why micoring?

Embedded projects constantly need a queue between interrupt context and normal
execution: UART RX, event dispatch, sampled data, DMA handoff, logging, and
telemetry. `micoring` provides a compact, reusable ring buffer with a fixed API
instead of one-off implementations that tend to diverge in edge cases.

It is designed for:

- Bare-metal and RTOS-based embedded projects
- Single-producer / single-consumer pipelines
- Fixed-size element storage without heap allocation
- Portable C99 code that can also be tested on desktop targets

## Features

- Generic storage for any fixed-size element type
- Lock-free SPSC operation for one producer and one consumer
- Power-of-2 capacity for fast bitmask wrapping
- Single-item and batch push/pop operations
- `peek`, `peek_at`, and `mring_ptr_at` access helpers
- Overwrite mode for "latest N samples" use cases
- Typed wrapper macros via `MRING_DEFINE_TYPED`
- Small implementation with no external dependencies

## Quick Start

```c
#include "mring.h"
#include <stdint.h>

typedef struct {
    uint16_t id;
    float value;
} sample_t;

static uint8_t storage[16 * sizeof(sample_t)];
static mring_t ring;

void app_init(void)
{
    mring_init(&ring, storage, 16, sizeof(sample_t));
}

void producer_step(void)
{
    sample_t sample = { .id = 1, .value = 23.5f };
    mring_push(&ring, &sample);
}

void consumer_step(void)
{
    sample_t out;
    while (mring_pop(&ring, &out) == MRING_OK) {
        process_sample(out);
    }
}
```

## Use Cases

- UART RX buffering between ISR and main loop
- Event queues for finite state machines
- Sensor sample pipelines
- Logging backends with bounded memory
- Latest-value windows via overwrite mode

## Build And Test

### Local build

```bash
cc -std=c99 -Wall -Wextra -Wpedantic -Werror -Iinclude src/mring.c tests/test_all.c -o test_all
./test_all
```

### Make-based test run

```bash
cd tests
make
```

## API Summary

| Function | Purpose |
|----------|---------|
| `mring_init` | Initialise a ring buffer instance |
| `mring_push` / `mring_pop` | Push or pop one element |
| `mring_peek` / `mring_peek_at` | Read data without removing it |
| `mring_push_many` / `mring_pop_many` | Batch operations |
| `mring_push_overwrite` | Push while discarding the oldest item if full |
| `mring_ptr_at` | Direct pointer access to an element |
| `mring_count` / `mring_free` | Occupancy helpers |
| `mring_clear` | Reset the ring |
| `MRING_DEFINE_TYPED` | Generate typed wrappers |

See [docs/API_REFERENCE.md](docs/API_REFERENCE.md) for the detailed behavior and
concurrency notes.

## Constraints

| Constraint | Requirement |
|-----------|-------------|
| Capacity | Must be a power of 2 |
| Storage buffer | Must be at least `capacity * elem_size` bytes |
| Element size | Must be greater than 0 |
| Concurrency model | Lock-free for one producer and one consumer |

For multi-producer or multi-consumer scenarios, use external synchronization.

## Repository Layout

- `include/mring.h`: public API
- `src/mring.c`: implementation
- `tests/test_all.c`: unit tests
- `docs/`: design rationale, API notes, and porting guidance

## Documentation

- [API reference](docs/API_REFERENCE.md)
- [Design rationale](docs/DESIGN.md)
- [Porting guide](docs/PORTING_GUIDE.md)
- [Changelog](CHANGELOG.md)
- [Contributing guide](CONTRIBUTING.md)

## Ecosystem

The library is intended to fit cleanly into embedded utility stacks such as:

- [microsh](https://github.com/Vanderhell/microsh)
- [microlog](https://github.com/Vanderhell/microlog)
- [microfsm](https://github.com/Vanderhell/microfsm)
- [iotspool](https://github.com/Vanderhell/iotspool)

## License

Released under the MIT License. See [LICENSE](LICENSE).

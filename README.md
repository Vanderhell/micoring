# micoring

[![CI](https://github.com/Vanderhell/micoring/actions/workflows/ci.yml/badge.svg)](https://github.com/Vanderhell/micoring/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

`micoring` is a fixed-capacity ring buffer for caller-owned storage.

The default build is portable single-context C99. An optional `MRING_CONCURRENCY_SPSC_ATOMIC`
configuration supports one producer and one consumer with release/acquire publication on
supported compiler backends. The library does not perform internal locking, does not allocate,
and does not provide pointer-borrow APIs.

## Highlights

- Caller-owned fixed storage
- Sized initialization with overflow checks
- Copy-based push, pop, peek, and `peek_at`
- Batch push/pop with exact transferred counts
- Stable public ABI based on fixed-width integers and `size_t`
- CMake package export and `add_subdirectory` support

## Quick Start

```c
#include "mring.h"
#include <stdint.h>

typedef struct {
    uint16_t id;
    uint16_t state;
    uint32_t reading;
} sample_t;

static uint8_t storage[8U * sizeof(sample_t)];
static mring_t ring;

int app_init(void)
{
    return mring_init(&ring, storage, sizeof(storage), 8U, sizeof(sample_t));
}
```

## Concurrency Model

- `MRING_CONCURRENCY_SINGLE_CONTEXT`: default. One execution context owns one ring instance at a time. Any task/thread/ISR/core sharing requires external serialization.
- `MRING_CONCURRENCY_SPSC_ATOMIC`: optional. One producer owns `push`; one consumer owns `pop` and `peek`. Generic snapshot queries are still quiescent or externally synchronized operations.
- `mring_push_overwrite()` is not a concurrent SPSC primitive. In atomic mode it returns `MRING_ERR_UNSUPPORTED`.

## Build

### CMake

```bash
cmake -S . -B build -DMRING_BUILD_TESTS=ON -DMRING_CONCURRENCY_MODE=single-context
cmake --build build
ctest --test-dir build
```

### Make

```bash
make
```

`CC`, `CPPFLAGS`, `CFLAGS`, and `LDFLAGS` are honored.

## Documentation

- [API reference](docs/API_REFERENCE.md)
- [Cookbook](docs/COOKBOOK.md)
- [Design notes](docs/DESIGN.md)
- [Porting guide](docs/PORTING_GUIDE.md)
- [Troubleshooting](docs/TROUBLESHOOTING.md)
- [Verification status](docs/VERIFICATION.md)
- [Contributing](CONTRIBUTING.md)
- [Security](SECURITY.md)
- [Changelog](CHANGELOG.md)

## Release Policy

Releases are tag-based. The repository workflow only creates GitHub releases for pushed tags
matching `v*`; branch pushes do not create releases.
